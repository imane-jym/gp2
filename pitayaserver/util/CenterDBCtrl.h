#ifndef __CENTER_DB_CTRL__
#define __CENTER_DB_CTRL__

#include "DatabaseMysql.h"
#include "GmCommand.h"
#include <map>
#include "ByteBuffer.h"

#define GET_ARRAY_LENGTH(array) 	sizeof(array)/sizeof(array[0])

namespace CenterDB
{

enum ServerLoginEnable
{
	E_SERVER_LOGIN_ENABLE_FALSE	= 0,
	E_SERVER_LOGIN_ENABLE_TRUE	= 1,
};

enum ServerStatus
{
	E_SERVER_STATUS_NORMAL		= 0,
	E_SERVER_STATUS_NEW			= 1,
	E_SERVER_STATUS_HOT			= 2,
	E_SERVER_STATUS_MAINTAIN	= 3,
	E_SERVER_STATUS_GM_ONLY		= 4,

	E_SERVER_STATUS_COUNT
};

enum RoleStatus
{
	E_ROLE_STATUS_NORMAL		= 0,
	E_ROLE_STATUS_FORBID		= 1,
	E_ROLE_STATUS_BAN_SPEAK		= 2,

	E_ROLE_STATUS_CNT			= 4,
};

enum LoginAuthType
{
	E_LOGIN_AUTH_TYPE_ACCOUNT		= 0,	// 注册帐号登录
	E_LOGIN_AUTH_TYPE_PLATFORM		= 1,	// 第三方平台登录
	E_LOGIN_AUTH_TYPE_FAST			= 2,	// 快速登录
};
enum CDKeyState
{
    E_CDK_STATE_NO_NEED_VERIFY      = 0,    //  不需要验证
    E_CDK_STATE_VERIFIED            = 1,    //  已经验证
    E_CDK_STATE_NOT_VERIFIED        = 2,    //  未验证
};
enum LoginResultType
{
	E_LOGIN_RESULT_TYPE_OK			= 0,
	E_LOGIN_RESULT_TYPE_NOT_FOUND	= 1,
	E_LOGIN_RESULT_TYPE_WRONG_PWD	= 2,
	E_LOGIN_RESULT_ERROR			= 3,
	E_LOGIN_RESULT_VERIFY_ERROR		= 4,	//	校验码验证失败
};

enum ChargeState
{
	E_CHARGE_STATE_UNPAY			= 0,
	E_CHARGE_STATE_PAYED			= 1,
	E_CHARGE_STATE_DISTRIBUTED		= 2,
};

enum ChargeType
{
	E_CHARGE_TYPE_NORMAL			= 0,
	E_CHARGE_TYPE_OMIT				= 2,
};

enum GoodsState
{
	E_GOODS_STATE_OFF_SALE			= 0,
	E_GOODS_STATE_ON_SALE			= 1,
	E_GOODS_STATE_HOT				= 2,
	E_GOODS_STATE_NEW				= 3,
	E_GOODS_STATE_DISCOUNT			= 4,
};

enum LoginStrategyType
{
	E_LOGIN_STRATEGY_TYPE_PLATFORM	= 1,
	E_LOGIN_STRATEGY_TYPE_IP		= 2,
	E_LOGIN_STRATEGY_TYPE_AUTH		= 3,
	E_LOGIN_STRATEGY_TYPE_VERSION	= 4,
	E_LOGIN_STRATEGY_TYPE_DEVICE	= 5,
	E_LOGIN_STRATEGY_TYPE_REG_TIME	= 6,
//	E_LOGIN_STRATEGY_TYPE_JS_SVN_VER 	= 7,
	E_LOGIN_STRATEGY_TYPE_RES_SVN_VER	= 8,
};

enum LoginBindResultType
{
	E_LOGIN_BIND_RESULT_TYPE_SUCC			= 0,
	E_LOGIN_BIND_RESULT_TYPE_EMPTY_PASSPORT	= 1,
	E_LOGIN_BIND_RESULT_TYPE_INVALID_PWD_OR_MAIL = 2,
	E_LOGIN_BIND_RESULT_TYPE_PASSPORT_NOT_EXIST	 = 3,
	E_LOGIN_BIND_RESULT_TYPE_DUPLICATE_PASSPORT	 = 4,
	E_LOGIN_BIND_RESULT_TYPE_INVALID_AUTH_TYPE	 = 5,
};

enum NoticeUseType
{
	E_NOTICE_USE_TYPE_LOGIN				= 1,
	E_NOTICE_USE_TYPE_GAME				= 2,
	E_NOTICE_USE_TYPE_CUSTOM_SERVICE	= 3,
	E_NOTICE_USE_TYPE_WEIBO				= 4,
	E_NOTICE_USE_TYPE_UPDATE_ADDR		= 5,
	E_NOTICE_USE_TYPE_SO_FILE_MD5		= 23,	//so文件校验码
};

enum NoticeConditionType
{
	E_NOTICE_CONDITION_TYPE_DEFAULT		= 0,
	E_NOTICE_CONDITION_TYPE_PLATFORM	= 1,
	E_NOTICE_CONDITION_TYPE_SERVER_ID	= 2,
};

enum IOSVersion
{
	E_IOS_VERSION_5			= 5,
	E_IOS_VERSION_6			= 6,
	E_IOS_VERSION_7			= 7,
};

enum ServerType
{
    E_SERVER_TYPE_GAME_SERVER           = 1,
    E_SERVER_TYPE_RELAY_SERVER          = 2,
};

typedef struct STC_RELAY_SERVER_STATUS
{
	uint32	dwServerId;
	uint8	byType;
	uint32	dwGroup;
	uint8	byMergeGroup;
	uint32	dwUpdateTime;
	bool	bIsAlive;
} STC_RELAY_SERVER_STATUS;
struct STC_BATTLE_SERVER_DETAIL
{
	uint32		dwRelayServerId;
	std::string	strServerIp;
	uint32	 	dwServerPort;
	std::string	strBattleId;
	uint32		dwLastUpdateTime;
	uint32		dwConnectPlayerNum;
	STC_BATTLE_SERVER_DETAIL():dwRelayServerId(0),dwServerPort(0),dwLastUpdateTime(0),dwConnectPlayerNum(0){}
};
typedef std::map<std::pair<uint8/*RelayServerType*/,uint32/*RelayServerGroup*/>, STC_RELAY_SERVER_STATUS> RelayServerStatusContainer;
typedef std::vector<STC_BATTLE_SERVER_DETAIL> BattleServerDetailContainer;
typedef struct STC_SERVER_STATUS
{
	uint32 			dwServerId;
	std::string		strServerName;
	std::string 	strIp;
	std::string		strLocalIp;
	uint32 			dwPort;
	std::string		strVersion;
	std::string		strClientVer;
	std::string		strClientResVer;
	std::string		strResVer;
	std::string		strResServerAddr;
	uint32 			dwOnlineNum;
	uint8 			byCanLogin;
	uint8 			byStatus;
	uint32			dwLoginStrategy;
	uint32			dwLastUpdateTime;
	bool 			bIsAlive;
	bool			bIsTest;
	uint8			byActivateReq;
	uint8           byServerIdentify;
	std::string     strDbName;
} STC_SERVER_STATUS;

typedef struct STC_PASSPORT_INFO
{
	STC_PASSPORT_INFO()
	:ddwPassportId(0),dwPlatform(0),byAuthType(0),dwCreateTime(0),byGmAuth(0),
	dwLastLoginTime(0),
	dwLastLoginServerId(0),
	dwTokenTime(0),
	byCDKStatus(0)
	{
	}
	uint64			ddwPassportId;
	std::string		strPassport;
	std::string		strPwd;
	std::string		strMail;
	std::string		strUid;
	std::string		strToken;
	uint32			dwPlatform;
	uint8			byAuthType;
	uint32			dwCreateTime;
	uint8			byGmAuth;
	std::string		strCreateIp;
	std::string		strCreateDevice;
	std::string		strCreateDeviceType;
	uint32			dwLastLoginTime;
	uint32			dwLastLoginServerId;
	std::string		strOpenUdid;
	std::string		strAppleUdid;
	std::string		strTokenIp;
	uint32			dwTokenTime;
	uint8           byCDKStatus;
} STC_PASSPORT_INFO;

typedef struct STC_LOGIN_STRATEGY_CONDITION
{
	uint8		byType;
	std::string strValue;
	bool		bIsNot;

} STC_LOGIN_STRATEGY_CONDITION;

typedef struct STC_LOGIN_STRATEGY
{
	std::vector<
		std::vector< STC_LOGIN_STRATEGY_CONDITION >	// or
	> vvConditions;	// and
} STC_LOGIN_STRATEGY;

typedef struct STC_CHARGE_INFO
{
	uint32		dwAutoId;
	uint32		dwRoleId;
	uint32		dwGoodsId;
	uint32		dwGoodsQuantity;
	std::string	strAddition2;
	uint32		dwPlatform;

	std::string strAddition5;

} STC_CHARGE_INFO;

typedef struct STC_SERVER_MERGE
{
	uint32 originid;
	uint32 mergeid;
	uint32 mergetype;
}STC_SERVER_MERGE;

typedef struct STC_GOODS_INFO
{
	uint32			dwGoodsId;
	uint8			byShopType;
	uint32			dwBuyTypeId;
	uint32			dwBuyContentId;
	uint32			dwBuyCount;
	uint32			dwCostTypeId;
	uint32			dwCostContentId;
	uint32			dwCostCount;
	uint32			dwCostCountOld;
	GoodsState		byStatus;
	uint32			dwLimitDay;
	uint32			dwSortIdx;
	std::string		strIcon;
	std::string		strName;
	std::string		strDescription;
	std::string		strPlatformGoodsId;
	uint32			dwPlatformType;

	uint32			dwDiscount;
	uint16			wRoleLvReq;
	uint32			dwBonusBuyCount;
	uint32			dwBonusLimit;
	uint8			byBonusResetType;
	uint32			dwDiamondPay;
	STC_GOODS_INFO():dwGoodsId(0),byShopType(0),dwBuyTypeId(0),dwBuyContentId(0),dwBuyCount(0),dwCostTypeId(0)
	,dwCostContentId(0),dwCostCount(0),dwCostCountOld(0),byStatus(E_GOODS_STATE_ON_SALE),dwLimitDay(0),dwSortIdx(0),
	dwPlatformType(0),dwDiscount(0),wRoleLvReq(0),dwBonusBuyCount(0),dwBonusLimit(0),byBonusResetType(0),dwDiamondPay(0){}
} STC_GOODS_INFO;

typedef struct STC_DUNGEON_HELP_INFO
{
	uint32		dwAutoId;
	uint32		dwRoleId;	// 帮助者
	uint32		dwTargetId;	// 被帮助者
	uint16		wDungeonIdx;
	uint32		dwKingHeroId;// 帮助者国王Id

} STC_DUNGEON_HELP_INFO;

class LoginDBNoticeInfo
{
public:
	bool	bIsPlateForm;
	uint32	dwPlateFormId;

	uint32	dwAutoId;
	uint32	dwStartTs;
	uint32	dwEndTs;
	std::string strContent;
};

class CenterDBCtrl
{
public:

	static bool ExistsIndex( const char* table, const char* index );
	static bool ExistsColumn( const char* table, const char* column );
	static bool ExistsTable( const char* table );
	static bool ExistsServer( uint32 dwServerid );
	static bool InitCenterDB( DatabaseMysql* db );
	static bool InitCenterDB( DatabaseMysql* db, uint8 byLoginServerId );

	static uint32 NextRoleId();
	static uint64 NextPassportId( uint32 dwPlatformId );

	static uint32 GetDBTime();

	static bool ExistsPassport( uint64 dwPassport );
	static bool ExistsPassport( std::string strPassport, uint8 byAuthType, uint32 dwPlatform );
	static bool ExistsRole( uint32 dwRoleId );

	//////////////////////////////////////
	/////////// Server /// ///////////////

	static bool GetMergedListInfo(std::map<uint32, STC_SERVER_MERGE>& out);
	static uint32 GetServerIdOriginByRoleId(uint32 roleId);

	/** 由Game Server定时写入自身信息 */
	static bool UpdateGameServerInfo(
			uint32 			dwServerId,
			std::string		strServerName,
			std::string		strIp,
			std::string		strLocalIp,
			uint32 			dwPort,
			std::string 	strResServerAddr,
			uint32 			dwOnlineNum,
			std::string 	strVersion,
			std::string		strResVersionFull,
			std::string		strResVersionConfig,
			uint8			byCanLogin,
			uint8 			byStatus,
			uint32			dwLoginStrategyId,
			uint8           byServerIdentify,
			uint8 			byIsTest,
			std::string     strDbName );

	static bool UpdateRelayServerInfo(
			uint32 dwServerId,
			uint8  byType,
			uint32  dwGroup,
			uint32 dwDBTime);
    static uint32 GetServerIdMerged( uint32 dwServerId );
    static bool GetServerMergeLog( std::map<uint32,uint32>& mapMergeTable );
	//获取合服的Role，及其合服前的ServerId
	// mapRoleOriginSID: <RoleId, OriginServerId>
	static bool GetMergedRoleIds(std::map<uint32, uint32>& mapRoleOriginSID, uint32 dwServerId);

	static bool UpdateBattleserverInfo(uint32 dwRelayId,uint32 dwTotal,uint32 dwRunning,uint32 dwPlayerNum);
//	static bool UpdateBattleserverList(BattleServerDetailContainer& vecBattleServer);
	static bool DelBattleServer(std::string ip,uint32 dwPort);

	static bool GetLoginStrategy(
			uint32				dwStrategyId,
			STC_LOGIN_STRATEGY& oStrategy );

	static bool GetOrUpdateGameServerStatus(
			std::map<uint32, STC_SERVER_STATUS>& mapServer );
	static bool GetOrUpdateRelayServerStatus(
			RelayServerStatusContainer& mapRelayServer);
	/*static bool GetOrUpdateBattleServerStatus(
			BattleServerStatusContainer& mapRelayServer);*/

	static bool UpdateClosedGameServer( uint32 dwServerId );

	static bool IsTestServer( uint32 dwServerId );

	//////////////////////////////////////
	/////////// Passport /////////////////
	static bool IsPassportRegistered(
	        std::string     strPassport
	                                );
	static LoginResultType ValidateAuthAccount(
			std::string 	strPassport,
			std::string 	strPwd,
			uint32			dwPlatform,
			uint64&			odwPassportId );

	static LoginResultType ValidateAuthPlatform(
			std::string		strPlatformToken,
			std::string		strUid,
			std::string 	strDeviceToken,
			uint32			dwPlatform,
			std::string		strRegIp,
			std::string		strRegDevice,
			std::string		strRegDeviceType,
			std::string		strOpenUdid,
			std::string		strAppleUdid,
			uint64&			odwPassportId );

	static LoginResultType ValidateAuthFast(
			std::string		strUid,
			std::string		strOpenUdid,
			std::string		strAppleUdid,
			uint8			byIOSVersion,
			std::string		strDeviceToken,
			uint32			dwPlatform,
			std::string		strRegIp,
			std::string		strRegDevice,
			std::string		strRegDeviceType,
			uint64&			odwPassportId,
			uint8           byCDKStatus);

	static bool RegisterPassport(
			std::string		strPassport,
			std::string		strPwd,
			std::string		strMail,
			std::string		strUid,
			std::string		strToken,
			uint32			dwPlatform,
			std::string		strRegIp,
			std::string		strRegDevice,
			std::string		strRegDeviceType,
			std::string		strOpenUdid,
			std::string		strAppleUdid,
			uint8           );

	static bool ModifyPassportAndPassword( uint64 dwPassportId, std::string Passport, std::string strPwd );
	static bool ModifyPassword( uint64 dwPassportId, std::string strNewPwd );
	static bool ModifyPassword( uint32 dwRoleId, std::string strOldPwd, std::string strNewPwd );

	static bool UpdateCDKStatus( uint64 dwPassportId, uint8 byCDKStatus );

	static bool InsertPassportInfo(
			std::string		strPassport,
			std::string		strPwd,
			std::string		strMail,
			std::string		strUid,
			std::string		strToken,
			uint32			dwPlatform,
			uint8			byAuthType,
			uint32			dwCreateTime,
			uint8			byGmAuth,
			std::string		strCreateIp,
			std::string		strCreateDevice,
			std::string		strCreateDeviceType,
			std::string		strOpenUdid,
			std::string		strAppleUdid,
			uint64&			odwPassportId,
			uint8           byCDKStatus );

	static bool GetPassportInfo( uint64 dwPassportId, STC_PASSPORT_INFO& stcInfo );

	static bool UpdatePassportGmAuth(
			uint64			dwPassportId,
			uint8			byGmAuth
	);
	static bool SetLastLoginServer( uint64 dwPassportId, uint16 wServerId );

	// return passport_id
	static uint64 UpdateRoleToken( uint32 dwRoleId, std::string strToken );

	//////////////////////////////////////
	/////////// Role /////////////////////

	static uint64 GetPassportId( uint32 dwRoleId );
	static uint32 GetRoleId( uint64 dwPassportId, uint32 dwServerIdOrigin );
	static uint32 GetOrInsertRoleId( uint64 dwPassportId, uint32 dwServerIdOrigin );
	static bool IsRoleForbid( uint32 dwRole );

	static bool InsertOrUpdateRoleInfo(
			uint32			dwRoleId,
			std::string		strRoleName,
			uint8           byGmAuth,
			uint32			dwProgress,
			uint32			dwLevel,
			uint32			dwGold,
			uint32			dwDiamond,
			uint32			dwCurStage,
			uint32			dwCurTrain,
			uint32			dwVipLevel,
			uint32			dwVipExp,
			uint32			dwStamina,
			uint32			dwEnergy,
			uint32			dwMainQuestId,
			uint32			dwDiamondPay,
			uint32			dwCreateTime
	);

	static uint32 InsertLoginInfo(
			uint32 			dwRoleId,
			std::string		strLoginIp,
			std::string		strLoginDevice,
			std::string		strLoginDeviceType );

	static bool InsertLogoutInfo( uint32 dwAutoId );

	static bool UpdateRoleGmAuth(
			uint32			dwRoleId,
			uint8			byGmAuth );

	static uint8 GetGmAuthByPassportId( uint64 ddwPassportId );
	static uint8 GetGmAuth( uint32 dwRoleId );
	static uint32 GetPlatformId( uint32 dwRoleId );
	static uint32 GetRoleCreateTime( uint32 dwRoleId );
	static std::string GetRoleName( uint32 dwRoleId );
	static uint16 GetRoleServerId( uint32 dwRoleId );
	static uint16 GetRoleServerIdOrigin( uint32 dwRoleId ); // add by swt
	static int GetRolePlatform( uint32 dwRoleId ); // return -1 if error

	static bool SetRoleStatus( uint32 dwRoleId, uint8 byStatus );
	static uint8 GetRoleStatus( uint32 dwRoleId );

	static bool SetRoleLevel( uint32 dwRoleId,uint16 wLevel);
	static void GetRoleLevel(uint64 dwPassportId, std::map<uint32/*serverId*/,uint32/*level*/>& mapServerLevel);

	static uint32 GetPtrCount( uint32 dwRoleId );

	//////////////////////////////////////
	/////////// Charge ///////////////////

	static bool GetUnhandledCharge(
			std::list<STC_CHARGE_INFO>& vCharges );

	static bool ChargeHandled(
			uint32		dwAutoId,
			uint32		dwDiamondValue,
			std::string strIp,
			std::string strDevice,
			std::string strDeviceType,
			std::string strDeviceUid,
			uint32		dwDiamondPay );

	static uint32 CreateCharge(
			uint32		dwRoleId,
			uint32		dwGoodsId,
			uint32		dwGoodsQty,
			std::string	strCurrency,
			uint32		dwValue,
			std::string	strInnerOrderId,
			std::string strPlatformOrderId,
			std::string strPlatformAccount,
			uint32		dwPlatform,
			uint16		wPaymentType,
			uint32		dwPaymentTime,
			std::string	strClientOrderId );

	static bool HasCharge( uint32 dwPlatform, std::string strPlatformOrderId );

	static uint32 InsertPurchaseInfo(
			uint32			dwRoleId,
			uint32			dwGoodsId,
			uint32			dwGoodsQuantity,
			uint32			dwValue,
			uint32			dwDiamondPaidUse,
			uint32			dwTime );

	static bool SetChargeStatus( std::vector< uint32 >& vecRoleId, uint32 dwStaratTime, uint32 dwEndTime, std::string& strErrMsg );

	static std::string GetNotice( NoticeUseType eUseType, NoticeConditionType eCondType, uint32 dwCondValue );

	// 获取GameServer的公告信息，建议每10分钟自动更新一次（也可GM指令触发）
	// @ref	GetNotice | E_NOTICE_USE_TYPE_GAME
	static bool GetGameNotice(uint32 dwServId, std::vector<LoginDBNoticeInfo> & vecNotice);

	////////////////////////////////////
	//////////// Bind //////////////////
	static uint8 BindPassport( uint32 dwRoleId, LoginAuthType eAuthType, std::string strPassport,
			std::string strPassword = "", std::string strMail = "" );

	///////////////////////////////////
	//////////// Guild Reg ////////////
	static uint32 RegisterGuild( uint32 dwRoleId );

	////////////////////////////////////
	/////  获取单个server的信息 //////////
	static bool GetServerName( uint32 dwServerId, std::string &strServerName);

	/////
	static bool GetServerDbName( uint32 dwServerId, std::string &strDbName);

	static bool GetServerLocalIp( uint32 dwServerId, std::string &strIp );

	static bool UpdateServerIsTest( uint32 dwServerId, uint32 dwIsTest );

	//////////////////////////////////////
	/////////// GM Command ///////////////

	static bool InitGmCommand();

	// poll in game server
	static bool ReadNewGmCommand();
	static bool UpdateGmCommand();
	static bool HasGmCommand( uint32 dwGmCommandId );
	static bool CancelGmCommand( uint32 dwGmCommandId );
	static const GmCommand* GetGmCommand( uint32 dwGmCommandId );
	static uint32 HandlerGmCommandRole( void* pRole, uint32 dwLastCmdTime );
	static uint32 CreateSysGmCommand( std::string strOpr, std::string strParams, uint8 byTargetType,
			uint32 dwTargetId, uint32 dwStartTime, uint32 dwEndTime );

	////////// login token checking /////////////
	static void CreateLoginToken( uint64 ddwPassportId, std::string strIp,bool bIsShadowLogin=false/*不记录留存*/ );
	static void ClearLoginToken( uint32 dwRoleId );
	static bool CheckLoginToken( uint32 dwRoleId, std::string strIp ,bool& bIsShadowLogin);

	//////////////////////////////////////
	/////////// Gift Box /////////////////
	static bool GetGiftCodeInfo( uint32 dwId, uint32& odwParam1, uint32& odwParam2, uint32& odwParam3,
			std::string& ostrReward, std::string& ostrServers, std::string& ostrPlatform, uint32& odwDeadTime,
			uint32& odwMaxUse, uint32& odwEveryUse );
	static uint32 GetGiftCodeUsed( uint64 ddwRoleId, uint32 dwId, uint32 dwIdx );
	static uint32 GetGiftCodeUsed( uint32 dwId, uint32 dwIdx );
	static uint32 GetGiftCodeSameTypeUsed( uint64 dwRoleId, uint32 dwId ); //giftcode-roleid cdk-passportid
	static bool InsertGiftCodeUse( uint64 dwRoleId, uint32 dwId, uint32 dwIdx );

	//////////////////////////////////////
	//////////// Goods ///////////////////
	static void GetGoodsInfoOfGameServer( std::map<uint32, STC_GOODS_INFO>& vGoods, uint32 dwGameServerId );
	static void UpdateGoodsInfoOfGameServerAll( std::map<uint32, STC_GOODS_INFO>& vGoods, uint32 dwGameServerId );
	static void InsertOrUpdateGoodsInfo(STC_GOODS_INFO& stcGood, uint32 dwGameServerId);

private:

	static void ReadCommands( QueryResult* result );
	static bool AppendGmCommand( GmCommand* pCommand );
	static bool RemoveGmCommand( GmCommand* pCommand );

	static void UpdateGoodsInfoOfGameServer( std::map<uint32, STC_GOODS_INFO>& vGoods, uint32 dwGameServerId );



	static DatabaseMysql* 	m_db;
	static uint8			m_byLoginServerId;
	static uint32			m_maxPassportAutoInc;
	static uint32			m_maxRoleAutoInc;
	static bool 			m_bReadOnly;

	static uint32			m_dwMaxHandledGmCommandId;
	static uint32			m_dwMaxRunningGmCommandTime;

	typedef std::map<uint32, GmCommand*> GmCommandMapType;
	static GmCommandMapType m_mapGmCommandAll;

};

}
#endif
