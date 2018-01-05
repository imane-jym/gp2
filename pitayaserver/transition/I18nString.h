#ifndef __I18N_STRING_H__
#define __I18N_STRING_H__

#include <string>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>
#include "ByteBuffer.h"


enum E_TYPE_I18STRING
{
    E_TYPE_I18STRING_TYPE_CONST,
    E_TYPE_I18STRING_TYPE_CONFIG,
    E_TYPE_I18STRING_TYPE_FORMAT,
    E_TYPE_I18STRING_TYPE_TAG,
};
/// 抽象基类
class I18nString
{
public:
    I18nString() {}
	virtual ~I18nString();
public:
	virtual std::string Get( const std::string& lang ) const = 0;
	virtual std::string Default() const = 0;
	virtual const std::string& GetDef( const std::string& lang ){static std::string empty=""; return empty;}
	virtual const std::string& DefaultDef(){static std::string empty=""; return empty;}
	virtual void Serialize(ByteBuffer& byte)const{}
	virtual void Deserialize( ByteBuffer& byte ){}
	virtual bool IsNull()const{return true;}
};

typedef boost::shared_ptr<I18nString> I18nStringPtr;




/// 用于包装固定字符串
/// I18nStringPtr t = I18nConstString::Create("This is constant string");
class I18nConstString : public I18nString
{
public:
    I18nConstString() : I18nString() {}
    virtual ~I18nConstString(){}
	static I18nStringPtr Create( const std::string& content );
public:
	virtual std::string Get( const std::string& lang ) const;
	virtual std::string Default() const;
	virtual void Serialize( ByteBuffer& )const;
	virtual void Deserialize( ByteBuffer& );
	virtual bool IsNull()const;

private:
	I18nConstString( const std::string& content );
	std::string m_content;
};

/// 格式化字符串
/// \see PARSE_I18N_STRING1
class I18nFormatString : public I18nString
{
public:
    I18nFormatString(): I18nString(),m_pFormat(I18nConstString::Create("")){}
    virtual ~I18nFormatString(){}
	static I18nStringPtr Create( I18nStringPtr pFormat, const std::vector<I18nStringPtr>& vArgs );
public:
	virtual std::string Get( const std::string& lang ) const;
	virtual std::string Default() const;
	virtual void Serialize( ByteBuffer& byte ) const;
	virtual void Deserialize( ByteBuffer& byte );
	virtual bool IsNull()const;
private:
	I18nFormatString( I18nStringPtr pFormat, const std::vector<I18nStringPtr>& vArgs );
	I18nStringPtr 				m_pFormat;
	std::vector<I18nStringPtr>	m_vArgs;
};

/// "@@zh@@你好\n下一行 @@en@@Hello\nNext line"
class I18nTagString : public I18nString
{
public:
    I18nTagString(): I18nString(){}
    virtual ~I18nTagString(){}
	static std::string strDefaultLang;
	static I18nStringPtr Create( const std::string& content );
public:
	virtual std::string Get( const std::string& lang ) const;
	virtual std::string Default() const;
	virtual void Serialize( ByteBuffer& byte ) const;
	virtual void Deserialize( ByteBuffer& byte );
	virtual bool IsNull()const;
private:
	I18nTagString( const std::string& content );
	std::map<std::string, std::string>	m_mapContent;
};

typedef I18nStringPtr (*pI18nCreate)(uint8 byType);
class I18nStringFactory
{
public:
    static I18nStringPtr Create( uint8 byType );
    static I18nStringPtr Create( ByteBuffer& pByte );
	static void RegisterFunc(pI18nCreate f1);
	static pI18nCreate m_f1;
};

#endif
