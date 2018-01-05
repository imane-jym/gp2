#include "SqlOperation.h"
#include "SqlDelayThread.h"
#include "Database.h"
#include "DatabaseImpl.h"
#include "QueryResult.h"

/// ---- ASYNC STATEMENTS / TRANSACTIONS ----

bool SqlPlainRequest::Execute(SqlConnection* conn)
{
    /// just do it
	SqlConnection::Lock guard(conn);
    return conn->Execute(m_sql);
}

SqlTransaction::~SqlTransaction()
{
    while (!m_queue.empty())
    {
        delete m_queue.back();
        m_queue.pop_back();
    }
}

bool SqlTransaction::Execute(SqlConnection* conn)
{
    if (m_queue.empty())
        return true;

    SqlConnection::Lock guard(conn);

    conn->BeginTransaction();

    const int nItems = m_queue.size();
    for (int i = 0; i < nItems; ++i)
    {
        SqlOperation* pOprt = m_queue[i];

        if ( !pOprt->Execute(conn) )
        {
            conn->RollbackTransaction();
            return false;
        }
    }

    return conn->CommitTransaction();
}

/// ---- ASYNC QUERIES ----

bool SqlQuery::Execute(SqlConnection* conn)
{
    if (!m_callback || !m_queue)
        return false;

    SqlConnection::Lock guard(conn);

    /// execute the query and store the result in the callback
    m_callback->SetResult(conn->Query(m_sql));
    /// add the callback to the sql result queue of the thread it originated from
    m_queue->add(m_callback);

    return true;
}

void SqlResultQueue::Update()
{
    /// execute the callbacks waiting in the synchronization queue
	Util::IQueryCallback* callback = NULL;
    while (next(callback))
    {
        callback->Execute();
        delete callback;
    }
}

/////////////////////  SqlQueryBatch //////////////////////////

bool SqlQueryBatch::Execute(Util::IQueryCallback* callback, SqlDelayThread* thread, SqlResultQueue* queue)
{
    if (!callback || !thread || !queue)
        return false;

    /// delay the execution of the queries, sync them with the delay thread
    /// which will in turn resync on execution (via the queue) and call back
    SqlQueryBatchOperation* holderEx = new SqlQueryBatchOperation(this, callback, queue);
    thread->Delay(holderEx);
    return true;
}

bool SqlQueryBatch::ExecuteDirect( Database* db )
{
	for ( size_t i = 0; i < m_queries.size(); ++i )
	{
		const char* sql = m_queries[i].first;
		if ( sql ) SetResult(i, db->Query(sql));
	}

	return true;
}

bool SqlQueryBatch::PushQuery(const char* sql)
{
	m_queries.push_back( SqlResultPair( strdup_new(sql), (QueryResult*)NULL ) );
	return true;
}

bool SqlQueryBatch::PushPQuery(const char* format, ... )
{
	if (!format)
	{
		IME_SQLERROR( "Query empty." );
		return false;
	}

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

	return PushQuery( szQuery );
}

size_t SqlQueryBatch::GetSize()
{
	return m_queries.size();
}

//bool SqlQueryBatch::SetQuery(size_t index, const char* sql)
//{
//    if (m_queries.size() <= index)
//    {
//    	IME_SQLERROR( "Query index ( %d ) out of range (size: %d ) for query: %s", (int)index, (int)m_queries.size(), sql );
//        return false;
//    }
//
//    if (m_queries[index].first != NULL)
//    {
//    	IME_SQLERROR( "Attempt assign query to holder index ( %d ) where other query stored (Old: [%s] New: [%s])",
//    			(int)index, m_queries[index].first, sql );
//        return false;
//    }
//
//    /// not executed yet, just stored (it's not called a holder for nothing)
//    m_queries[index] = SqlResultPair(strdup_new(sql), (QueryResult*)NULL);
//    return true;
//}

//bool SqlQueryBatch::SetPQuery(size_t index, const char* format, ...)
//{
//    if (!format)
//    {
//    	IME_SQLERROR( "Query (index: %d) is empty.", (int)index );
//        return false;
//    }
//
//    va_list ap;
//    char szQuery [MAX_QUERY_LEN];
//    va_start(ap, format);
//    int res = vsnprintf(szQuery, MAX_QUERY_LEN, format, ap);
//    va_end(ap);
//
//    if (res == -1)
//    {
//    	IME_SQLERROR( "SQL Query truncated (and not execute) for format: %s", format );
//        return false;
//    }
//
//    return SetQuery(index, szQuery);
//}

QueryResult* SqlQueryBatch::GetResult(size_t index)
{
    if (index < m_queries.size())
    {
        /// the query strings are freed on the first GetResult or in the destructor
        if (m_queries[index].first != NULL)
        {
            delete[](const_cast<char*>(m_queries[index].first));
            m_queries[index].first = NULL;
        }
        /// when you get a result aways remember to delete it!
        return m_queries[index].second;
    }
    else
        return NULL;
}

void SqlQueryBatch::SetResult(size_t index, QueryResult* result)
{
    /// store the result in the holder
    if (index < m_queries.size())
        m_queries[index].second = result;
}

SqlQueryBatch::~SqlQueryBatch()
{
    for (size_t i = 0; i < m_queries.size(); ++i)
    {
        /// if the result was never used, free the resources
        /// results used already (getresult called) are expected to be deleted
        if (m_queries[i].first != NULL)
        {
            delete[](const_cast<char*>(m_queries[i].first));
            delete m_queries[i].second;
        }
    }
}

//void SqlQueryBatch::SetSize(size_t size)
//{
//    /// to optimize push_back, reserve the number of queries about to be executed
//    m_queries.resize(size);
//}

bool SqlQueryBatchOperation::Execute(SqlConnection* conn)
{
    if (!m_batch || !m_callback || !m_queue)
        return false;

    SqlConnection::Lock guard(conn);

    /// we can do this, we are friends
    std::vector<SqlQueryBatch::SqlResultPair>& queries = m_batch->m_queries;

    conn->BeginTransaction(); // shall we?
    for (size_t i = 0; i < queries.size(); ++i)
    {
        /// execute all queries in the holder and pass the results
        char const* sql = queries[i].first;
        if (sql) m_batch->SetResult(i, conn->Query(sql)); // if SQL isn't a select statement, result will be null
    }
    conn->CommitTransaction();

    /// sync with the caller thread
    m_queue->add(m_callback);

    return true;
}
