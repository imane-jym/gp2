
#ifndef _DEFINES_H
#define _DEFINES_H
#include <stdint.h>

typedef int64_t      int64;
typedef uint64_t 	uint64;


typedef int32_t      int32;
typedef int16_t      int16;
typedef int8_t       int8;
typedef uint32_t     uint32;
typedef uint16_t     uint16;
typedef uint8_t      uint8;

enum
{
	MAIN_THREAD,                                /* main */
	WORLD_THREAD                                /* logical */
};

enum
{
	TIMER_1S = 1,
	TIMER_3S = 3,
	TIMER_5S = 5,
	TIMER_15S = 15,
	TIMER_30S = 30,
	TIMER_1MIN = 60,
	TIMER_3MIN = 180,
	TIMER_5MIN = 300,
	TIMER_10MIN = 600,
	TIMER_30MIN = 1800,
};

#define MACRO_PROPERTY_DEF( T, varName, funName )					\
	protected: T varName;											\
	public: virtual T Get##funName() const {return varName;}		\
	public: virtual void Set##funName( T var ) {varName = var;}

#define MACRO_PROPERTY_READONLT( T, varName, funName )				\
	protected: T varName;											\
	public: virtual T Get##funName() const {return varName;}

#define MACRO_PROPERTY_REF_DEF( T, varName, funName )				\
	protected: T varName;											\
	public: virtual T& Get##funName() {return varName;}	\
	public: virtual void Set##funName( const T& var ) { varName = var; }

#define MACRO_PROPERTY_REF_CONST_DEF( T, varName, funName )			\
	protected: T varName;											\
	public: virtual const T& Get##funName() {return varName;}		\
	public: virtual void Set##funName( const T& var ) { varName = var; }

/* 结构体内map属性定义专用
 * c-style
 * 修改时请勿将name改为##name方式，因为不宜批量改名
 */
#define MACRO_MAP_PROPERTY_DEF(keyT, valT, name) 					\
	typedef std::map<keyT, valT> Map##name; 						\
	Map##name name; 												\
	valT* get_##name(const keyT& key) 								\
	{ 																\
		Map##name::iterator itMap = name.find(key); 				\
		if (itMap == name.end()) return NULL; 						\
		return &(itMap->second); 									\
	}

#define ROLE_PROPERTY( T, Name )		\
	protected: T m_##Name;				\
	public: inline T& Get##Name() { return m_##Name; }

#define ROOM_PROPERTY( T, Name )		\
	protected: T m_##Name;				\
	public: inline T& Get##Name() { return m_##Name; }

#define SAFE_DELETE( _p )		\
	do\
	{\
		if ( _p )\
		{\
			delete _p;\
			_p = NULL;\
		}\
	} while (0);

#define MK( a, b ) std::make_pair( a, b )
#define NO_ZERO( v ) ( abs( v ) < 1e-6f ? ( v >= 0 ? 1e-6f : -1e-6f ) : v )

// iterator loop
#define FOR_EACH_FROWARD(it, container) \
    for( typeof((container).begin()) it = (container).begin(); it != (container).end(); ++it )

#define FOR_EACH_REVERSE(it, container) \
    for( typeof((container).rbegin()) it = (container).rbegin(); it != (container).rend(); ++it )


#endif
