#ifndef __CONFIG_BASE_H__
#define __CONFIG_BASE_H__

/**
 * CSV Configure
 */

#include "CsvReader.h"
#include <string>
#include <map>

#define CONFIG_DECLARATION_BEGIN( CLASS_NAME )	\
	class CLASS_NAME : public ConfigBase		\
	{											\
		public:									\

#define CONFIG_DECLARATION_END( CLASS_NAME ) 				\
	static const char* GetName() { return #CLASS_NAME; }	\
	};

#define CONFIG_GET_FUNC( retType, keyType )										\
public: typedef retType	STC;													\
public: typedef std::map< keyType, retType > ConfigMap;							\
protected: ConfigMap m_mapConfig;												\
public: inline ConfigMap& GetMap()												\
{																				\
    return m_mapConfig;															\
}    																			\
public: inline retType* GetById( keyType id )									\
{																				\
    ConfigMap::iterator it = m_mapConfig.find( id );							\
    if ( it == m_mapConfig.end() )												\
    {																			\
        IME_ERROR( "CONFIG ERROR - %s - ID(%d) not found", #retType, id );		\
    }																			\
    return it == m_mapConfig.end() ? NULL : &(it->second);						\
}                                                                               \
virtual void ClearData()                                                        \
{                                                                               \
    m_mapConfig.clear();                                                        \
}

#define CONFIG_GET_FUNC_CLEAR( retType, keyType ,cleardata)										\
public: typedef retType	STC;													\
public: typedef std::map< keyType, retType > ConfigMap;							\
protected: ConfigMap m_mapConfig;												\
public: inline ConfigMap& GetMap()												\
{																				\
    return m_mapConfig;															\
}    																			\
public: inline retType* GetById( keyType id )									\
{																				\
    ConfigMap::iterator it = m_mapConfig.find( id );							\
    if ( it == m_mapConfig.end() )												\
    {																			\
        IME_ERROR( "CONFIG ERROR - %s - ID(%d) not found", #retType, id );		\
    }																			\
    return it == m_mapConfig.end() ? NULL : &(it->second);						\
}                                                                               \
virtual void ClearData()                                                        \
{                                                                               \
    m_mapConfig.clear();                                                        \
    cleardata.clear();															\
}
#define CONFIG_GET_FUNC_KEY_STRING( retType, keyType )										\
public: typedef retType	STC;													\
public: typedef std::map< keyType, retType > ConfigMap;							\
protected: ConfigMap m_mapConfig;												\
public: inline ConfigMap& GetMap()												\
{																				\
    return m_mapConfig;															\
}    																			\
public: inline retType* GetById( keyType id )									\
{																				\
    ConfigMap::iterator it = m_mapConfig.find( id );							\
    if ( it == m_mapConfig.end() )												\
    {																			\
        IME_ERROR( "CONFIG ERROR - %s - ID(%s) not found", #retType, id.c_str() );		\
    }																			\
    return it == m_mapConfig.end() ? NULL : &(it->second);						\
}                                                                               \
virtual void ClearData()                                                        \
{                                                                               \
    m_mapConfig.clear();                                                        \
}


// read the cell and assign to an Integer, if this cell is empty, assign 0
#define GET_INT_DATA( _to, _header_id )			\
	do	\
	{	\
		int _idx = getColumnIdxByHeaderId( _header_id );	\
		if ( _idx < 0 )	\
		{ \
			IME_ERROR( "Column Header Not Found, id=%u", _header_id );	\
			return false;	\
		} \
		if ( row->size() <= _idx ) \
		{ \
			IME_ERROR( "Column Idx Exceed Size, id=%u", _header_id );	\
			return false; \
		} \
		const char* v = (*row)[ _idx ].c_str();	\
		if ( strlen( v ) == 0 )	\
		{\
			_to = 0;	\
		}\
		else	\
		{\
			_to = atoi(v);	\
		}\
	} while(0);

#define GET_FLOAT_DATA( _to, _header_id )			\
	do	\
	{	\
		int _idx = getColumnIdxByHeaderId( _header_id );	\
		if ( _idx < 0 )	\
		{ \
			IME_ERROR( "Column Header Not Found, id=%u", _header_id );	\
			return false;	\
		} \
		if ( row->size() <= _idx ) \
		{ \
			IME_ERROR( "Column Idx Exceed Size, id=%u", _header_id );	\
			return false; \
		} \
		const char* v = (*row)[ _idx ].c_str();	\
		if ( strlen( v ) == 0 )	\
		{\
			_to = 0;	\
		}\
		else	\
		{\
			_to = atof(v);	\
		}\
	} while(0);

#define GET_LONG_INT_DATA( _to, _header_id )		\
	do	\
	{	\
		int _idx = getColumnIdxByHeaderId( _header_id );	\
		if ( _idx < 0 )	\
		{ \
			IME_ERROR( "Column Header Not Found, id=%u", _header_id );	\
			return false;	\
		} \
		if ( row->size() <= _idx ) \
		{ \
			IME_ERROR( "Column Idx Exceed Size, id=%u", _header_id );	\
			return false; \
		} \
		const char* v = (*row)[ _idx ].c_str();	\
		if ( strlen( v ) == 0 )	\
		{\
			_to = 0;	\
		}\
		else	\
		{\
			_to = atoll(v);	\
		}\
	} while(0);

#define INSERT_AND_CHECK( V, F )	\
	if (  !m_mapConfig.insert( std::make_pair( V.F, V ) ).second )	\
	{\
		IME_ERROR( "Duplicate ID, id=%u", V.F );\
	}

#define GET_STRING_DATA( _to, _header_id )	\
	do	\
	{	\
		int _idx = getColumnIdxByHeaderId( _header_id );	\
		if ( _idx < 0 )	\
		{ \
			IME_ERROR( "Column Header Not Found, id=%u", _header_id );	\
			return false;	\
		} \
		if ( row->size() <= _idx ) \
		{ \
			IME_ERROR( "Column Idx Exceed Size, id=%u", _header_id );	\
			return false; \
		} \
		_to = (*row)[ _idx ]; \
	} while(0);

#define CONVERT_STRING_TO_VECAWARD( _str, _vec,_firstSplit,_secondSplit)\
	do\
	{\
		std::vector<std::string> vecSplit;\
		Util::StrSplit(_str,_firstSplit,vecSplit);\
		for(std::vector<std::string>::iterator iter = vecSplit.begin(); iter != vecSplit.end();iter++)\
		{\
			std::vector<std::string> vecTemp;\
			Util::StrSplit(*iter,_secondSplit,vecTemp);\
			if(vecTemp.size() == 0) continue;\
			if(vecTemp.size() != 3)\
			{\
				IME_ERROR( "Convert To VecAward Failed, str=%s", _str.c_str() );	\
				return false;\
			}\
			STC_AWARD stcAward;\
			stcAward.byType = atoi(vecTemp[0].c_str());\
			stcAward.dwId = atoi(vecTemp[1].c_str());\
			stcAward.dwQty = atoi(vecTemp[2].c_str());\
			_vec.push_back(stcAward);\
		}\
	}while(0);

class CCsvReader;

typedef std::vector<std::string> CSV_ROW_TYPE;

class ConfigBase
{
public:
	ConfigBase();
	ConfigBase(const ConfigBase& rhs);
	ConfigBase& operator=(const ConfigBase& rhs);

	virtual ~ConfigBase();

	bool Load( const char* filePath );
	bool LoadRawData( const char* data );
	bool Reload();

	virtual bool ParseData( const CSV_ROW_TYPE* row );
    virtual void ClearData();

    void SetFilePath( const char* filePath );
	int getColumnIdxByHeaderId( int id );

protected:
	bool 		m_bInited;
	std::string m_pFilePath;

	bool 		m_bUseRawData;
	std::map<std::string, int> m_header;

private:
	void Init( const ConfigBase& rhs );
};


#endif
