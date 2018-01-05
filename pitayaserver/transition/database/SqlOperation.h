#ifndef __SQLOPERATIONS_H
#define __SQLOPERATIONS_H

#include "ace/Thread_Mutex.h"
#include "LockedQueue.h"
#include <queue>
#include "Callback.h"
#include "Common.h"

class Database;
class SqlConnection;
class SqlDelayThread;

/// ---- ABSTRACT BASE ----
class SqlOperation
{
    public:
        virtual void OnRemove() { delete this; }
        virtual bool Execute(SqlConnection* conn) = 0;
        virtual ~SqlOperation() {}
};

/// ---- ASYNC STATEMENTS / TRANSACTIONS ----

// ( Without CallBack )

class SqlPlainRequest : public SqlOperation
{
    public:
        SqlPlainRequest( const char* sql ) :
        	m_sql( strdup_new(sql) ) {}
        ~SqlPlainRequest() { delete[] const_cast<char*>(m_sql); }

        bool Execute(SqlConnection* conn) override;

    private:
		const char* m_sql;
};

class SqlTransaction : public SqlOperation
{
    public:
        SqlTransaction() {}
        ~SqlTransaction();

        void DelayExecute(SqlOperation* sql) { m_queue.push_back(sql); }

        bool Execute(SqlConnection* conn) override;

    private:
		std::vector<SqlOperation* > m_queue;
};

/// ---- ASYNC QUERIES ----

class SqlQuery;                                             /// contains a single async query
class QueryResult;                                          /// the result of one
class SqlResultQueue;                                       /// queue for thread sync
class SqlQueryBatch;                                       	/// groups several async queries
class SqlQueryBatchOperationa;                    			/// points to a batch, added to the delay thread

class SqlResultQueue : public ACE_Based::LockedQueue<Util::IQueryCallback* , ACE_Thread_Mutex>
{
    public:
        SqlResultQueue() {}
        void Update();
};

class SqlQuery : public SqlOperation
{
    public:
        SqlQuery(const char* sql, Util::IQueryCallback* callback, SqlResultQueue* queue)
            : m_sql(strdup_new(sql)), m_callback(callback), m_queue(queue) {}
        ~SqlQuery() { delete[] const_cast<char*>(m_sql); }

        bool Execute(SqlConnection* conn) override;

    private:
	   const char* 				m_sql;
	   Util::IQueryCallback* 	m_callback;
	   SqlResultQueue* 			m_queue;
};

class SqlQueryBatch
{
	friend class SqlQueryBatchOperation;

    public:
        SqlQueryBatch() {}
        ~SqlQueryBatch();

//        bool SetQuery(size_t index, const char* sql);
//        bool SetPQuery(size_t index, const char* format, ...) ATTR_PRINTF(3, 4);
//        void SetSize(size_t size);

        bool PushQuery(const char* sql);
        bool PushPQuery(const char* format, ... ) ATTR_PRINTF(2, 3);
        size_t GetSize();

        QueryResult* GetResult(size_t index);

        void SetResult(size_t index, QueryResult* result);
        bool Execute(Util::IQueryCallback* callback, SqlDelayThread* thread, SqlResultQueue* queue);
        bool ExecuteDirect( Database* db );

    private:

		typedef std::pair<const char*, QueryResult*> SqlResultPair;
		std::vector<SqlResultPair> m_queries;
};

class SqlQueryBatchOperation : public SqlOperation
{
    public:
        SqlQueryBatchOperation(SqlQueryBatch* batch, Util::IQueryCallback* callback, SqlResultQueue* queue)
            : m_batch(batch), m_callback(callback), m_queue(queue) {}

        bool Execute(SqlConnection* conn) override;

    private:
		SqlQueryBatch* 			m_batch;
		Util::IQueryCallback* 	m_callback;
		SqlResultQueue* 		m_queue;
};
#endif
