#include "SqlDelayThread.h"
#include "SqlOperation.h"
#include "Database.h"

SqlDelayThread::SqlDelayThread(Database* db, SqlConnection* conn) : m_dbEngine(db), m_dbConnection(conn), m_running(true)
{
}

SqlDelayThread::~SqlDelayThread()
{
    // process all requests which might have been queued while thread was stopping
    ProcessRequests();
}

bool SqlDelayThread::Delay(SqlOperation* sql)
{
	m_sqlQueue.add(sql);
//	IME_LOG( "-------PUSH ( len = %u ) ------", (uint32)m_sqlQueue.size() );
	return true;
}

void SqlDelayThread::run()
{
	m_dbEngine->ThreadStart();

    const uint32 loopSleepms = 20;
    const uint32 pingEveryLoop = m_dbEngine->GetPingIntervall() / loopSleepms;

    uint32 loopCounter = 0;
    while (m_running)
    {
        // if the running state gets turned off while sleeping
        // empty the queue before exiting
        ACE_Based::Thread::Sleep(loopSleepms);

        ProcessRequests();

        if ((loopCounter++) >= pingEveryLoop)
        {
            loopCounter = 0;
            m_dbEngine->Ping();
        }
    }

    m_dbEngine->ThreadEnd();
}

void SqlDelayThread::Stop()
{
    m_running = false;
}

void SqlDelayThread::ProcessRequests()
{
    SqlOperation* s = NULL;
    while (m_sqlQueue.next(s))
    {
        s->Execute(m_dbConnection);
        delete s;
//        IME_LOG( "-------POP ( len = %u ) ------", (uint32)m_sqlQueue.size() );
    }
}
