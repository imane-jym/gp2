#ifndef __GIFT_CODE_H__
#define __GIFT_CODE_H__

#include <vector>
#include "Defines.h"
#include <string>

namespace Util
{
	std::string CalcGiftCode( uint32 dwId, uint32 dwIdx, uint32 dwParam1, uint32 dwParam2, uint32 dwParam3 );
	bool FilterGiftCodeFormat( std::string& strCode, uint32& odwId, uint32& odwIdx );
}


#endif
