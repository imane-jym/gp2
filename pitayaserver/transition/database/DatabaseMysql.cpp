#include "DatabaseMysql.h"
#include "Util.h"
#include "Threading.h"
#include "Common.h"
#include "QueryResultMysql.h"

#include <mysql/mysqld_error.h>
#include <mysql/errmsg.h>

size_t DatabaseMysql::db_count = 0;

void DatabaseMysql::ThreadStart()
{
    mysql_thread_init();
}

void DatabaseMysql::ThreadEnd()
{
    mysql_thread_end();
}

DatabaseMysql::DatabaseMysql()
{
    // before first connection
    if (db_count++ == 0)
    {
        // Mysql Library Init
        mysql_library_init(-1, NULL, NULL);

        if (!mysql_thread_safe())
        {
        	IME_SQLERROR( "FATAL ERROR: Used MySQL library isn't thread-safe." );
            exit(1);
        }
    }
}

DatabaseMysql::~DatabaseMysql()
{
    // Free Mysql library pointers for last ~DB
    if (--db_count == 0)
        mysql_library_end();
}

SqlConnection* DatabaseMysql::CreateConnection()
{
    return new MySQLConnection(*this);
}

const char* DatabaseMysql::GetDbName()
{
	if ( m_pSyncConn )
	{
		return dynamic_cast<MySQLConnection*>( m_pSyncConn )->mDatabase.c_str();
	}
	return NULL;
}

MySQLConnection::~MySQLConnection()
{
    mysql_close(mMysql);
}

bool MySQLConnection::Initialize(const char* infoString)
{
    Util::Tokens tokens;
    Util::StrSplit( infoString, "'", tokens );

    Util::Tokens::iterator iter;

    iter = tokens.begin();

    if (iter != tokens.end())
        mHost = *iter++;
    if (iter != tokens.end())
        mPortOrSocket = *iter++;
    if (iter != tokens.end())
        mUser = *iter++;
    if (iter != tokens.end())
        mPassword = *iter++;
    if (iter != tokens.end())
        mDatabase = *iter++;

    return _Open();
}

bool MySQLConnection::_Open()
{
	MYSQL* mysqlInit = mysql_init(NULL);
	if (!mysqlInit)
	{
		IME_SQLERROR( "Could not initialize MySQL connection" );
		return false;
	}

	mysql_options(mysqlInit, MYSQL_SET_CHARSET_NAME, "utf8");
	char value = 1;
	mysql_options(mysqlInit, MYSQL_OPT_RECONNECT, (char *)&value);

	int port;
	char const* unix_socket;

	if (mHost == ".")                                        // socket use option (Unix/Linux)
	{
		unsigned int opt = MYSQL_PROTOCOL_SOCKET;
		mysql_options(mysqlInit, MYSQL_OPT_PROTOCOL, (char const*)&opt);
		mHost = "localhost";
		port = 0;
		unix_socket = mPortOrSocket.c_str();
	}
	else                                                    // generic case
	{
		port = atoi(mPortOrSocket.c_str());
		unix_socket = 0;
	}

	mMysql = mysql_real_connect(mysqlInit, mHost.c_str(), mUser.c_str(),
								mPassword.c_str(), mDatabase.c_str(), port, unix_socket, 0);

	if (!mMysql)
	{
		IME_SQLERROR( "Could not connect to MySQL database at %s: %s\n",
				mHost.c_str(), mysql_error(mysqlInit) );
		mysql_close(mysqlInit);
		return false;
	}

	if ( !mReconnecting )
	{
		IME_SQLDRIVER( "MySQL client library: %s", mysql_get_client_info() );
		IME_SQLDRIVER( "MySQL server ver: %s ", mysql_get_server_info(mMysql) );
	}

	IME_SQLDRIVER( "Connected to MySQL database %s@%s:%s/%s", mUser.c_str(), mHost.c_str(), mPortOrSocket.c_str(), mDatabase.c_str() );

	/*----------SET AUTOCOMMIT ON---------*/
	// It seems mysql 5.0.x have enabled this feature
	// by default. In crash case you can lose data!!!
	// So better to turn this off
	// ---
	// This is wrong since mangos use transactions,
	// autocommit is turned of during it.
	// Setting it to on makes atomic updates work
	// ---
	// LEAVE 'AUTOCOMMIT' MODE ALWAYS ENABLED!!!
	// W/O IT EVEN 'SELECT' QUERIES WOULD REQUIRE TO BE WRAPPED INTO 'START TRANSACTION'<>'COMMIT' CLAUSES!!!
	if (!mysql_autocommit(mMysql, 1))
		IME_SQLDRIVER("AUTOCOMMIT SUCCESSFULLY SET TO 1");
	else
		IME_SQLDRIVER("AUTOCOMMIT NOT SET TO 1");
	/*-------------------------------------*/
	return true;
}

bool MySQLConnection::_Query(const char* sql, MYSQL_RES** pResult, MYSQL_FIELD** pFields, uint64* pRowCount, uint32* pFieldCount)
{
	if ( !mMysql && !_Open() )
        return 0;

    if (mysql_query(mMysql, sql))
    {
    	uint32 err = mysql_errno( mMysql );

    	IME_SQLERROR( "Query Error: %s, SQL: %s", mysql_error(mMysql), sql );

    	if ( _HandleMySQLErrno( err ) ) // If it returns true, an error was handled successfully (i.e. reconnection)
    		return _Query( sql, pResult, pFields, pRowCount, pFieldCount ); // Try again

        return false;
    }

    *pResult = mysql_store_result(mMysql);
    *pRowCount = mysql_affected_rows(mMysql);
    *pFieldCount = mysql_field_count(mMysql);

    if (!*pResult)
        return false;

    if (!*pRowCount)
    {
        mysql_free_result(*pResult);
        return false;
    }

    *pFields = mysql_fetch_fields(*pResult);
    return true;
}

QueryResult* MySQLConnection::Query(const char* sql)
{
    MYSQL_RES* result = NULL;
    MYSQL_FIELD* fields = NULL;
    uint64 rowCount = 0;
    uint32 fieldCount = 0;

    if (!_Query(sql, &result, &fields, &rowCount, &fieldCount))
        return NULL;

    QueryResultMysql* queryResult = new QueryResultMysql(result, fields, rowCount, fieldCount);

    queryResult->NextRow();
    return queryResult;
}

bool MySQLConnection::Execute(const char* sql)
{
    if ( !mMysql && !_Open() )
        return false;

	if ( mysql_query(mMysql, sql) )
	{
		uint32 err = mysql_errno( mMysql );

		IME_SQLERROR( "Query Error: %s, SQL: %s", mysql_error(mMysql), sql );

		if ( _HandleMySQLErrno( err ) ) // If it returns true, an error was handled successfully (i.e. reconnection)
			return Execute( sql ); // Try again

		return false;
	}

	// Unexpected result when using SqlTransaction
//	if (mysql_affected_rows(mMysql) == 0)
//	{
//		return false;
//	}

    return true;
}

// TODO
bool MySQLConnection::BeginTransaction()
{
    return Execute("START TRANSACTION");
//	return true;
}

bool MySQLConnection::CommitTransaction()
{
    return Execute("COMMIT");
//	return true;
}

bool MySQLConnection::RollbackTransaction()
{
    return Execute("ROLLBACK");
//	return true;
}

void MySQLConnection::Ping()
{
	if (!mMysql) return;
	mysql_ping(mMysql);
}

unsigned long MySQLConnection::escape_string(char* to, const char* from, unsigned long length)
{
    if (!mMysql || !to || !from || !length)
        return 0;

    return(mysql_real_escape_string(mMysql, to, from, length));
}

void MySQLConnection::escape_string(std::string& str)
{
    if(str.empty())
        return;
    char* buf = new char[str.size()*2+1];
    escape_string(buf,str.c_str(),str.size());
    str = buf;
    delete[] buf;
}

bool MySQLConnection::_HandleMySQLErrno(uint32 dwError)
{
    switch (dwError)
    {
        case CR_SERVER_GONE_ERROR:
        case CR_SERVER_LOST:
        case CR_INVALID_CONN_HANDLE:
        case CR_SERVER_LOST_EXTENDED:
        {
            mReconnecting = true;
            uint64 oldThreadId = mysql_thread_id(mMysql);
            mysql_close(mMysql);

            if ( this->_Open() )
            {
            	IME_SQLDRIVER( "Connection to the MySQL server is active." );
                if ( oldThreadId != mysql_thread_id(mMysql) )
                {
                	IME_SQLDRIVER( "Successfully reconnected to MySQL database %s@%s:%s/%s", mUser.c_str(), mHost.c_str(), mPortOrSocket.c_str(), mDatabase.c_str() );
                }
                mReconnecting = false;
                return true;
            }

            uint32 lErrno = mysql_errno(mMysql);   		// It's possible this attempted reconnect throws CR_SERVER_GONE_ERROR at us. To prevent crazy recursive calls, sleep here.
            ACE_OS::sleep(3);                           // Sleep 3 seconds
            return _HandleMySQLErrno(lErrno);           // Call self (recursive)
        }

        case ER_LOCK_DEADLOCK:
            return false;
        // Query related errors - skip query
        case ER_WRONG_VALUE_COUNT:
        case ER_DUP_ENTRY:
            return false;

        // Outdated table or database structure - terminate core
        case ER_BAD_FIELD_ERROR:
        case ER_NO_SUCH_TABLE:
        	IME_SQLERROR( "Database structure is not up to date." );
            ACE_OS::sleep(10);
            std::abort();
            return false;
        case ER_PARSE_ERROR:
        	IME_SQLERROR( "Error while parsing SQL." );
            ACE_OS::sleep(10);
            std::abort();
            return false;
        default:
        	IME_SQLERROR( "Unhandled MySQL errno %u. Unexpected behaviour possible.", dwError );
            return false;
    }
}
