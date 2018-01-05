#ifndef __SQLDELAYTHREAD_H
#define __SQLDELAYTHREAD_H

#include "ace/Thread_Mutex.h"
#include "LockedQueue.h"
#include "Threading.h"

class Database;
class SqlOperation;
class SqlConnection;

class SqlDelayThread : public ACE_Based::Runnable
{
	typedef ACE_Based::LockedQueue<SqlOperation*, ACE_Thread_Mutex> SqlQueue;

    public:
        SqlDelayThread(Database* db, SqlConnection* conn);
        ~SqlDelayThread();

        ///< Put sql statement to delay queue
        bool Delay(SqlOperation* sql);

        virtual void Stop();                                ///< Stop event
        virtual void run();                                 ///< Main Thread loop

    private:
		SqlQueue 		m_sqlQueue;         				///< Queue of SQL statements
		Database* 		m_dbEngine;              			///< Pointer to used Database engine
		SqlConnection* 	m_dbConnection;          			///< Pointer to DB connection
		volatile bool 	m_running;

		// process all enqueued requests
		void ProcessRequests();
};
#endif                                                      //__SQLDELAYTHREAD_H
