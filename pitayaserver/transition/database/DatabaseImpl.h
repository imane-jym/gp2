#ifndef __DATABASE_IMPL_H__
#define __DATABASE_IMPL_H__

#include "Database.h"
#include "SqlOperation.h"
#include "SqlDelayThread.h"

/// Function body definitions for the template function members of the Database class

#define ASYNC_QUERY_BODY(sql) if (!sql || !m_pResultQueue) return false;
#define ASYNC_DELAYHOLDER_BODY(holder) if (!holder || !m_pResultQueue) return false;

#define ASYNC_PQUERY_BODY(format, szQuery) \
    if(!format) return false; \
    \
    char szQuery [MAX_QUERY_LEN]; \
    \
    { \
        va_list ap; \
        \
        va_start(ap, format); \
        int res = vsnprintf( szQuery, MAX_QUERY_LEN, format, ap ); \
        va_end(ap); \
        \
        if(res==-1) \
        { \
            IME_SQLERROR( "SQL Query truncated (and not execute) for format: %s",format ); \
            return false; \
        } \
    }

// -- Query / member --

template<class Class>
bool
Database::AsyncQuery(Class* object, void (Class::*method)(QueryResult*), const char* sql)
{
    ASYNC_QUERY_BODY(sql)
    return m_threadBody->Delay(new SqlQuery(sql, new Util::QueryCallback<Class>(object, method), m_pResultQueue));
}

template<class Class, typename ParamType1>
bool
Database::AsyncQuery(Class* object, void (Class::*method)(QueryResult*, ParamType1), ParamType1 param1, const char* sql)
{
    ASYNC_QUERY_BODY(sql)
    return m_threadBody->Delay(new SqlQuery(sql, new Util::QueryCallback<Class, ParamType1>(object, method, (QueryResult*)NULL, param1), m_pResultQueue));
}

template<class Class, typename ParamType1, typename ParamType2>
bool
Database::AsyncQuery(Class* object, void (Class::*method)(QueryResult*, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char* sql)
{
    ASYNC_QUERY_BODY(sql)
    return m_threadBody->Delay(new SqlQuery(sql, new Util::QueryCallback<Class, ParamType1, ParamType2>(object, method, (QueryResult*)NULL, param1, param2), m_pResultQueue));
}

template<class Class, typename ParamType1, typename ParamType2, typename ParamType3>
bool
Database::AsyncQuery(Class* object, void (Class::*method)(QueryResult*, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char* sql)
{
    ASYNC_QUERY_BODY(sql)
    return m_threadBody->Delay(new SqlQuery(sql, new Util::QueryCallback<Class, ParamType1, ParamType2, ParamType3>(object, method, (QueryResult*)NULL, param1, param2, param3), m_pResultQueue));
}

// -- Query / static --

template<typename ParamType1>
bool
Database::AsyncQuery(void (*method)(QueryResult*, ParamType1), ParamType1 param1, const char* sql)
{
    ASYNC_QUERY_BODY(sql)
    return m_threadBody->Delay(new SqlQuery(sql, new Util::SQueryCallback<ParamType1>(method, (QueryResult*)NULL, param1), m_pResultQueue));
}

template<typename ParamType1, typename ParamType2>
bool
Database::AsyncQuery(void (*method)(QueryResult*, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char* sql)
{
    ASYNC_QUERY_BODY(sql)
    return m_threadBody->Delay(new SqlQuery(sql, new Util::SQueryCallback<ParamType1, ParamType2>(method, (QueryResult*)NULL, param1, param2), m_pResultQueue));
}

template<typename ParamType1, typename ParamType2, typename ParamType3>
bool
Database::AsyncQuery(void (*method)(QueryResult*, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char* sql)
{
    ASYNC_QUERY_BODY(sql)
    return m_threadBody->Delay(new SqlQuery(sql, new Util::SQueryCallback<ParamType1, ParamType2, ParamType3>(method, (QueryResult*)NULL, param1, param2, param3), m_pResultQueue));
}

// -- PQuery / member --

template<class Class>
bool
Database::AsyncPQuery(Class* object, void (Class::*method)(QueryResult*), const char* format, ...)
{
    ASYNC_PQUERY_BODY(format, szQuery)
    return AsyncQuery(object, method, szQuery);
}

template<class Class, typename ParamType1>
bool
Database::AsyncPQuery(Class* object, void (Class::*method)(QueryResult*, ParamType1), ParamType1 param1, const char* format, ...)
{
    ASYNC_PQUERY_BODY(format, szQuery)
    return AsyncQuery(object, method, param1, szQuery);
}

template<class Class, typename ParamType1, typename ParamType2>
bool
Database::AsyncPQuery(Class* object, void (Class::*method)(QueryResult*, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char* format, ...)
{
    ASYNC_PQUERY_BODY(format, szQuery)
    return AsyncQuery(object, method, param1, param2, szQuery);
}

template<class Class, typename ParamType1, typename ParamType2, typename ParamType3>
bool
Database::AsyncPQuery(Class* object, void (Class::*method)(QueryResult*, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char* format, ...)
{
    ASYNC_PQUERY_BODY(format, szQuery)
    return AsyncQuery(object, method, param1, param2, param3, szQuery);
}

// -- PQuery / static --

template<typename ParamType1>
bool
Database::AsyncPQuery(void (*method)(QueryResult*, ParamType1), ParamType1 param1, const char* format, ...)
{
    ASYNC_PQUERY_BODY(format, szQuery)
    return AsyncQuery(method, param1, szQuery);
}

template<typename ParamType1, typename ParamType2>
bool
Database::AsyncPQuery(void (*method)(QueryResult*, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char* format, ...)
{
    ASYNC_PQUERY_BODY(format, szQuery)
    return AsyncQuery(method, param1, param2, szQuery);
}

template<typename ParamType1, typename ParamType2, typename ParamType3>
bool
Database::AsyncPQuery(void (*method)(QueryResult*, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char* format, ...)
{
    ASYNC_PQUERY_BODY(format, szQuery)
    return AsyncQuery(method, param1, param2, param3, szQuery);
}

// -- QueryHolder --

template<class Class>
bool
Database::DelayQueryBatch(Class* object, void (Class::*method)(QueryResult*, SqlQueryBatch*), SqlQueryBatch* batch)
{
    ASYNC_DELAYHOLDER_BODY(batch)
    return batch->Execute(new Util::QueryCallback<Class, SqlQueryBatch*>(object, method, (QueryResult*)NULL, batch), m_threadBody, m_pResultQueue);
}

template<class Class, typename ParamType1>
bool
Database::DelayQueryBatch(Class* object, void (Class::*method)(QueryResult*, SqlQueryBatch*, ParamType1), SqlQueryBatch* batch, ParamType1 param1)
{
    ASYNC_DELAYHOLDER_BODY(batch)
    return batch->Execute(new Util::QueryCallback<Class, SqlQueryBatch*, ParamType1>(object, method, (QueryResult*)NULL, batch, param1), m_threadBody, m_pResultQueue);
}

template<typename ParamType1>
bool
Database::DelayQueryBatch(void (*method)(QueryResult*, SqlQueryBatch*, ParamType1), SqlQueryBatch* batch, ParamType1 param1 )
{
	ASYNC_DELAYHOLDER_BODY(batch)
	return batch->Execute(new Util::SQueryCallback<SqlQueryBatch*, ParamType1>(method, (QueryResult*)NULL, batch, param1), m_threadBody, m_pResultQueue );
}

#undef ASYNC_QUERY_BODY
#undef ASYNC_PQUERY_BODY
#undef ASYNC_DELAYHOLDER_BODY

#endif
