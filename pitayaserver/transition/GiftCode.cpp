#include "GiftCode.h"
#include "Util.h"
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include "Log.h"

namespace Util
{

	static const char digit[] =
	{
			'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A',
			'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
			'L', 'M', 'N', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
			'W', 'X', 'Y', 'Z'
	};
	static const int size = 34;

	static std::string NumToCode( uint64 num, uint32 shift )
	{
		std::string tmp;

		int t = 0;
		while ( num )
		{
			tmp.append( 1, digit[(num + shift) % size] );
			num /= size;
			if ( ++t % 4 == 0 ) tmp.append( "-" );
		}

		return std::string( tmp.rbegin(), tmp.rend() );
	}

	std::string CalcGiftCode( uint32 dwId, uint32 dwIdx, uint32 dwParam1, uint32 dwParam2, uint32 dwParam3 )
	{
		if ( dwId >= 26 * 26 * 100 /** 2 digits (26 base) + 2 digits (10 base) */ || dwIdx > 1336335 /** 4 digits (34 base) */ ) return "Error";

		uint64 sum = 0;

		sum += dwIdx;
		sum = sum * 1003 + dwId;
		sum = sum * ( dwParam1 % 109 + 1 );

		uint64 t = dwParam2 % 100003 + 99881;
		for ( int i = 0; i < 3; i++ ) t = (t << (8 - i)) | t;

		sum ^= t;

		std::string str;
		{
			uint32 v = ( ( 'H' - 'A' ) * 26 + ( 'D' - 'A' ) + ( dwId / 100 ) ) % ( 26 * 26 );
			str.append( 1, ( v / 26 ) + 'A' );
			str.append( 1, ( v % 26 ) + 'A' );
		}

	//	str.append( "YX" );

		char buf[16];
		sprintf( buf, "%02u-", dwId % 100 );
		str.append( buf );

		std::string strIdx = NumToCode( dwIdx, 0 );
		while ( strIdx.length() < 4 ) strIdx.insert( 0, "0" );
		str.append( strIdx );

		str.append( NumToCode( sum, (dwIdx ^ dwParam3) % 34 + 1 ) );

		return str;
	}

	bool FilterGiftCodeFormat( std::string& strCode, uint32& odwId, uint32& odwIdx )
	{
		size_t pos;
		if ( strCode.substr( 0, 2 ) != "HD" &&
				strCode.substr( 0, 2 ) != "hd" &&
				strCode.substr( 0, 2 ) != "hD" &&
				strCode.substr( 0, 2 ) != "Hd" )
		{
		    strCode = std::string("HD") + strCode;
		}
		while ( ( pos = strCode.find( "-" ) ) != std::string::npos )
		{
			strCode.replace( pos, 1, "" );
		}

		if ( strCode.length() != 16 ) return false;

		bool bFlag = false;

		for ( int i = 0; i < 16; i++ )
		{
			if ( '1' <= strCode[i] && strCode [i] <= '9' )
			{

			}
			else if ( 'a' <= strCode[i] && strCode[i] <= 'z' )
			{
				strCode[i] = strCode[i] - 'a' + 'A';
			}
			else if ( 'A' <= strCode[i] && strCode[i] <= 'Z' )
			{

			}
			else if ( strCode[i] == '0' )
			{
				if ( bFlag ) return false;
			}
			else
			{
				return false;
			}

			if ( i >= 4 && strCode[i] != '0' ) bFlag = true;
			if ( strCode[i] == 'O' ) return false;
		}

		for ( int i = 0; i < 2; i++ )
		{
			if ( strCode[i] < 'A' || strCode[i] > 'Z' ) return false;
		}

		{

			uint32 v1 = ( strCode[0] - 'A' ) * 26 + ( strCode[1] - 'A' );
			uint32 v2 = ( 'H' - 'A' ) * 26 + ( 'D' - 'A' );

			uint32 v = ( v1 >= v2 ? v1 - v2 : v1 + 26 * 26 - v2 );
			odwId = v * 100;
		}
	//	if ( strCode[0] != 'Y' || strCode[1] != 'X' ) return false;
		if ( strCode[2] < '0' || strCode[2] > '9' ) return false;
		if ( strCode[3] < '0' || strCode[3] > '9' ) return false;

		odwId += (strCode[2] - '0') * 10 + (strCode[3] - '0');
		odwIdx = 0;

		for ( int i = 4; i < 8; i++ )
		{
			if ( strCode[i] == '0' ) continue;

			odwIdx *= 34;

			if ( '1' <= strCode[i] && strCode[i] <= '9' )
			{
				odwIdx += ( strCode[i] - '1' );
			}
			else if ( 'A' <= strCode[i] && strCode[i] <= 'N' )
			{
				odwIdx += ( strCode[i] - 'A' + 9 );
			}
			else
			{
				odwIdx += ( strCode[i] - 'A' + 8 );
			}
		}

		return true;
	}
}



