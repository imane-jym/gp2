#include "I18nString.h"
#include "Defines.h"
#include <boost/regex.hpp>
#include "Log.h"
#include "Util.h"
std::string I18nTagString::strDefaultLang = "en";

I18nString::~I18nString()
{
}

/* I18nConstString */


I18nConstString::I18nConstString( const std::string& content ):m_content( content )
{

}

I18nStringPtr I18nConstString::Create( const std::string& content )
{
	return I18nStringPtr( new I18nConstString( content ) );
}

std::string I18nConstString::Get( const std::string& lang ) const
{
	return m_content;
}

std::string I18nConstString::Default() const
{
	return m_content;
}
void I18nConstString::Serialize(ByteBuffer& byte) const
{
    byte << (uint8)E_TYPE_I18STRING_TYPE_CONST << m_content;
}
void I18nConstString::Deserialize( ByteBuffer& byte )
{
    byte >> m_content;
}
bool I18nConstString::IsNull()const
{
    return m_content == "";
}

/* I18nFormatString */

I18nFormatString::I18nFormatString( I18nStringPtr pFormat, const std::vector<I18nStringPtr>& vArgs ):m_pFormat(pFormat), m_vArgs(vArgs)
{
}

I18nStringPtr I18nFormatString::Create( I18nStringPtr pFormat, const std::vector<I18nStringPtr>& vArgs )
{
	return I18nStringPtr( new I18nFormatString( pFormat, vArgs ) );
}

std::string I18nFormatString::Get( const std::string& lang ) const
{
	std::string format = m_pFormat->Get( lang );
	std::vector<std::string> args;
	for ( std::vector<I18nStringPtr>::const_iterator it = m_vArgs.begin();
			it != m_vArgs.end(); ++it )
	{
		args.push_back( (*it)->Get( lang ) );
	}

	CUtil::StrReplace( format, args );
	return format;
}

std::string I18nFormatString::Default() const
{
	std::string format = m_pFormat->Default();
	std::vector<std::string> args;
	for ( std::vector<I18nStringPtr>::const_iterator it = m_vArgs.begin();
			it != m_vArgs.end(); ++it )
	{
		args.push_back( (*it)->Default() );
	}

	CUtil::StrReplace( format, args );
	return format;
}



bool I18nFormatString::IsNull()const
{
    return ( !m_pFormat ) ;
}
void I18nFormatString::Serialize( ByteBuffer& byte ) const
{
    byte << (uint8)E_TYPE_I18STRING_TYPE_FORMAT;
    m_pFormat->Serialize( byte );
    byte << (uint32)m_vArgs.size();
    for ( std::vector<I18nStringPtr>::const_iterator it = m_vArgs.begin(); it != m_vArgs.end(); ++it )
    {
        if ( (*it) )
        {
            (*it)->Serialize( byte );
        }
    }
}
void I18nFormatString::Deserialize( ByteBuffer& byte )
{
    m_pFormat = I18nStringFactory::Create( byte );
    uint32 dwSize;

    byte >> dwSize;
    for (unsigned int i = 0; i < dwSize; ++i )
    {
        I18nStringPtr pStr = I18nStringFactory::Create( byte );
//        if ( !( pStr.get() ) )
//        {
//            IME_ERROR("Create I18nString Error");
//            return;
//        }
        m_vArgs.push_back( pStr );
    }
}
/* I18nHashString */

I18nTagString::I18nTagString( const std::string& content )
{
	boost::cmatch mat;
	boost::regex reg( "@@(\\w+)@@\\s*" );
	boost::cregex_iterator itEnd;

	int p = -1;
	std::string lang = "";

	for( boost::cregex_iterator it = boost::make_regex_iterator( content.c_str() , reg );
			it != itEnd; ++it )
	{
		int st = (*it)[0].first 	- content.c_str();
		int ed = (*it)[0].second 	- content.c_str();

		if ( p >= 0 ) m_mapContent.insert( MK( lang, content.substr( p, st - p ) ) );

		lang 	= (*it)[1].str();
		p 		= ed;
	}
	if ( p >= 0 )
	{
		m_mapContent.insert( MK( lang, content.substr( p ) ) );
	}
	else
	{
		// not found tag ##language##
		m_mapContent.insert( MK( "", content ) );
	}
}

I18nStringPtr I18nTagString::Create( const std::string& content )
{
	return I18nStringPtr( new I18nTagString( content ) );
}

std::string I18nTagString::Get( const std::string& lang ) const
{
	std::map<std::string, std::string>::const_iterator it = m_mapContent.find( lang );

	if ( it == m_mapContent.end() )
	{
		IME_SYSTEM_ERROR( "I18n", "Language Not Found In I18nHashString, lang=%s,content=%s", lang.c_str(),Default().c_str() );
		return Default();
	}
	return it->second;
}

std::string I18nTagString::Default() const
{
	std::map<std::string, std::string>::const_iterator it = m_mapContent.find( strDefaultLang );
	if ( it != m_mapContent.end() ) return it->second;
	else if ( !m_mapContent.empty() ) return m_mapContent.begin()->second;
	return "";
}

void I18nTagString::Serialize( ByteBuffer& byte ) const
{
    byte << (uint8)E_TYPE_I18STRING_TYPE_TAG <<strDefaultLang << m_mapContent;
}
void I18nTagString::Deserialize( ByteBuffer& byte )
{
    byte >> strDefaultLang >> m_mapContent;
}
bool I18nTagString::IsNull()const
{
    return  m_mapContent.size() == 0;
}
pI18nCreate I18nStringFactory::m_f1 = NULL;
void I18nStringFactory::RegisterFunc(pI18nCreate f1)
{
	m_f1 = f1;
}

I18nStringPtr I18nStringFactory::Create( ByteBuffer& Byte )
{
    I18nStringPtr ptr;
	if (m_f1 == NULL)
	{
		IME_SYSTEM_ERROR("I18nString", "you have not registered I18nStringFactory create function");
		return ptr;
	}
    uint8 byType;
    Byte >> byType;
    ptr = m_f1(byType);
    if ( ptr.get() )
    {
    	ptr->Deserialize( Byte );
    }
    return ptr;
}
