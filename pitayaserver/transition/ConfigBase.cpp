#include "ConfigBase.h"
#include "Log.h"
#include "Util.h"

ConfigBase::ConfigBase()
:m_bInited(false), m_bUseRawData( false )
{

}

ConfigBase::ConfigBase(const ConfigBase& rhs)
:m_bInited(false), m_bUseRawData( false )
{
	Init( rhs );
}

ConfigBase& ConfigBase::operator=(const ConfigBase& rhs)
{
	Init( rhs );
	return *this;
}

ConfigBase::~ConfigBase()
{
}

bool ConfigBase::Load( const char* filePath )
{
	if ( m_bInited ) return true;

//	if ( !m_bUseRawData )
//	{
//		m_pFilePath = std::string( filePath );
//		m_pReader = new CsvReader( m_pFilePath.c_str() );
//
//		if ( !m_pReader->load( 0, true ) ) return false;
//	}
//	else
//	{
//		m_pReader = new CsvReader( "" );
//		if ( !m_pReader->loadWithData( m_pRawData, 0, true ) ) return false;
//	}
	CCsvReader Reader;

	if (!Reader.Init(m_pFilePath.c_str()))
	{
		IME_ERROR("config err %s", Reader.GetErrorStr().c_str());
		return false;
	}

	ClearData();

//	int wRow = m_pReader->rowCount();
//	for ( int i = 0; i < wRow; i++ )
//	{
//		const CsvReader::CSV_ROW_TYPE* row = m_pReader->getRow(i);
//
//		if ( !ParseData( row ) ) return false;
//	}
	int wRow = Reader.GetLineCount();
	for (int i = 0; i < wRow; i++)
	{
		if (i == 0)
		{
			for (int j = 0; j < Reader.GetRowCount(); j++)
			{
				m_header[Reader.GetCell(i, j)] = i;
			}
		}
		else
		{
			std::vector<std::string>* pRow = Reader.GetLine(i);
			if (pRow == NULL || ParseData(pRow)) return false;
		}
	}
	m_bInited = true;
	return true;
}

bool ConfigBase::LoadRawData( const char* data )
{
	CCsvReader Reader;

	if (!Reader.InitByFile(data))
	{
		IME_ERROR("config err %s", Reader.GetErrorStr().c_str());
		return false;
	}

	ClearData();

//	int wRow = m_pReader->rowCount();
//	for ( int i = 0; i < wRow; i++ )
//	{
//		const CsvReader::CSV_ROW_TYPE* row = m_pReader->getRow(i);
//
//		if ( !ParseData( row ) ) return false;
//	}
	int wRow = Reader.GetLineCount();
	for (int i = 0; i < wRow; i++)
	{
		if (i == 0)
		{
			for (int j = 0; j < Reader.GetRowCount(); j++)
			{
				m_header[Reader.GetCell(i, j)] = i;
			}
		}
		else
		{
			std::vector<std::string>* pRow = Reader.GetLine(i);
			if (pRow == NULL || ParseData(pRow)) return false;
		}
	}
	m_bInited = true;
	m_bUseRawData = true;
	return true;
}

bool ConfigBase::Reload()
{
	// unsafe
	if ( m_bUseRawData )
	{
		return false;
	}

	if ( m_bInited )
	{
		m_bInited = false;
	}

	return Load( m_pFilePath.c_str() );
}

bool ConfigBase::ParseData( const CSV_ROW_TYPE* row )
{
	return true;
}

void ConfigBase::SetFilePath( const char* filePath )
{
	m_pFilePath = filePath;
}

void ConfigBase::ClearData()
{

}

void ConfigBase::Init( const ConfigBase& rhs )
{
	m_bInited	= rhs.m_bInited;
	m_pFilePath	= rhs.m_pFilePath;

	m_bUseRawData = rhs.m_bUseRawData;
	m_header = rhs.m_header;
}

int ConfigBase::getColumnIdxByHeaderId( int id )
{
	std::map<std::string, int>::iterator it = m_header.find(CUtil::Uint32ToString((uint32_t)id));
	if (it != m_header.end())
	{
		return it->second;
	}
	return -1;
}
