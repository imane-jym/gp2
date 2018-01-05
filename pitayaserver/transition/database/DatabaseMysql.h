#ifndef _DATABASEMYSQL_H
#define _DATABASEMYSQL_H

//#include "Common.h"
#include "Database.h"
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"

#include <mysql/mysql.h>

class DatabaseMysql;

class MySQLConnection : public SqlConnection
{
	friend class DatabaseMysql;
    public:
        MySQLConnection(Database& db) : SqlConnection(db), mMysql(NULL), mReconnecting(false) {}
        virtual ~MySQLConnection();

        //! Initializes Mysql and connects to a server.
        /*! infoString should be formated like hostname'port'username'password'database. */
        bool Initialize(const char* infoString);

        QueryResult* Query(const char* sql);
        bool Execute(const char* sql);

        unsigned long escape_string(char* to, const char* from, unsigned long length);
        void escape_string(std::string& str);

        bool BeginTransaction();
        bool CommitTransaction();
        bool RollbackTransaction();

        void Ping();

    private:
        bool _Open();
        bool _Query(const char* sql, MYSQL_RES** pResult, MYSQL_FIELD** pFields, uint64* pRowCount, uint32* pFieldCount);
        bool _HandleMySQLErrno(uint32 dwError);

        MYSQL* 			mMysql;
        bool            mReconnecting;  //! Are we reconnecting?

        // connection info
        std::string 	mHost;
        std::string		mPortOrSocket;
        std::string		mUser;
        std::string		mPassword;
        std::string		mDatabase;
};

class DatabaseMysql : public Database
{
    public:
        DatabaseMysql();
        virtual ~DatabaseMysql();

        // must be call before first query in thread
        void ThreadStart();
        // must be call before finish thread run
        void ThreadEnd();

        const char* GetDbName();

    protected:
        virtual SqlConnection* CreateConnection();

    private:
        static size_t db_count;
};

#endif
