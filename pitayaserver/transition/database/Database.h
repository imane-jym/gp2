#ifndef __DATABASE_H__
#define __DATABASE_H__

#include "Common.h"
#include "Threading.h"
#include "Log.h"

#include <ace/Null_Mutex.h>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/TSS_T.h>
#include <ace/Atomic_Op.h>

class SqlTransaction;
class SqlResultQueue;
class SqlQueryBatch;
class Database;
class QueryResult;
class SqlDelayThread;

#define MAX_QUERY_LEN   64*1024

class SqlConnection
{
    public:
        virtual ~SqlConnection() {}

        // method for initializing DB connection
        virtual bool Initialize(const char* infoString) = 0;

        // public methods for making queries
        virtual QueryResult* Query(const char* sql) = 0;
        // public methods for making requests
        virtual bool Execute(const char* sql) = 0;

        // escape string generation
        virtual unsigned long escape_string(char* to, const char* from, unsigned long length) { strncpy(to, from, length); return length; }

        // nothing do if DB not support transactions
        virtual bool BeginTransaction() { return true; }
        virtual bool CommitTransaction() { return true; }
        // can't rollback without transaction support
        virtual bool RollbackTransaction() { return false; }

        virtual void Ping() {}

        // SqlConnection object lock
        class Lock
        {
            public:
                Lock(SqlConnection* conn) : m_pConn(conn) { m_pConn->m_mutex.acquire(); }
                ~Lock() { m_pConn->m_mutex.release(); }

                SqlConnection* operator->() const { return m_pConn; }

            private:
                SqlConnection* const m_pConn;
        };

        // get DB object
        Database& DB() { return m_db; }

    protected:
        SqlConnection(Database& db) : m_db(db) {}

        Database& m_db;

    private:
//        typedef ACE_Null_Mutex LOCK_TYPE;
        typedef ACE_Recursive_Thread_Mutex LOCK_TYPE;
        LOCK_TYPE m_mutex;
};

class Database
{
    public:
        virtual ~Database();

        virtual bool Initialize(const char* infoString);
        // start worker thread for async DB request execution
        virtual void InitDelayThread();
        // stop worker thread
        virtual void HaltDelayThread();

        void StopServer();

        ///////////////////////////////
        /// Synchronous DB queries/////
        ///////////////////////////////
        QueryResult* Query(const char* sql);
        QueryResult* PQuery(const char* format, ...) ATTR_PRINTF(2, 3);

        bool DirectExecute(const char* sql);
        bool DirectPExecute(const char* format, ...) ATTR_PRINTF(2, 3);

        /////////////////////////////////////////////////////////////////////////////
        /// Asynchronous queries and query holders, implemented in DatabaseImpl.h ///
        /////////////////////////////////////////////////////////////////////////////

        // Query / member
        template<class Class>
        bool AsyncQuery(Class* object, void (Class::*method)(QueryResult*), const char* sql);
        template<class Class, typename ParamType1>
        bool AsyncQuery(Class* object, void (Class::*method)(QueryResult*, ParamType1), ParamType1 param1, const char* sql);
        template<class Class, typename ParamType1, typename ParamType2>
        bool AsyncQuery(Class* object, void (Class::*method)(QueryResult*, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char* sql);
        template<class Class, typename ParamType1, typename ParamType2, typename ParamType3>
        bool AsyncQuery(Class* object, void (Class::*method)(QueryResult*, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char* sql);

        // Query / static
        template<typename ParamType1>
        bool AsyncQuery(void (*method)(QueryResult*, ParamType1), ParamType1 param1, const char* sql);
        template<typename ParamType1, typename ParamType2>
        bool AsyncQuery(void (*method)(QueryResult*, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char* sql);
        template<typename ParamType1, typename ParamType2, typename ParamType3>
        bool AsyncQuery(void (*method)(QueryResult*, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char* sql);

        // PQuery / member
        template<class Class>
        bool AsyncPQuery(Class* object, void (Class::*method)(QueryResult*), const char* format, ...) ATTR_PRINTF(4, 5);
        template<class Class, typename ParamType1>
        bool AsyncPQuery(Class* object, void (Class::*method)(QueryResult*, ParamType1), ParamType1 param1, const char* format, ...) ATTR_PRINTF(5, 6);
        template<class Class, typename ParamType1, typename ParamType2>
        bool AsyncPQuery(Class* object, void (Class::*method)(QueryResult*, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char* format, ...) ATTR_PRINTF(6, 7);
        template<class Class, typename ParamType1, typename ParamType2, typename ParamType3>
        bool AsyncPQuery(Class* object, void (Class::*method)(QueryResult*, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char* format, ...) ATTR_PRINTF(7, 8);

        // PQuery / static
        template<typename ParamType1>
        bool AsyncPQuery(void (*method)(QueryResult*, ParamType1), ParamType1 param1, const char* format, ...) ATTR_PRINTF(4, 5);
        template<typename ParamType1, typename ParamType2>
        bool AsyncPQuery(void (*method)(QueryResult*, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char* format, ...) ATTR_PRINTF(5, 6);
        template<typename ParamType1, typename ParamType2, typename ParamType3>
        bool AsyncPQuery(void (*method)(QueryResult*, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char* format, ...) ATTR_PRINTF(6, 7);

        // QueryBatch
        template<class Class>
        bool DelayQueryBatch(Class* object, void (Class::*method)(QueryResult*, SqlQueryBatch*), SqlQueryBatch* batch);
        template<class Class, typename ParamType1>
        bool DelayQueryBatch(Class* object, void (Class::*method)(QueryResult*, SqlQueryBatch*, ParamType1), SqlQueryBatch* batch, ParamType1 param1);

        // QueryBatch / static
        template<typename ParamType1>
        bool DelayQueryBatch(void (*method)(QueryResult*, SqlQueryBatch*, ParamType1), SqlQueryBatch* batch, ParamType1 param1 );

        // Execute (Update SQL) Without CallBack
        bool Execute(const char* sql);
        bool PExecute(const char* format, ...) ATTR_PRINTF(2, 3);



        bool BeginTransaction();
        bool CommitTransaction();
        bool RollbackTransaction();
        // for sync transaction execution
        bool CommitTransactionDirect();

        operator bool () const { return m_pSyncConn != 0 && m_pAsyncConn != 0; }

        // escape string generation
        void escape_string(std::string& str);

        // must be called before first query in thread (one time for thread using one from existing Database objects)
        virtual void ThreadStart();
        // must be called before finish thread run (one time for thread using one from existing Database objects)
        virtual void ThreadEnd();

        // set database-wide result queue. also we should use object-bases and not thread-based result queues
        void ProcessResultQueue();

        uint32 GetPingIntervall() { return m_pingIntervallms; }

        // function to ping database connections
        void Ping();

        // set this to allow async transactions
        // you should call it explicitly after your server successfully started up
        // NO ASYNC TRANSACTIONS DURING SERVER STARTUP - ONLY DURING RUNTIME!!!
        void AllowAsyncTransactions() { m_bAllowAsyncTransactions = true; }

    protected:
        Database() :
            m_pSyncConn(NULL), m_pAsyncConn(NULL), m_pResultQueue(NULL),
            m_threadBody(NULL), m_delayThread(NULL), m_bAllowAsyncTransactions(false),
            m_pingIntervallms(0)
        {
        }

        // factory method to create SqlConnection objects
        virtual SqlConnection* CreateConnection() = 0;
        // factory method to create SqlDelayThread objects
        virtual SqlDelayThread* CreateDelayThread();

        class TransHelper
        {
            public:
                TransHelper() : m_pTrans(NULL) {}
                ~TransHelper();

                // initializes new SqlTransaction object
                SqlTransaction* init();
                // gets pointer on current transaction object. Returns NULL if transaction was not initiated
                SqlTransaction* get() const { return m_pTrans; }
                // detaches SqlTransaction object allocated by init() function
                // next call to get() function will return NULL!
                // do not forget to destroy obtained SqlTransaction object!
                SqlTransaction* detach();
                // destroyes SqlTransaction allocated by init() function
                void reset();

            private:
                SqlTransaction* m_pTrans;
        };

        // per-thread based storage for SqlTransaction object initialization - no locking is required
        typedef ACE_TSS<Database::TransHelper> DBTransHelperTSS;
        Database::DBTransHelperTSS m_TransStorage;

        ///< DB connections

        SqlConnection* getQueryConnection() const { return m_pSyncConn; }
        // for now return one single connection for async requests
        SqlConnection* getAsyncConnection() const { return m_pAsyncConn; }

        // DB connection for sync queries
        SqlConnection* m_pSyncConn;

        // DB connection for asyn queries
        SqlConnection* m_pAsyncConn;

        SqlResultQueue*     m_pResultQueue;                 ///< Transaction queues from diff. threads
        SqlDelayThread*     m_threadBody;                   ///< Pointer to delay SQL executer (owned by m_delayThread)
        ACE_Based::Thread* m_delayThread;                   ///< Pointer to executer thread

        bool m_bAllowAsyncTransactions;                     ///< flag which specifies if async transactions are enabled

    private:

        uint32 m_pingIntervallms;
};
#endif
