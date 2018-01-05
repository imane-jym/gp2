#ifndef __QUERY_RESULT_MYSQL_H__
#define __QUERY_RESULT_MYSQL_H__

#include "QueryResult.h"
#include "Common.h"
#include <mysql/mysql.h>

class QueryResultMysql : public QueryResult
{
    public:
        QueryResultMysql(MYSQL_RES* result, MYSQL_FIELD* fields, uint64 rowCount, uint32 fieldCount);
        ~QueryResultMysql();

        bool NextRow();

    private:
        enum Field::DataTypes ConvertNativeType(enum_field_types mysqlType) const;
        void EndQuery();

        MYSQL_RES* mResult;
};
#endif
