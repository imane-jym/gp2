#include "Database.h"
#include "SqlOperation.h"
#include "QueryResult.h"
#include "SqlDelayThread.h"
#include "Common.h"
#include "Log.h"
#include "Config.h"

#include <ctime>
#include <iostream>
#include <fstream>
#include <memory>

//////////////////////////////////////////////////////////////////////////
Database::~Database()
{
    StopServer();
}

bool Database::Initialize(const char* infoString)
{
    m_pingIntervallms = sConfig->GetIntDefault("database.ping_interval", 300) * IN_MILLISECONDS;

    // create DB connections
	m_pSyncConn = CreateConnection();
	if ( !m_pSyncConn->Initialize( infoString ) )
		return false;

    m_pAsyncConn = CreateConnection();
    if ( !m_pAsyncConn->Initialize( infoString ) )
        return false;

    m_pResultQueue = new SqlResultQueue;

    InitDelayThread();
    return true;
}

void Database::StopServer()
{
    HaltDelayThread();

    delete m_pResultQueue;
    delete m_pSyncConn;
    delete m_pAsyncConn;

    m_pResultQueue 	= NULL;
    m_pSyncConn		= NULL;
    m_pAsyncConn 	= NULL;
}

SqlDelayThread* Database::CreateDelayThread()
{
    ASSERT(m_pAsyncConn);
    return new SqlDelayThread(this, m_pAsyncConn);
}

void Database::InitDelayThread()
{
	ASSERT(!m_delayThread);

    // New delay thread for delay execute
    m_threadBody = CreateDelayThread();              // will deleted at m_delayThread delete
    m_delayThread = new ACE_Based::Thread(m_threadBody);
}

void Database::HaltDelayThread()
{
    if (!m_threadBody || !m_delayThread) return;

    m_threadBody->Stop();                                   // Stop event
    m_delayThread->wait();                                  // Wait for flush to DB
    delete m_delayThread;                                   // This also deletes m_threadBody
    m_delayThread = NULL;
    m_threadBody = NULL;
}

void Database::ThreadStart()
{
}

void Database::ThreadEnd()
{
}

void Database::ProcessResultQueue()
{
    if (m_pResultQueue)
        m_pResultQueue->Update();
}

void Database::escape_string(std::string& str)
{
    if (str.empty())
        return;

    char* buf = new char[str.size() * 2 + 1];
    // we don't care what connection to use - escape string will be the same
    m_pSyncConn->escape_string( buf, str.c_str(), str.size() );
    str = buf;
    delete[] buf;
}

void Database::Ping()
{
    {
        SqlConnection::Lock guard(m_pAsyncConn);
        guard->Ping();
    }

    {
    	SqlConnection::Lock guard(m_pSyncConn);
    	guard->Ping();
    }
}

QueryResult* Database::Query( const char* sql )
{
	SqlConnection::Lock guard(m_pSyncConn);
	return guard->Query(sql);
}

QueryResult* Database::PQuery(const char* format, ...)
{
    if (!format) return NULL;

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf(szQuery, MAX_QUERY_LEN, format, ap);
    va_end(ap);

    if (res == -1)
    {
    	IME_SQLERROR( "SQL Query truncated (and not execute) for format: %s", format );
        return NULL;
    }

    return Query(szQuery);
}

bool Database::Execute(const char* sql)
{
    if (!m_pAsyncConn)
        return false;

    SqlTransaction* pTrans = m_TransStorage->get();
    if (pTrans)
    {
        // add SQL request to trans queue
        pTrans->DelayExecute(new SqlPlainRequest(sql));
    }
    else
    {
        // if async execution is not available
        if (!m_bAllowAsyncTransactions)
            return DirectExecute(sql);

        // Simple sql statement
        m_threadBody->Delay(new SqlPlainRequest(sql));
    }

    return true;
}

bool Database::PExecute(const char* format, ...)
{
    if (!format)
        return false;

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf(szQuery, MAX_QUERY_LEN, format, ap);
    va_end(ap);

    if (res == -1)
    {
    	IME_SQLERROR( "SQL Query truncated (and not execute) for format: %s", format );
        return false;
    }

    return Execute(szQuery);
}

bool Database::DirectExecute( const char* sql )
{
	if (!m_pSyncConn)
		return false;

	SqlConnection::Lock guard(m_pSyncConn);
	return guard->Execute(sql);
}

bool Database::DirectPExecute(const char* format, ...)
{
    if (!format)
        return false;

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf(szQuery, MAX_QUERY_LEN, format, ap);
    va_end(ap);
    if (res == -1)
    {
    	IME_SQLERROR( "SQL Query truncated (and not execute) for format: %s", format );
        return false;
    }

    return DirectExecute(szQuery);
}

bool Database::BeginTransaction()
{
    // initiate transaction on current thread
    // currently we do not support queued transactions
    m_TransStorage->init();
    return true;
}

bool Database::CommitTransaction()
{
    // check if we have pending transaction
    if (!m_TransStorage->get())
        return false;

    // if async execution is not available
    if (!m_bAllowAsyncTransactions)
        return CommitTransactionDirect();

    // add SqlTransaction to the async queue
    m_threadBody->Delay(m_TransStorage->detach());
    return true;
}

bool Database::CommitTransactionDirect()
{
    // check if we have pending transaction
    if (!m_TransStorage->get())
        return false;

    // directly execute SqlTransaction
    SqlTransaction* pTrans = m_TransStorage->detach();
    pTrans->Execute(m_pSyncConn);
    delete pTrans;

    return true;
}

bool Database::RollbackTransaction()
{
    if (!m_TransStorage->get())
        return false;

    // remove scheduled transaction
    m_TransStorage->reset();

    return true;
}

// HELPER CLASSES AND FUNCTIONS
Database::TransHelper::~TransHelper()
{
    reset();
}

SqlTransaction* Database::TransHelper::init()
{
    ASSERT(!m_pTrans);   // if we will get a nested transaction request - we MUST fix code!!!
    m_pTrans = new SqlTransaction;
    return m_pTrans;
}

SqlTransaction* Database::TransHelper::detach()
{
    SqlTransaction* pRes = m_pTrans;
    m_pTrans = NULL;
    return pRes;
}

void Database::TransHelper::reset()
{
    delete m_pTrans;
    m_pTrans = NULL;
}
