#include "CenterDBCtrl.h"
#include "Log.h"
#include "QueryResult.h"
#include <vector>
#include <string>
#include "Util.h"
#include <stdlib.h>
#include <sstream>

namespace CenterDB
{
DatabaseMysql* 	CenterDBCtrl::m_db = NULL;
uint8			CenterDBCtrl::m_byLoginServerId = 0;
uint32			CenterDBCtrl::m_maxPassportAutoInc = 0;
uint32			CenterDBCtrl::m_maxRoleAutoInc = 0;
bool			CenterDBCtrl::m_bReadOnly = false;
uint32			CenterDBCtrl::m_dwMaxHandledGmCommandId = 0;
uint32			CenterDBCtrl::m_dwMaxRunningGmCommandTime = 0;

std::map<uint32, GmCommand*> CenterDBCtrl::m_mapGmCommandAll;

bool CenterDBCtrl::ExistsIndex( const char* table, const char* index )
{
	QueryResult* result = m_db->PQuery(
		"select * from information_schema.statistics where table_schema='%s' and table_name='%s' and index_name='%s'",
		m_db->GetDbName(), table, index
	);

	if ( result )
	{
		delete result;
		return true;
	}
	return false;
}

bool CenterDBCtrl::ExistsColumn( const char* table, const char* column )
{
	QueryResult* result = m_db->PQuery(
		"select * from information_schema.columns where table_schema='%s' and table_name='%s' and column_name='%s'",
		m_db->GetDbName(), table, column );
	if ( result )
	{
		delete result;
		return true;
	}
	return false;
}

bool CenterDBCtrl::ExistsTable( const char* table )
{
	QueryResult* result = m_db->PQuery(
		"select * from information_schema.tables where table_schema='%s' and table_name='%s'",
		m_db->GetDbName(), table
	);
	if ( result )
	{
		delete result;
		return true;
	}
	return false;
}

bool CenterDBCtrl::ExistsServer( uint32 dwServerId )
{
    QueryResult* result = m_db->PQuery(
            "select 1 from gameserver_info where server_id=%u", dwServerId
            );
    if ( result )
    {
        delete result;
        return true;
    }
    return false;
}

uint32 CenterDBCtrl::NextRoleId()
{
	// 28 bit Inc + 3 bit LoginServerId + 1 bit RobotFlag
	++m_maxRoleAutoInc;
	return ( m_maxRoleAutoInc << 4 ) | ( (uint32)m_byLoginServerId << 1 ); // | 0  [lowest bit reserved for robot]
}

uint64 CenterDBCtrl::NextPassportId( uint32 dwPlatformId )
{
	// 32 bit Inc + 3 bit LoginServerId + 29 bit PlatformId
	++m_maxPassportAutoInc;
	return ( ((uint64)m_maxPassportAutoInc) << 32 ) | ( ((uint64)m_byLoginServerId) << 29 ) | ( ((uint64)dwPlatformId) );
}

bool CenterDBCtrl::InitCenterDB( DatabaseMysql* db )
{
	if ( !InitCenterDB( db, 0 ) )
	{
		return false;
	}
	m_bReadOnly = true;
	return true;
}

bool CenterDBCtrl::InitCenterDB( DatabaseMysql* db, uint8 byLoginServerId )
{
	if ( db == NULL )
	{
		IME_SYSTEM_ERROR( "InitCenterDB", "Param Is Null");
		return false;
	}

	if ( m_db )
	{
		IME_SYSTEM_ERROR( "InitCenterDB", "Duplicated Init" );
		return false;
	}

	if ( m_byLoginServerId >= ( 1 << 3 ) )
	{
		IME_SYSTEM_ERROR( "InitCenterDB", "Max LoginServerId Is 8" );
		return false;
	}

	m_bReadOnly = false;
	m_db = db;
	m_byLoginServerId = byLoginServerId;
	// gift_generate_record
	m_db->DirectExecute(
		"CREATE TABLE IF NOT EXISTS gift_generate_record ("
			"id				INT			UNSIGNED	NOT NULL,"
			"idx			INT			UNSIGNED	NOT NULL,"
			"gen_start		INT			UNSIGNED	NOT NULL,"
			"gen_end		INT			UNSIGNED	NOT NULL,"
			"gen_use		VARCHAR(512) CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"create_time	INT			UNSIGNED	NOT NULL,"
			"creator		VARCHAR(32)	CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"PRIMARY KEY(id, idx)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	// gift_generate_record_backup
	m_db->DirectExecute(
		"CREATE TABLE IF NOT EXISTS gift_generate_record_backup ("
			"id				INT			UNSIGNED	NOT NULL,"
			"idx			INT			UNSIGNED	NOT NULL,"
			"gen_start		INT			UNSIGNED	NOT NULL,"
			"gen_end		INT			UNSIGNED	NOT NULL,"
			"gen_use		VARCHAR(512) CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"create_time	INT			UNSIGNED	NOT NULL,"
			"creator		VARCHAR(32)	CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"backup_time	INT			UNSIGNED	NOT NULL,"
			"PRIMARY KEY(id, idx)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);


	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS login_strategy_backup ("
			"idx 			INT		UNSIGNED	NOT NULL	AUTO_INCREMENT,"
			"auto_id		INT		UNSIGNED	NOT NULL	DEFAULT '0',"
			"strategy_id	INT 	UNSIGNED	NOT NULL,"
			"condition_id	TINYINT	UNSIGNED	NOT NULL,"
			"type			TINYINT	UNSIGNED	NOT NULL,"
			"value			VARCHAR(64)	CHARACTER SET utf8 DEFAULT NULL COMMENT '记录创建人',"
			"creator		VARCHAR(32) CHARACTER SET utf8 DEFAULT NULL COMMENT '备注',"
			"remark 		VARCHAR(100) DEFAULT NULL COMMENT '备注',"
			"backup_time	INT		UNSIGNED	NOT NULL	DEFAULT '0'	COMMENT '备份时间',"
			"is_not 		TINYINT UNSIGNED	NOT NULL DEFAULT '0',"
			"PRIMARY KEY(idx)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);
	// logindevicelevel_info
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS logindevicelevel_info ("
			"summarydate	DATE					NOT	NULL COMMENT '统计日期',"
			"server_id		INT			UNSIGNED	NOT	NULL COMMENT '分区',"
			"platform		INT			UNSIGNED	NOT	NULL COMMENT '用户平台',"
			"uid			VARCHAR(128) CHARACTER SET utf8 DEFAULT NULL COMMENT '设备号',"
			"passport_id	BIGINT		UNSIGNED	NOT	NULL COMMENT '用户ID',"
			"role_id		INT			UNSIGNED	NOT	NULL COMMENT '角色ID',"
			"level			INT			UNSIGNED	NOT	NULL COMMENT '角色等级',"
			"vip_level		INT			UNSIGNED	NOT	NULL COMMENT 'vip等级',"
			"vip_exp		INT			UNSIGNED	NOT	NULL COMMENT 'vip总经验',"
			"reg_device		VARCHAR(32)	CHARACTER SET utf8 DEFAULT NULL COMMENT '设备',"
			"player_icon	INT			UNSIGNED	NOT	NULL COMMENT '职业',"
			"role_last_login_time	INT	UNSIGNED	NOT	NULL COMMENT '角色最后登录时间',"
			"morrowloginflag	INT		UNSIGNED	NOT	NULL COMMENT '是否次日登录',"
			"create_time	INT			UNSIGNED	NOT	NULL COMMENT '用户创建时间',"
			"role_create_time	INT		UNSIGNED	NOT	NULL COMMENT '角色创建时间',"
			"PRIMARY KEY (summarydate,server_id,platform,uid,role_id,passport_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	// gameserver_info
	m_db->PExecute(
			"CREATE TABLE IF NOT EXISTS relayserver_info ("
				"relay_type		 TINYINT		UNSIGNED	NOT NULL,"
				"relay_group	 INT			UNSIGNED	NOT NULL,"//废弃
				"server_id		 SMALLINT		UNSIGNED	NOT NULL,"
				"update_time	 INT			UNSIGNED	NOT NULL,"
				"merge_group	 TINYINT		UNSIGNED	NOT NULL,"//0：没有进行过合服，	X：该组已被并入X组
				"PRIMARY KEY (relay_type,relay_group)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);
	m_db->PExecute(
			"CREATE TABLE IF NOT EXISTS battleserver_info ("
				"relay_id		INT			UNSIGNED	NOT NULL,"
				"connect_num	INT			UNSIGNED	NOT NULL,"
				"running_battle	INT			UNSIGNED	NOT NULL,"
				"player_num		INT			UNSIGNED	NOT NULL,"
				"PRIMARY KEY (relay_id)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);
	if ( !ExistsColumn( "battleserver_info", "player_num" ) )
	        m_db->PExecute( "ALTER TABLE battleserver_info ADD COLUMN player_num INT UNSIGNED    NOT NULL" );

	m_db->PExecute(
			"CREATE TABLE IF NOT EXISTS battleserver_list ("
//				"auto_id 			INT			UNSIGNED	NOT NULL	AUTO_INCREMENT,"
				"relay_id			INT			UNSIGNED	NOT NULL,"
				"server_ip			VARCHAR(64)	CHARACTER SET utf8 NOT NULL,"
				"server_port		INT			UNSIGNED	NOT NULL,"
				"battle_id			VARCHAR(128)CHARACTER SET utf8 NOT NULL,"
				"last_update_time	INT			UNSIGNED	NOT NULL,"
				"player_num			INT			UNSIGNED	NOT NULL,"
				"PRIMARY KEY (server_ip,server_port)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);
	if ( !ExistsColumn( "battleserver_list", "player_num" ) )
	        m_db->PExecute( "ALTER TABLE battleserver_list ADD COLUMN player_num INT UNSIGNED    NOT NULL" );

	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS gameserver_info ("
			"server_id		SMALLINT	UNSIGNED	NOT NULL,"
			"server_name	VARCHAR(32)	CHARACTER SET utf8 NOT NULL,"
			"ip				VARCHAR(64)	CHARACTER SET utf8 NOT NULL,"
			"local_ip		VARCHAR(64)	CHARACTER SET utf8 NOT NULL,"
			"port			SMALLINT	UNSIGNED	NOT NULL,"
			"version		VARCHAR(20)	CHARACTER SET utf8 NOT NULL,"
			"res_version	VARCHAR(200) CHARACTER SET utf8 NOT NULL,"
			"res_version_config VARCHAR(200) CHARACTER SET utf8 NOT NULL,"
			"res_server_ip	VARCHAR(64) CHARACTER SET utf8 NOT NULL,"
			"online_num		MEDIUMINT	UNSIGNED	NOT NULL,"
			"can_login		TINYINT		UNSIGNED	NOT NULL,"
			"status			TINYINT		UNSIGNED	NOT NULL,"
			"update_time	INT			UNSIGNED	NOT NULL,"
			"login_strategy_id	INT		UNSIGNED	NOT NULL,"
			"is_test		TINYINT		UNSIGNED	NOT NULL DEFAULT '0',"
			"activate_req	TINYINT		UNSIGNED	NOT NULL,"	// 0 ~ 255
	        "server_identify TINYINT    UNSIGNED    NOT NULL,"
	        "db_name        VARCHAR(32) CHARACTER SET utf8 NOT NULL,"       // server_version
			"PRIMARY KEY (server_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);
    if ( !ExistsColumn( "gameserver_info", "server_identify" ) )
        m_db->PExecute( "ALTER TABLE gameserver_info ADD COLUMN server_identify TINYINT UNSIGNED NOT NULL" );
	if ( !ExistsColumn( "gameserver_info", "activate_req" ) )
		m_db->PExecute( "ALTER TABLE gameserver_info ADD COLUMN activate_req TINYINT UNSIGNED	NOT NULL" );
	if ( !ExistsColumn( "gameserver_info", "db_name" ) )
	        m_db->PExecute( "ALTER TABLE gameserver_info ADD COLUMN db_name VARCHAR(20) CHARACTER SET utf8 NOT NULL" );

	// player_info
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS player_info ("
			"role_id		INT			UNSIGNED	NOT NULL,"
			"name			VARCHAR(32)	CHARACTER SET utf8 NOT NULL,"
			"gm_auth		TINYINT		UNSIGNED	NOT NULL,"
			"status			TINYINT		UNSIGNED	NOT NULL,"
			"progress		INT			UNSIGNED	NOT NULL,"
			"level			INT			UNSIGNED	NOT NULL,"
			"gold			INT			UNSIGNED	NOT NULL,"
			"diamond		INT			UNSIGNED	NOT NULL,"
			"cur_stage		INT			UNSIGNED	NOT NULL,"
			"cur_train		INT			UNSIGNED	NOT NULL,"
			"vip_level		INT			UNSIGNED	NOT NULL,"
			"vip_exp		INT			UNSIGNED	NOT NULL,"
			"stamina		INT			UNSIGNED	NOT NULL,"
			"energy			INT			UNSIGNED	NOT NULL,"
			"quest			INT			UNSIGNED	NOT NULL,"
			"last_login_time	INT		UNSIGNED	NOT NULL,"
			"create_time	INT			UNSIGNED	NOT NULL DEFAULT '0',"
			"diamond_pay	INT			UNSIGNED	NOT NULL DEFAULT '0',"
			"PRIMARY KEY (role_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);
	if ( !ExistsColumn( "player_info", "create_time" ) )
	        m_db->PExecute( "ALTER TABLE player_info ADD COLUMN create_time INT UNSIGNED    NOT NULL DEFAULT '0'" );
	// passport_info
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS passport_info ("
			"passport_id	BIGINT		UNSIGNED	NOT NULL,"
			"passport		VARCHAR(128)	CHARACTER SET utf8 NOT NULL,"
			"pwd			VARCHAR(64) CHARACTER SET utf8 NOT NULL,"
			"mail			VARCHAR(64) CHARACTER SET utf8 NOT NULL,"
			"uid			VARCHAR(128) CHARACTER SET utf8 NOT NULL,"
			"token			VARCHAR(128) CHARACTER SET utf8 NOT NULL,"
			"platform		INT			UNSIGNED	NOT NULL,"
			"auth_type		TINYINT		UNSIGNED	NOT NULL,"
			"create_time	INT			UNSIGNED	NOT NULL,"
			"gm_auth		TINYINT		UNSIGNED	NOT NULL,"
			"reg_ip			VARCHAR(64) CHARACTER SET utf8 NOT NULL,"
			"reg_device		VARCHAR(32)	CHARACTER SET utf8 NOT NULL,"
			"reg_device_type VARCHAR(64) CHARACTER SET utf8 NOT NULL,"
			"last_login_time	INT		UNSIGNED	NOT NULL,"
			"last_login_server	SMALLINT	UNSIGNED	NOT NULL DEFAULT '0',"
			"open_udid		VARCHAR(128)	CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"apple_udid		VARCHAR(128)	CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"token_ip		VARCHAR(32)	CHARACTER SET utf8 NOT NULL,"
			"token_time		INT	UNSIGNED	NOT NULL,"
	        "cdk_status     TINYINT     UNSIGNED    NOT NULL,"
	        "shadow_login   TINYINT     UNSIGNED    NOT NULL,"
			"PRIMARY KEY (passport_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);
	if ( !ExistsColumn( "passport_info", "shadow_login" ) )
	        m_db->PExecute( "ALTER TABLE passport_info ADD COLUMN shadow_login TINYINT UNSIGNED    NOT NULL" );
	if ( !ExistsColumn( "passport_info", "cdk_status" ) )
	        m_db->PExecute( "ALTER TABLE passport_info ADD COLUMN cdk_status TINYINT UNSIGNED    NOT NULL" );
	// login_strategy
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS login_strategy ("
			"auto_id		INT		UNSIGNED	NOT NULL	AUTO_INCREMENT,"
			"strategy_id	INT		UNSIGNED	NOT NULL,"
			"condition_id	TINYINT	UNSIGNED	NOT NULL,"
			"type			TINYINT	UNSIGNED	NOT NULL,"
			"value			VARCHAR(64) CHARACTER SET utf8 NOT NULL,"
			"is_not			TINYINT	UNSIGNED	NOT NULL DEFAULT '0',"
			"PRIMARY KEY(auto_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);
	if ( !ExistsColumn( "login_strategy", "is_not" ) )
		m_db->PExecute( "ALTER TABLE login_strategy ADD COLUMN is_not TINYINT UNSIGNED	NOT NULL DEFAULT '0'" );
	if ( !ExistsColumn( "login_strategy", "remark" ) )
		m_db->PExecute( "ALTER TABLE login_strategy ADD COLUMN remark varchar(100) DEFAULT NULL COMMENT '备注'" );

	// login_strategy_backup
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS login_strategy_backup ("
			"idx 			INT		UNSIGNED	NOT NULL	AUTO_INCREMENT,"
			"auto_id		INT		UNSIGNED	NOT NULL	DEFAULT '0',"
			"strategy_id	INT 	UNSIGNED	NOT NULL,"
			"condition_id	TINYINT	UNSIGNED	NOT NULL,"
			"type			TINYINT	UNSIGNED	NOT NULL,"
			"value			VARCHAR(64)	CHARACTER SET utf8 DEFAULT NULL COMMENT '记录创建人',"
			"creator		VARCHAR(32) CHARACTER SET utf8 DEFAULT NULL COMMENT '备注',"
			"remark 		varchar(100) DEFAULT NULL COMMENT '备注',"
			"backup_time	INT		UNSIGNED	NOT NULL	DEFAULT '0'	COMMENT '备份时间',"
			"is_not 		TINYINT UNSIGNED	NOT NULL DEFAULT '0',"
			"PRIMARY KEY(idx)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	if ( !ExistsColumn( "login_strategy_backup", "remark" ) )
		m_db->PExecute( "ALTER TABLE login_strategy_backup ADD COLUMN remark varchar(100) DEFAULT NULL COMMENT '备注'," );
	if ( !ExistsColumn( "login_strategy_backup", "is_not" ) )
		m_db->PExecute( "ALTER TABLE login_strategy_backup ADD COLUMN is_not TINYINT UNSIGNED	NOT NULL DEFAULT '0'" );

	// re_passport_player
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS re_passport_player ("
			"role_id		INT			UNSIGNED	NOT NULL,"
			"passport_id	BIGINT		UNSIGNED	NOT NULL,"
			"server_id		SMALLINT	UNSIGNED	NOT NULL,"
			"server_id_origin SMALLINT	UNSIGNED	NOT NULL,"
			"create_time	INT			UNSIGNED	NOT NULL,"
			"role_level		INT			UNSIGNED	NOT NULL,"
			"INDEX index_passport (passport_id, server_id_origin),"
			"PRIMARY KEY (role_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);
	if ( !ExistsColumn( "re_passport_player", "role_level" ) )
		m_db->PExecute( "ALTER TABLE re_passport_player ADD COLUMN role_level INT UNSIGNED NOT NULL DEFAULT '0'" );

	// role_login_info
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS role_login_info ("
			"auto_id		INT			UNSIGNED	NOT NULL AUTO_INCREMENT,"
			"role_id		INT			UNSIGNED	NOT NULL,"
			"login_time		INT			UNSIGNED	NOT NULL,"
			"logout_time	INT			UNSIGNED	NOT NULL,"
			"login_ip				VARCHAR(64) CHARACTER SET utf8 NOT NULL,"
			"login_device			VARCHAR(32)	CHARACTER SET utf8 NOT NULL,"
			"login_device_type 		VARCHAR(64) CHARACTER SET utf8 NOT NULL,"
			"PRIMARY KEY (auto_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	// notice_info_v2
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS notice_info_v2 ("
			"auto_id			INT		UNSIGNED	NOT NULL AUTO_INCREMENT,"
			"use_type			TINYINT		UNSIGNED	NOT NULL,"
			"condition_type		TINYINT		UNSIGNED	NOT NULL,"
			"condition_value	MEDIUMINT	UNSIGNED	NOT NULL,"
			"content		VARCHAR(1024) CHARACTER SET utf8 NOT NULL,"
			"start_time			INT		UNSIGNED	NOT NULL DEFAULT '0',"
			"end_time			INT		UNSIGNED	NOT NULL DEFAULT '0',"
			"PRIMARY KEY (auto_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	if ( !ExistsColumn( "notice_info_v2", "start_time" ) )
		m_db->PExecute( "ALTER TABLE notice_info_v2 ADD COLUMN start_time INT UNSIGNED NOT NULL" );
	if ( !ExistsColumn( "notice_info_v2", "end_time" ) )
		m_db->PExecute( "ALTER TABLE notice_info_v2 ADD COLUMN end_time	INT	UNSIGNED NOT NULL" );

	// charge_info
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS charge_info ("
			"auto_id			INT		UNSIGNED	NOT NULL AUTO_INCREMENT,"
			"role_id			INT		UNSIGNED	NOT NULL,"
			"goods_id			INT		UNSIGNED	NOT NULL,"
			"goods_quantity		INT		UNSIGNED	NOT NULL,"
			"currency			VARCHAR(16) CHARACTER SET utf8 NOT NULL,"
			"value				INT		UNSIGNED	NOT NULL,"
			"virtual_value		INT		UNSIGNED	NOT NULL DEFAULT '0',"
			"type				INT		UNSIGNED	NOT NULL,"
			"inner_order_id			VARCHAR(128) CHARACTER SET utf8 NOT NULL,"
			"platform			INT		UNSIGNED	NOT NULL,"
			"platform_order_id		VARCHAR(128) CHARACTER SET utf8 NOT NULL,"
			"platform_account_id	VARCHAR(128) CHARACTER SET utf8 NOT NULL,"
			"platform_payment_type	TINYINT	UNSIGNED	NOT NULL,"
			"state				TINYINT	UNSIGNED	NOT NULL,"
			"payment_time		INT	UNSIGNED	NOT NULL DEFAULT '0',"
			"distribute_time	INT UNSIGNED 	NOT NULL DEFAULT '0',"
			"payment_ip			VARCHAR(64) CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"payment_device		VARCHAR(32) CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"payment_device_type VARCHAR(64) CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"payment_device_uid	VARCHAR(128) CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"client_order_id	VARCHAR(128) CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"addition1			VARCHAR(128) CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"addition2			VARCHAR(128) CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"addition3			VARCHAR(128) CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"addition4			VARCHAR(128) CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"addition5			VARCHAR(128) CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"diamond_pay		INT		UNSIGNED	NOT NULL DEFAULT '0',"
			"PRIMARY KEY (auto_id),"
			"INDEX index_client_order_id ( client_order_id )"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	if ( !ExistsColumn( "charge_info", "diamond_pay" ) )
		m_db->PExecute( "ALTER TABLE `charge_info` ADD COLUMN `diamond_pay` int(10) unsigned NOT NULL DEFAULT 0 COMMENT '充值付费虚拟货币';");

	// purchase_info (diamond only)
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS purchase_info ("
			"auto_id		INT		UNSIGNED	NOT NULL AUTO_INCREMENT,"
			"role_id		INT		UNSIGNED	NOT NULL,"
			"goods_id		INT		UNSIGNED	NOT NULL,"
			"goods_quantity	INT		UNSIGNED	NOT NULL,"
			"value			INT		UNSIGNED	NOT NULL,"
			"diamond_paid_use	INT		UNSIGNED	NOT NULL,"
			"time			INT		UNSIGNED	NOT NULL,"
			"PRIMARY KEY (auto_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	// gm_cmd
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS gm_cmd ("
			"auto_id			INT		UNSIGNED	NOT NULL AUTO_INCREMENT,"
			"opr			VARCHAR(32)	CHARACTER SET utf8 NOT NULL,"
			"params			TEXT NOT NULL DEFAULT '',"
			"target_type	TINYINT		UNSIGNED	NOT NULL,"
			"target_id		BIGINT		UNSIGNED	NOT NULL,"
			"start_time			INT		UNSIGNED	NOT NULL DEFAULT '0',"
			"end_time			INT		UNSIGNED	NOT NULL DEFAULT '0',"
			"backup_value	VARCHAR(128) 	CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"status			TINYINT		UNSIGNED	NOT NULL DEFAULT '0',"
			"sortserial		INT(3)		UNSIGNED	NOT NULL DEFAULT '100',"
			"error_msg		VARCHAR(256)	CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"create_time		INT		UNSIGNED	NOT NULL DEFAULT '0',"
			"author			VARCHAR(32) 	CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"PRIMARY KEY (auto_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	if ( !ExistsColumn( "gm_cmd", "sortserial" ) )
	{
		m_db->PExecute( "ALTER TABLE gm_cmd ADD COLUMN sortserial INT UNSIGNED NOT NULL DEFAULT '100'" );
	}

	// gameserver_merge_log
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS gameserver_merge_log ("
			"auto_id			INT		UNSIGNED 	NOT NULL AUTO_INCREMENT,"
			"server_id_origin	INT		UNSIGNED	NOT NULL DEFAULT 0,"
			"server_id_merged	INT		UNSIGNED	NOT NULL DEFAULT 0,"
			"merge_time			INT		UNSIGNED	NOT NULL DEFAULT 0,"
			"PRIMARY KEY( auto_id )"
		")ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	// player_detail_info
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS player_detail("
			"role_id		INT			UNSIGNED	NOT NULL,"
			"diamond_pay	INT			UNSIGNED	NOT NULL DEFAULT '0',"
			"last_op		INT			UNSIGNED	NOT NULL DEFAULT '0',"
			"PRIMARY KEY(role_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	// gift_box
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS gift_box("
			"id			INT		UNSIGNED	NOT NULL,"
			"idx		INT		UNSIGNED	NOT NULL,"
	        "role_id    BIGINT  UNSIGNED    NOT NULL,"
			"use_time	INT		UNSIGNED	NOT NULL,"
			"PRIMARY KEY(id, idx, role_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	// gift_box_backup
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS gift_box_backup("
			"id			INT		UNSIGNED	NOT NULL,"
			"idx		INT		UNSIGNED	NOT NULL,"
			"role_id	BIGINT	UNSIGNED	NOT NULL,"
			"backup_time	INT		UNSIGNED	NOT NULL,"
			"PRIMARY KEY(id, idx, role_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	// gift_box_config
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS gift_box_config("
			"id			INT		UNSIGNED	NOT NULL,"
			"param1		INT		UNSIGNED	NOT NULL,"
			"param2		INT		UNSIGNED	NOT NULL,"
			"param3		INT		UNSIGNED	NOT NULL,"
			"reward		VARCHAR(512) 	CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"server		VARCHAR(512) 	CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"dead_time	INT		UNSIGNED	NOT NULL,"
			"use_max	INT		UNSIGNED	NOT NULL,"
			"platform	VARCHAR(512) 	CHARACTER SET utf8 NOT NULL DEFAULT '0',"
			"use_every	INT		UNSIGNED	NOT NULL DEFAULT '1',"
			"PRIMARY KEY(id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);

	if ( !ExistsColumn( "gift_box_config", "use_every" ) )
	{
		m_db->PExecute( "ALTER TABLE gift_box_config ADD COLUMN use_every INT UNSIGNED NOT NULL DEFAULT '1'" );
	}

	// gift_box_config_backup
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS gift_box_config_backup("
			"id			INT		UNSIGNED	NOT NULL,"
			"param1		INT		UNSIGNED	NOT NULL,"
			"param2		INT		UNSIGNED	NOT NULL,"
			"param3		INT		UNSIGNED	NOT NULL,"
			"reward		VARCHAR(512) 	CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"server		VARCHAR(512) 	CHARACTER SET utf8 NOT NULL DEFAULT '',"
			"dead_time	INT		UNSIGNED	NOT NULL,"
			"use_max	INT		UNSIGNED	NOT NULL,"
			"platform	VARCHAR(512) 	CHARACTER SET utf8 NOT NULL DEFAULT '0',"
			"backup_time INT	UNSIGNED	NOT NULL,"
			"use_every	INT		UNSIGNED	NOT NULL DEFAULT '1',"
			"PRIMARY KEY(id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	);
	// goods_info
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS goods_info ("
			"server_id		SMALLINT	UNSIGNED	NOT NULL,"
			"goods_id			INT		UNSIGNED	NOT NULL,"
			"shop_type			INT		UNSIGNED	NOT NULL,"
			"buy_type_id		INT		UNSIGNED	NOT NULL,"
			"buy_content_id		INT		UNSIGNED	NOT NULL,"
			"buy_count			INT		UNSIGNED	NOT NULL,"
			"cost_type_id		INT		UNSIGNED	NOT NULL,"
			"cost_content_id	INT		UNSIGNED	NOT NULL,"
			"cost_count			INT		UNSIGNED	NOT NULL,"
			"cost_count_old		INT		UNSIGNED	NOT NULL,"
			"status			TINYINT		UNSIGNED	NOT NULL,"
			"limit_day			INT		UNSIGNED	NOT NULL,"
			"sort_idx			INT		UNSIGNED	NOT NULL,"
			"icon_id			VARCHAR(64)	CHARACTER SET utf8 NOT NULL,"
			"goods_name			VARCHAR(64)	CHARACTER SET utf8 NOT NULL,"
			"description	VARCHAR(256) CHARACTER SET utf8 NOT NULL,"
			"platform_goods_id	VARCHAR(64)	CHARACTER SET utf8 NOT NULL,"
			"platform_type		INT		UNSIGNED	NOT NULL,"
			"discount			INT		UNSIGNED	NOT NULL,"
			"role_lv_req		INT		UNSIGNED	NOT NULL,"
			"bonus_buy_count	INT		UNSIGNED	NOT NULL,"
			"bonus_limit		INT		UNSIGNED	NOT NULL,"
			"bonus_reset_type	TINYINT	UNSIGNED	NOT NULL,"
			"diamond_pay		INT		UNSIGNED	NOT NULL,"
			"PRIMARY KEY (server_id, goods_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8;" );

	if ( !ExistsColumn( "goods_info", "bonus_buy_count" ) )
		m_db->PExecute( "ALTER TABLE goods_info ADD COLUMN bonus_buy_count INT UNSIGNED NOT NULL" );
	if ( !ExistsColumn( "goods_info", "bonus_limit" ) )
		m_db->PExecute( "ALTER TABLE goods_info ADD COLUMN bonus_limit INT UNSIGNED NOT NULL" );
	if ( !ExistsColumn( "goods_info", "bonus_reset_type" ) )
		m_db->PExecute( "ALTER TABLE goods_info ADD COLUMN bonus_reset_type TINYINT UNSIGNED NOT NULL" );
	if ( !ExistsColumn( "goods_info", "diamond_pay" ) )
		m_db->PExecute( "ALTER TABLE goods_info ADD COLUMN diamond_pay INT UNSIGNED NOT NULL" );

	// guild_reg
	m_db->PExecute(
		"CREATE TABLE IF NOT EXISTS guild_reg("
			"guild_id		INT		UNSIGNED	NOT NULL AUTO_INCREMENT,"
			"role_id		INT		UNSIGNED	NOT NULL,"
			"create_time	INT		UNSIGNED	NOT NULL,"
			"PRIMARY KEY(guild_id)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=10000;"
	);

	QueryResult* result = m_db->PQuery(
		"select MAX(passport_id) from passport_info"
	);

	if ( !result )
	{
		IME_SQLERROR( "Error Get Max 'passport_id' from passport_info" );
		return false;
	}

	uint64 maxPassportId = result->Fetch()[0].GetUInt64();
	m_maxPassportAutoInc = (uint32)( maxPassportId >> 32 );

	IME_SYSTEM_LOG( "CenterDBCtrl Init", "maxPassportAutoInc=%u", m_maxPassportAutoInc );
	delete result;
	result = NULL;

	result = m_db->PQuery(
		"select MAX(role_id) from re_passport_player where mod(role_id >> 1, 128)=%u", m_byLoginServerId
	);

	if ( !result )
	{
		IME_SQLERROR( "Error Get Max 'role_id' from re_passport_player" );
		return false;
	}
	uint32 maxRoleId = result->Fetch()[0].GetUInt32();
	m_maxRoleAutoInc = std::max( (uint32)( maxRoleId >> 4 ), (uint32)100000 );

	IME_SYSTEM_LOG( "CenterDBCtrl Init", "maxRoleAutoInc=%u", m_maxRoleAutoInc );

	delete result;
	return true;
}

uint32 CenterDBCtrl::GetDBTime()
{
	QueryResult* result = m_db->PQuery( "select unix_timestamp()" );
	if ( result )
	{
		uint32 ret = result->Fetch()[0].GetUInt32();

		delete result;
		return ret;
	}
	else
	{
		IME_SQLERROR( "Error Get DataBase Time" );
	}

	return 0;
}

bool CenterDBCtrl::ExistsPassport( uint64 dwPassport )
{
	QueryResult* result = m_db->PQuery(
		"select passport_id from passport_info where passport_id=%llu", dwPassport
	);
	if ( result )
	{
		delete result;
		return true;
	}
	return false;
}

bool CenterDBCtrl::ExistsPassport( std::string strPassport, uint8 byAuthType, uint32 dwPlatform)
{
	m_db->escape_string( strPassport );

	QueryResult* result = m_db->PQuery(
		"select passport_id from passport_info where passport='%s' and platform=%u and auth_type=%u",
		strPassport.c_str(), dwPlatform, byAuthType );

	if ( result )
	{
		delete result;
		return true;
	}
	return false;
}

bool CenterDBCtrl::ExistsRole( uint32 dwRoleId )
{
	QueryResult* result = m_db->PQuery(
		"select role_id from player_info where role_id=%u", dwRoleId );
	if ( result )
	{
		delete result;
		return true;
	}
	return false;
}
bool CenterDBCtrl::UpdateRelayServerInfo(
			uint32 dwServerId,
			uint8  byType,
			uint32  dwGroup,
			uint32 dwDBTime)
{
	return m_db->PExecute( "insert into relayserver_info "
			"(relay_type, relay_group, server_id, update_time) "
			"values(%u,%u,%u,%u) on duplicate key update "
			"server_id=%u, update_time=%u",
			byType, dwGroup, dwServerId,  dwDBTime,
			dwServerId , dwDBTime
			);
}
bool CenterDBCtrl::UpdateGameServerInfo(
			uint32 			dwServerId,
			std::string		strServerName,
			std::string		strIp,
			std::string		strLocalIp,
			uint32 			dwPort,
			std::string 	strResServerAddr,
			uint32 			dwOnlineNum,
			std::string 	strVersion,
			std::string		strClientVersion,
			std::string		strResVersion,
			uint8			byCanLogin,
			uint8 			byStatus,
			uint32			dwLoginStrategyId,
			uint8           byServerIdentify,
			uint8			byIsTest,
			std::string     strDbName)
{
	m_db->escape_string( strIp );
	m_db->escape_string( strServerName );
	m_db->escape_string( strVersion );
	m_db->escape_string( strClientVersion );
	m_db->escape_string( strResVersion );
	m_db->escape_string( strLocalIp );
	m_db->escape_string( strResServerAddr );
	m_db->escape_string( strDbName );

	// Async
	return m_db->PExecute( "insert into gameserver_info "
			"(server_id, server_name, ip, local_ip, port, version, res_version, res_version_config, online_num, can_login, status, login_strategy_id, res_server_ip, update_time, server_identify,is_test,db_name) "
			"values(%u, '%s', '%s', '%s', %u, '%s', '%s', '%s', %u, %u, %u, %u, '%s', %u, %u,%u,'%s') on duplicate key update "
			"server_name='%s', ip='%s',local_ip='%s', port=%u, version='%s', res_version='%s', res_version_config='%s', "
			"online_num=%u, can_login=%u, status=%u, login_strategy_id=%u, res_server_ip='%s', update_time=%u, server_identify=%u, is_test=%u, db_name='%s'",
			dwServerId,
			strServerName.c_str(), strIp.c_str(), strLocalIp.c_str(), dwPort, strVersion.c_str(), strClientVersion.c_str(), strResVersion.c_str(),
			dwOnlineNum, byCanLogin, byStatus, dwLoginStrategyId, strResServerAddr.c_str(), GetDBTime(), byServerIdentify,byIsTest, strDbName.c_str(),
			strServerName.c_str(), strIp.c_str(), strLocalIp.c_str(), dwPort, strVersion.c_str(), strClientVersion.c_str(), strResVersion.c_str(),
			dwOnlineNum, byCanLogin, byStatus, dwLoginStrategyId, strResServerAddr.c_str(), GetDBTime(), byServerIdentify ,byIsTest, strDbName.c_str());
}
//bool CenterDBCtrl::UpdateBattleserverList(BattleServerDetailContainer& vecBattleServer)
//{
//    SqlQueryBatch batch;
//    for(BattleServerDetailContainer::iterator iter = vecBattleServer.begin();iter != vecBattleServer.end();iter++)
//    {
//    	char sql[4096];
//    	snprintf(sql, 4096, "insert into battleserver_list "
//    			"(relay_id, server_ip,server_port,battle_id,last_update_time,player_num) "
//    			"values(%u, '%s',%u,'%s',%u,%u) on duplicate key update "
//    			"relay_id=%u,server_ip='%s',server_port=%u,battle_id='%s',last_update_time=%u,player_num=%u",
//    			iter->dwRelayServerId,iter->strServerIp.c_str(),iter->dwServerPort,iter->strBattleId.c_str(),iter->dwLastUpdateTime,iter->dwConnectPlayerNum,
//    			iter->dwRelayServerId,iter->strServerIp.c_str(),iter->dwServerPort,iter->strBattleId.c_str(),iter->dwLastUpdateTime,iter->dwConnectPlayerNum);
//    	batch.PushQuery(sql);
////        batch.PushQuery()
//    }
//    return batch.ExecuteDirect( m_db );
////	return true;
//}
bool CenterDBCtrl::DelBattleServer(std::string ip,uint32 dwPort)
{
	return m_db->PExecute("delete from battleserver_list where server_ip ='%s' and server_port = %u", ip.c_str(),dwPort);
}
bool CenterDBCtrl::UpdateBattleserverInfo(uint32 dwRelayId,uint32 dwTotal,uint32 dwRunning,uint32 dwPlayerNum)
{

//	IME_ERROR("%u %u %u",dwRelayId,dwTotal,dwRunning);
	return m_db->PExecute( "insert into battleserver_info "
			"(relay_id, connect_num, running_battle, player_num) "
			"values(%u, %u,%u,%u) on duplicate key update "
			"relay_id=%u, connect_num=%u,running_battle=%u,player_num=%u",
			dwRelayId,dwTotal,dwRunning,dwPlayerNum,
			dwRelayId,dwTotal,dwRunning,dwPlayerNum);

}
bool CenterDBCtrl::GetServerMergeLog( std::map<uint32,uint32>& mapMergeTable )
{
    QueryResult* result = m_db->PQuery("select server_id_origin,server_id_merged from gameserver_merge_log"    );
    if ( result )
    {
        uint64 cnt = result->GetRowCount();
        for (unsigned int i = 0; i < cnt; i++ )
        {
            Field* field = result->Fetch();

            uint32 origin  = field[0].GetUInt32();
            uint32 target  = field[1].GetUInt32();
            mapMergeTable[origin] = target;
            result->NextRow();
        }
        delete result;
    }
    return true;
}
bool CenterDBCtrl::GetMergedRoleIds(std::map<uint32, uint32>& mapRoleOriginSID, uint32 dwServerId)
{
	QueryResult* result = m_db->PQuery("select role_id,server_id_origin from re_passport_info "
			" where server_id=%u server_id_origin!=server_id", dwServerId);
	if (result)
	{
		uint64 cnt = result->GetRowCount();
		for (unsigned int i = 0; i < cnt; i++)
		{
			Field* field = result->Fetch();

			uint32 dwRoleId			= field[0].GetUInt32();
			uint32 dwOriginServerId	= field[1].GetUInt32();

			mapRoleOriginSID[dwRoleId] = dwOriginServerId;
			result->NextRow();
		}
		delete result;
	}
	return true;
}
uint32 CenterDBCtrl::GetServerIdMerged( uint32 dwServerIdOrigin )
{
	uint32 dwId = dwServerIdOrigin;

	while ( true )
	{
		QueryResult* result = m_db->PQuery( "select server_id_merged from gameserver_merge_log where server_id_origin = %u", dwId );

		if ( result )
		{
			dwId = result->Fetch()[0].GetUInt32();
			delete result;
		}
		else
		{
			break;
		}
	}

	return dwId;
}

bool CenterDBCtrl::IsPassportRegistered( std::string strPassport )
{
    m_db->escape_string( strPassport );
    QueryResult* result = m_db->PQuery( "select passport from passport_info where passport=%s", strPassport.c_str() );
    return ( result != NULL );
}

bool CenterDBCtrl::GetMergedListInfo(std::map<uint32, STC_SERVER_MERGE>& out)
{
	QueryResult* result = m_db->PQuery( "SELECT `server_id_origin`,`server_id_merged`,merge_type FROM `gameserver_merge_log`;");
	if (result)
	{
		uint64_t cnt = result->GetRowCount();

		std::map<uint32, STC_SERVER_MERGE> id2MergeMap;
		for (uint64 i = 0; i < cnt; ++i)
		{
			Field* field = result->Fetch();

			STC_SERVER_MERGE stc;
			stc.originid = field[0].GetUInt32();
			stc.mergeid = field[1].GetUInt32();
			stc.mergetype = field[2].GetUInt32();

			id2MergeMap[stc.originid] = stc;

			result->NextRow();
		}
		out.clear();

		for (std::map<uint32, STC_SERVER_MERGE>::iterator iter = id2MergeMap.begin(); iter != id2MergeMap.end(); ++iter)
		{
			STC_SERVER_MERGE stc = iter->second;
			while (true)
			{
				std::map<uint32, STC_SERVER_MERGE>::iterator iter_next = id2MergeMap.find(stc.mergeid);
				if (iter_next == id2MergeMap.end())          
					break;
				stc.mergeid = iter_next->second.mergeid;
				stc.mergetype = iter_next->second.mergetype;			
			}
			out[iter->first] = stc;
		}
		delete result;
		return true;
	}
	return false;
}

uint32 CenterDBCtrl::GetServerIdOriginByRoleId(uint32 roleId)
{
	uint32 dwId = 0;
	QueryResult* result = m_db->PQuery( "select server_id_origin from re_passport_player where role_id = %u", roleId );

	if ( result )
	{
		Field* field = result->Fetch();
		if (field)
			dwId = field[0].GetUInt32();
		delete result;
	}
	return dwId;
}

LoginResultType CenterDBCtrl::ValidateAuthAccount(
			std::string 	strPassport,
			std::string 	strPwd,
			uint32			dwPlatform,
			uint64&			odwPassportId )
{
	m_db->escape_string( strPassport );

	QueryResult* result = m_db->PQuery(
		"select passport_id, pwd from passport_info where passport='%s' and platform=%u and auth_type=%u",
		strPassport.c_str(), dwPlatform, E_LOGIN_AUTH_TYPE_ACCOUNT
	);

	if ( result )
	{
		Field* field = result->Fetch();
		uint64 dwPassportId 	= field[0].GetUInt64();
		std::string strTruePwd	= field[1].GetString();
		delete result;

		if ( strPwd == strTruePwd )
		{
			odwPassportId = dwPassportId;
			return E_LOGIN_RESULT_TYPE_OK;
		}
		else
		{
			return E_LOGIN_RESULT_TYPE_WRONG_PWD;
		}
	}
	else
	{
		return E_LOGIN_RESULT_TYPE_NOT_FOUND;
	}
}

LoginResultType CenterDBCtrl::ValidateAuthPlatform(
			std::string		strPlatformToken,
			std::string		strUid,
			std::string 	strDeviceToken,
			uint32			dwPlatform,
			std::string		strRegIp,
			std::string		strRegDevice,
			std::string		strRegDeviceType,
			std::string		strOpenUdid,
			std::string		strAppleUdid,
			uint64&			odwPassportId )
{
	m_db->escape_string( strPlatformToken );
	m_db->escape_string( strUid );
	m_db->escape_string( strDeviceToken );

	QueryResult* result = m_db->PQuery(
		"select passport_id from passport_info where passport='%s' and platform=%u and auth_type=%u",
		strPlatformToken.c_str(), dwPlatform, E_LOGIN_AUTH_TYPE_PLATFORM
	);

	if ( result == NULL)
	{
		/*
		int startPlatform[] = {5100};
		int endPlatform[] = {5110};
		int len = GET_ARRAY_LENGTH(startPlatform) > GET_ARRAY_LENGTH(endPlatform) ? GET_ARRAY_LENGTH(endPlatform) : GET_ARRAY_LENGTH(startPlatform);
		for (int i = 0; i < len; i++)
		{
			if (wPlatform >= startPlatform[i] && wPlatform <= endPlatform[i])
			{
				result = m_db->PQuery(
						"select passport_id,platform from passport_info where passport='%s' and (platform >= %u and platform <= %u) and auth_type=%u",
						strPlatformToken.c_str(), startPlatform[i], endPlatform[i], E_LOGIN_AUTH_TYPE_PLATFORM
						);
				break;
			}
		}*/
	}

	if ( result )
	{
		Field* field = result->Fetch();
		uint64 dwPassportId = field[0].GetUInt64();
		delete result;

		odwPassportId = dwPassportId;
		return E_LOGIN_RESULT_TYPE_OK;
	}
	else
	{
		m_db->escape_string( strRegIp );
		m_db->escape_string( strRegDevice );
		m_db->escape_string( strRegDeviceType );

		if ( InsertPassportInfo( strPlatformToken, "", "", strUid, strDeviceToken, dwPlatform, E_LOGIN_AUTH_TYPE_PLATFORM,
				GetDBTime(), 0, strRegIp.c_str(), strRegDevice.c_str(), strRegDeviceType.c_str(), strOpenUdid, strAppleUdid, odwPassportId, E_CDK_STATE_NO_NEED_VERIFY ) )
		{
			return E_LOGIN_RESULT_TYPE_OK;
		}
		else
		{
			return E_LOGIN_RESULT_ERROR;
		}
	}
}

LoginResultType CenterDBCtrl::ValidateAuthFast(
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
		uint8           byCDKStatus)
{
	m_db->escape_string( strUid );
	m_db->escape_string( strDeviceToken );

	QueryResult* result = NULL;

//	if ( byIOSVersion < E_IOS_VERSION_7 )
	if(false)
	{
		result = m_db->PQuery(
			"select passport_id from passport_info where passport='%s' and platform=%u and auth_type=%u",
			strUid.c_str(), dwPlatform, E_LOGIN_AUTH_TYPE_FAST
		);
	}
	else
	{
		do
		{
			if ( strOpenUdid.length() )
			{
				result =  m_db->PQuery(
					"select passport_id from passport_info where open_udid='%s' and platform=%u and auth_type=%u",
					strOpenUdid.c_str(), dwPlatform, E_LOGIN_AUTH_TYPE_FAST
				);

				if ( result ) break;
			}

//			if ( strAppleUdid.length() )
//			{
//				result = m_db->PQuery(
//					"select passport_id from passport_info where apple_udid='%s' and platform=%u and auth_type=%u",
//					strAppleUdid.c_str(), wPlatform, E_LOGIN_AUTH_TYPE_FAST
//				);
//
//				if ( result ) break;
//			}

		} while (0);
	}

	if ( result )
	{
		Field* field = result->Fetch();
		uint64 dwPassport = field[0].GetUInt64();
		delete result;

		if ( strOpenUdid.length() )
		{
			m_db->PExecute("update passport_info set open_udid='%s' where passport_id=%llu",
					strOpenUdid.c_str(), dwPassport );
		}

		if ( strAppleUdid.length() )
		{
			m_db->PExecute("update passport_info set apple_udid='%s' where passport_id=%llu",
					strAppleUdid.c_str(), dwPassport );
		}

		odwPassportId = dwPassport;
		IME_DEBUG("Fast login success, PassportId=%lu",odwPassportId);
		return E_LOGIN_RESULT_TYPE_OK;
	}
	else
	{
		if ( InsertPassportInfo( strUid, "", "", strUid, strDeviceToken, dwPlatform, E_LOGIN_AUTH_TYPE_FAST,
				GetDBTime(), 0, strRegIp, strRegDevice, strRegDeviceType, strOpenUdid, strAppleUdid, odwPassportId, byCDKStatus == 0 ? E_CDK_STATE_NO_NEED_VERIFY : E_CDK_STATE_NOT_VERIFIED ) )
		{
			IME_DEBUG("Fast login success, PassportId=%lu",odwPassportId);
			return E_LOGIN_RESULT_TYPE_OK;
		}
		else
		{
			return E_LOGIN_RESULT_ERROR;
		}
	}
}

bool CenterDBCtrl::ModifyPassportAndPassword( uint64 dwPassportId, std::string dwPassport, std::string strPwd )
{
    m_db->escape_string( dwPassport );
    m_db->escape_string( strPwd );

    return m_db->PExecute( "update passport_info set passport=%s, pwd=%s, auth_type=%u where passport_id=%llu",
            dwPassport.c_str(), strPwd.c_str(), E_LOGIN_AUTH_TYPE_ACCOUNT, dwPassportId );
}

bool CenterDBCtrl::ModifyPassword( uint64 dwPassportId, std::string strNewPwd )
{
	m_db->escape_string( strNewPwd );

	return m_db->PExecute(
		"update passport_info set pwd='%s' where passport_id=%llu and auth_type=%u",
		strNewPwd.c_str(), dwPassportId, E_LOGIN_AUTH_TYPE_ACCOUNT
	);
}

bool CenterDBCtrl::ModifyPassword( uint32 dwRoleId, std::string strOldPwd, std::string strNewPwd )
{
	m_db->escape_string( strOldPwd );
	m_db->escape_string( strNewPwd );

	uint64 dwPassportId = GetPassportId( dwRoleId );
	if ( dwPassportId == 0 ) return false;

	QueryResult* result = m_db->PQuery(
		"select pwd, auth_type from passport_info where passport_id=%llu", dwPassportId
	);

	std::string strPwd;
	uint8		byAuthType;

	if ( result )
	{
		Field* field = result->Fetch();

		strPwd		= field[0].GetString();
		byAuthType	= field[1].GetUInt8();

		delete result;

		if ( byAuthType != E_LOGIN_AUTH_TYPE_ACCOUNT ) return false;
		if ( strPwd != strOldPwd ) return false;
	}
	else
	{
		return false;
	}

	return m_db->PExecute(
		"update passport_info set pwd='%s' where passport_id=%llu",
		strNewPwd.c_str(), dwPassportId
	);
}

bool CenterDBCtrl::UpdateCDKStatus( uint64 dwPassportId, uint8 byCDKStatus )
{
    return m_db->PExecute(
            "update passport_info set cdk_status='%u' where passport_id=%llu",
            byCDKStatus, dwPassportId
            );
}

bool CenterDBCtrl::InsertPassportInfo(
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
            uint8           byCDKStatus )
{
	if ( m_bReadOnly )
	{
		IME_SQLERROR( "ReadOnly Is On" );
		return false;
	}

	m_db->escape_string( strPassport );
	m_db->escape_string( strPwd );
	m_db->escape_string( strMail );
	m_db->escape_string( strUid );
	m_db->escape_string( strToken );
	m_db->escape_string( strCreateIp );
	m_db->escape_string( strCreateDevice );
	m_db->escape_string( strCreateDeviceType );

	odwPassportId = NextPassportId( dwPlatform );

	bool ret = m_db->PExecute(
		"insert into passport_info(passport_id,passport,pwd,mail,uid,token,platform,auth_type,create_time,gm_auth,reg_ip,reg_device,reg_device_type,open_udid,apple_udid,cdk_status) "
		"values(%llu,'%s','%s','%s','%s','%s',%u,%u,%u,%u,'%s','%s','%s','%s','%s','%u')",
		odwPassportId, strPassport.c_str(), strPwd.c_str(), strMail.c_str(), strUid.c_str(), strToken.c_str(),
		dwPlatform, byAuthType, dwCreateTime, byGmAuth, strCreateIp.c_str(),
		strCreateDevice.c_str(), strCreateDeviceType.c_str(), strOpenUdid.c_str(), strAppleUdid.c_str(), byCDKStatus
	);

	return ret;
}

bool CenterDBCtrl::GetPassportInfo( uint64 dwPassportId, STC_PASSPORT_INFO& stcInfo )
{
	QueryResult* result = m_db->PQuery(
		"select passport, pwd, mail, uid, token, platform, auth_type, create_time, gm_auth, reg_ip, reg_device, reg_device_type, "
		"last_login_time, last_login_server, open_udid, apple_udid, token_ip, token_time, cdk_status from passport_info where passport_id=%llu",
		dwPassportId
	);

	if ( result )
	{
		Field* field = result->Fetch();
		int i = 0;

		stcInfo.ddwPassportId	= dwPassportId;
		stcInfo.strPassport		= field[i++].GetString();
		stcInfo.strPwd			= field[i++].GetString();
		stcInfo.strMail			= field[i++].GetString();
		stcInfo.strUid			= field[i++].GetString();
		stcInfo.strToken		= field[i++].GetString();
		stcInfo.dwPlatform		= field[i++].GetUInt32();
		stcInfo.byAuthType		= field[i++].GetUInt8();
		stcInfo.dwCreateTime	= field[i++].GetUInt32();
		stcInfo.byGmAuth		= field[i++].GetUInt8();
		stcInfo.strCreateIp		= field[i++].GetString();
		stcInfo.strCreateDevice	= field[i++].GetString();
		stcInfo.strCreateDeviceType = field[i++].GetString();
		stcInfo.dwLastLoginTime	= field[i++].GetUInt32();
		stcInfo.dwLastLoginServerId = field[i++].GetUInt32();
		stcInfo.strOpenUdid		= field[i++].GetString();
		stcInfo.strAppleUdid	= field[i++].GetString();
		stcInfo.strTokenIp		= field[i++].GetString();
		stcInfo.dwTokenTime		= field[i++].GetUInt32();
		stcInfo.byCDKStatus     = field[i++].GetUInt8();

		delete result;
		return true;
	}
	else
	{
		return false;
	}
}

uint64 CenterDBCtrl::GetPassportId( uint32 dwRoleId )
{
	QueryResult* result = m_db->PQuery(
		"select passport_id from re_passport_player where role_id=%u", dwRoleId
	);

	if ( result )
	{
		uint64 ret = result->Fetch()[0].GetUInt64();
		delete result;

		return ret;
	}
	return 0;
}

bool CenterDBCtrl::GetLoginStrategy(
		uint32				dwStrategyId,
		STC_LOGIN_STRATEGY& oStrategy )
{
	QueryResult* result = m_db->PQuery(
		"select condition_id, type, value, is_not from login_strategy where strategy_id=%u order by condition_id", dwStrategyId );
	if ( result )
	{
		oStrategy.vvConditions.clear();

		int cond = -1;
		uint64 cnt = result->GetRowCount();
		for (unsigned int i = 0; i < cnt; i++ )
		{
			Field* field = result->Fetch();

			if ( (int)field[0].GetUInt8() != cond )
			{
				std::vector<STC_LOGIN_STRATEGY_CONDITION> vCond;
				oStrategy.vvConditions.push_back( vCond );

				cond = (int)field[0].GetUInt8();
			}

			STC_LOGIN_STRATEGY_CONDITION t;
			t.byType	= field[1].GetUInt8();
			t.strValue	= field[2].GetString();
			t.bIsNot	= field[3].GetBool();

			oStrategy.vvConditions.back().push_back( t );

			result->NextRow();
		}

		delete result;
		return true;
	}
	else
	{
		return false;
	}
}

bool CenterDBCtrl::GetOrUpdateRelayServerStatus(
		RelayServerStatusContainer& mapRelayServer)
{
	QueryResult *result = m_db->Query(
			"select relay_type, relay_group, server_id, update_time, merge_group from relayserver_info" );
	if( result)
	{
		uint64 u64Cnt = result->GetRowCount();
		for(uint64 i = 0; i < u64Cnt; ++i)
		{
			Field *fields = result->Fetch();

			STC_RELAY_SERVER_STATUS stcServer;
			stcServer.byType 		= fields[0].GetUInt8();
			stcServer.dwGroup		= fields[1].GetUInt32();
			stcServer.dwServerId	= fields[2].GetUInt16();
			stcServer.dwUpdateTime	= fields[3].GetUInt32();
			stcServer.byMergeGroup	= fields[4].GetUInt8();

			RelayServerStatusContainer::iterator it = mapRelayServer.find( MK(stcServer.byType,stcServer.dwGroup) );

			if ( it != mapRelayServer.end() )
			{
				uint32 dwPreUpdateTime = it->second.dwUpdateTime;
				it->second = stcServer;
				it->second.bIsAlive = ( dwPreUpdateTime != it->second.dwUpdateTime );;
				if ( !it->second.bIsAlive )
				{

					UpdateClosedGameServer( stcServer.dwServerId );
				}
			}
			else
			{
				mapRelayServer.insert( std::make_pair(  MK(stcServer.byType,stcServer.dwGroup), stcServer ) );
			}

			result->NextRow();
		}
		delete result;
		for( RelayServerStatusContainer::iterator iterMaster = mapRelayServer.begin(); iterMaster != mapRelayServer.end(); iterMaster++ )
		{
			RelayServerStatusContainer::iterator iter = iterMaster;
			while(iter->second.byMergeGroup !=0)
			{
				iter = mapRelayServer.find(MK(iterMaster->second.byType, iterMaster->second.dwGroup));
				if( iter == mapRelayServer.end())
				{
//					iterMaster->second.dwServerId = 0;
					IME_SYSTEM_ERROR( "GetRelayServer", "The RelayServer After Merge Not Found, Type=%u , Group =%u"
							, iterMaster->second.byType,iterMaster->second.dwGroup );
					break;
				}
				if( iter == iterMaster)
				{
					IME_SYSTEM_ERROR( "GetRelayServer"," There is a loop when merge Relay Server");
					break;
				}
			}
			if( iter == mapRelayServer.end()) continue;
			iterMaster->second.dwServerId = iter->second.dwServerId;
		}


	}
	return true;
}

bool CenterDBCtrl::GetOrUpdateGameServerStatus(
		std::map<uint32, STC_SERVER_STATUS>& mapServer )
{
	for ( std::map<uint32, STC_SERVER_STATUS>::iterator it = mapServer.begin();
			it != mapServer.end(); ++it )
	{
		it->second.bIsAlive = false;
	}

	QueryResult *result = m_db->Query(
		"select server_id, server_name, ip,local_ip,port,version,res_version,res_version_config,online_num,can_login,status,"
		"login_strategy_id, res_server_ip, update_time, is_test, activate_req, server_identify, db_name from gameserver_info" );

	if ( result )
	{
		uint64 u64Cnt = result->GetRowCount();
		for(uint64 i = 0; i < u64Cnt; ++i)
		{
			Field *fields = result->Fetch();

			STC_SERVER_STATUS stcServer;

			stcServer.dwServerId 	= fields[0].GetUInt16();
			stcServer.strServerName	= fields[1].GetString();
			stcServer.strIp			= fields[2].GetString();
			stcServer.strLocalIp	= fields[3].GetString();
			stcServer.dwPort		= fields[4].GetUInt32();
			stcServer.strVersion	= fields[5].GetString();
			stcServer.strClientVer	= fields[6].GetString();
			stcServer.strResVer		= fields[7].GetString();
			stcServer.dwOnlineNum 	= fields[8].GetUInt32();
			stcServer.byCanLogin	= fields[9].GetUInt8();
			stcServer.byStatus		= fields[10].GetUInt8();
			stcServer.dwLoginStrategy = fields[11].GetUInt32();
			stcServer.strResServerAddr = fields[12].GetString();
			stcServer.dwLastUpdateTime = fields[13].GetUInt32();
			stcServer.bIsTest		= fields[14].GetBool();
			stcServer.byActivateReq	= fields[15].GetUInt8();
			stcServer.byServerIdentify = fields[16].GetUInt8();
			stcServer.strDbName     = fields[17].GetString();
			//
			std::vector<std::string> vs;
			CUtil::StrSplit(stcServer.strClientVer,"-",vs);
			if(vs.size() >=1) stcServer.strClientVer = vs[0];
			if(vs.size() >=2) stcServer.strClientResVer = vs[1];
			///
			std::map<uint32, STC_SERVER_STATUS>::iterator it = mapServer.find( stcServer.dwServerId );

			if ( it != mapServer.end() )
			{
				uint32 dwPreUpdateTime = it->second.dwLastUpdateTime;
				it->second = stcServer;
				it->second.bIsAlive = ( dwPreUpdateTime != it->second.dwLastUpdateTime );

				if ( !it->second.bIsAlive )
				{
					UpdateClosedGameServer( stcServer.dwServerId );
				}
			}
			else
			{
				stcServer.bIsAlive = false;
				mapServer.insert( std::make_pair( stcServer.dwServerId, stcServer ) );
			}

			result->NextRow();
		}

		delete result;

		//优化代码，不再对合服的数据进行多次mysql query,改成只查一次 2016-7-23 yijian
		std::map<uint32, STC_SERVER_MERGE> id2MergeMap;
		GetMergedListInfo(id2MergeMap);

		for ( std::map<uint32, STC_SERVER_STATUS>::iterator it = mapServer.begin();
				it != mapServer.end(); ++it )
		{
			if ( !it->second.bIsAlive )
			{
				// check whether this server has been merged
//				uint32 dwServerIdMerged = GetServerIdMerged( it->second.dwServerId );
				uint32 dwServerIdMerged = it->second.dwServerId;
			    uint32 dwMergedType = 0;

				std::map<uint32, STC_SERVER_MERGE>::iterator iter_merge = id2MergeMap.find(it->second.dwServerId);
				if (iter_merge != id2MergeMap.end())
				{
					dwServerIdMerged = iter_merge->second.mergeid;
					dwMergedType = iter_merge->second.mergetype;
				}
				if ( dwServerIdMerged != it->second.dwServerId )
				{
					std::map<uint32, STC_SERVER_STATUS>::iterator itMaster = mapServer.find( dwServerIdMerged );
					if ( itMaster != mapServer.end() )
					{
//						IME_SYSTEM_LOG( "GetOrUpdateGameServerStatus", "Replace With MasterInfo, slaveId=%u, masterId=%u,"
//								"slaveStra=%u, masterStra=%u", it->second.dwServerId, itMaster->second.dwServerId,
//								it->second.dwLoginStrategy, itMaster->second.dwLoginStrategy );
//
						// same entrance condition as its master server
						it->second.strIp		= itMaster->second.strIp;
						it->second.dwPort		= itMaster->second.dwPort;
						it->second.strVersion	= itMaster->second.strVersion;
						it->second.strClientVer	= itMaster->second.strClientVer;
						it->second.strResVer 	= itMaster->second.strResVer;

						it->second.dwOnlineNum 	= 0;
						it->second.byCanLogin	= itMaster->second.byCanLogin;
						it->second.byStatus		= itMaster->second.byStatus;
						if (dwMergedType == 0)
							it->second.dwLoginStrategy 	= itMaster->second.dwLoginStrategy;
						it->second.strResServerAddr = itMaster->second.strResServerAddr;
						it->second.bIsAlive 	= itMaster->second.bIsAlive;
						it->second.bIsTest		= itMaster->second.bIsTest;
						it->second.byActivateReq= itMaster->second.byActivateReq;
						it->second.byServerIdentify = itMaster->second.byServerIdentify;
						it->second.strDbName = itMaster->second.strDbName;
					}
				}
			}
		}
	}
	return true;
}

bool CenterDBCtrl::UpdateClosedGameServer( uint32 dwServerId )
{
	QueryResult* result = m_db->PQuery(
		"select online_num from gameserver_info where server_id=%u", dwServerId );

	if ( result )
	{
		uint16 wOnlineNum = result->Fetch()[0].GetUInt16();
		delete result;

		if ( wOnlineNum )
		{
			m_db->PExecute("update gameserver_info set online_num=%u where server_id=%u", 0, dwServerId );

//			uint32 dwTime = GetDBTime();
//			// slow
//			m_db->PExecute("update role_login_info set logout_time=%u where logout_time=0 and role_id in"
//					" ( select role_id from re_passport_player where server_id=%u )", dwTime, dwServerId );
		}

		return true;
	}
	else
	{
		return false;
	}
}

uint32 CenterDBCtrl::GetRoleId( uint64 dwPassportId, uint32 dwServerIdOrigin )
{
	QueryResult* result = m_db->PQuery(
		"select role_id from re_passport_player where passport_id=%llu and server_id_origin=%u",
		dwPassportId, dwServerIdOrigin
	);
	if ( result )
	{
		Field* field = result->Fetch();
		uint32 dwRoleId = field[0].GetUInt32();
		delete result;
		return dwRoleId;
	}
	return 0;
}

uint32 CenterDBCtrl::GetOrInsertRoleId( uint64 dwPassportId, uint32 dwServerIdOrigin )
{
	uint32 dwRoleId = GetRoleId( dwPassportId, dwServerIdOrigin );
	if ( dwRoleId )
	{
		return dwRoleId;
	}
	else
	{
		dwRoleId = NextRoleId();

		if (m_db->PExecute( "insert into re_passport_player(role_id, passport_id, server_id, server_id_origin, create_time) values(%u, %llu, %u, %u, %u)",
				dwRoleId, dwPassportId, GetServerIdMerged(dwServerIdOrigin), dwServerIdOrigin, GetDBTime() ) )
		{
			return dwRoleId;
		}
		else
		{
			IME_ERROR( "Error When Insert Into re_passport_player" );
			return 0;
		}
	}
}

bool CenterDBCtrl::IsRoleForbid( uint32 dwRole )
{
	return GetRoleStatus( dwRole ) == E_ROLE_STATUS_FORBID;
}

bool CenterDBCtrl::RegisterPassport(
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
			uint8           byCDKStatus )
{
	m_db->escape_string( strPassport );

	QueryResult* result = m_db->PQuery("select passport_id from passport_info where passport='%s' and platform=%u and auth_type=%u",
			strPassport.c_str(), dwPlatform, E_LOGIN_AUTH_TYPE_ACCOUNT );

	if ( result )
	{
		delete result;
		return false;
	}
	else
	{
		m_db->escape_string( strPwd );
		m_db->escape_string( strMail );
		m_db->escape_string( strUid );
		m_db->escape_string( strToken );
		m_db->escape_string( strRegIp );
		m_db->escape_string( strRegDevice );
		m_db->escape_string( strRegDeviceType );

		uint64 dwPassportId;

		return InsertPassportInfo( strPassport, strPwd, strMail, strUid, strToken, dwPlatform, E_LOGIN_AUTH_TYPE_ACCOUNT,
				GetDBTime(), 0, strRegIp, strRegDevice, strRegDeviceType, strOpenUdid, strAppleUdid, dwPassportId, byCDKStatus == 0 ? E_CDK_STATE_NO_NEED_VERIFY : E_CDK_STATE_NOT_VERIFIED);
	}
}

bool CenterDBCtrl::InsertOrUpdateRoleInfo(
        uint32          dwRoleId,
        std::string     strRoleName,
        uint8           byGmAuth,
        uint32          dwProgress,
        uint32          dwLevel,
        uint32          dwGold,
        uint32          dwDiamond,
        uint32          dwCurStage,
        uint32          dwCurTrain,
        uint32          dwVipLevel,
        uint32          dwVipExp,
        uint32          dwStamina,
        uint32          dwEnergy,
        uint32          dwMainQuestId,
        uint32          dwDiamondPay,
		uint32			dwCreateTime
	)
{
	if ( dwRoleId & 1 ) return true;

	m_db->escape_string( strRoleName );
	// Async
	return m_db->PExecute( "insert into player_info( role_id, name, gm_auth, status, progress, level, "
		"gold, diamond, cur_stage, cur_train, vip_level, vip_exp, stamina, energy, quest, diamond_pay,create_time ) "
		"values( %u, '%s', %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u ,%u) on duplicate key update "
		"name='%s', progress=%u, level=%u, gold=%u, diamond=%u, cur_stage=%u, cur_train=%u, vip_level=%u, "
		"vip_exp=%u, stamina=%u, energy=%u, quest=%u, diamond_pay=%u,create_time=%u",
		dwRoleId,
		strRoleName.c_str(), byGmAuth, 0, dwProgress, dwLevel, dwGold, dwDiamond, dwCurStage, dwCurTrain, dwVipLevel, dwVipExp,
		dwStamina, dwEnergy, dwMainQuestId, dwDiamondPay,dwCreateTime,
		strRoleName.c_str(), dwProgress, dwLevel, dwGold, dwDiamond, dwCurStage, dwCurTrain, dwVipLevel, dwVipExp,
		dwStamina, dwEnergy, dwMainQuestId, dwDiamondPay,dwCreateTime );
}

bool CenterDBCtrl::UpdatePassportGmAuth(
		uint64			dwPassportId,
		uint8			byGmAuth )
{
	return m_db->PExecute("update passport_info set gm_auth=%u where passport_id=%llu", byGmAuth, dwPassportId );
}

bool CenterDBCtrl::UpdateRoleGmAuth(
		uint32			dwRoleId,
		uint8			byGmAuth )
{
	return m_db->PExecute("update player_info set gm_auth=%u where role_id=%u", byGmAuth, dwRoleId );
}

uint32 CenterDBCtrl::InsertLoginInfo(
			uint32 			dwRoleId,
			std::string		strLoginIp,
			std::string		strLoginDevice,
			std::string		strLoginDeviceType )
{
	if ( dwRoleId & 1 ) return 0;

	m_db->escape_string( strLoginIp );
	m_db->escape_string( strLoginDevice );
	m_db->escape_string( strLoginDeviceType );

	uint32 dwTime = GetDBTime();

	// Async
	m_db->PExecute( "insert into player_info(role_id, last_login_time) values(%u, %u) "
			"on duplicate key update last_login_time=%u",
			dwRoleId, dwTime, dwTime );

	// Async
	m_db->PExecute( "update passport_info a inner join re_passport_player b on (a.passport_id=b.passport_id) "
			"set a.last_login_time=%u where b.role_id=%u",
			dwTime, dwRoleId );

	m_db->PExecute(
		"insert into role_login_info(role_id, login_time, logout_time, login_ip, login_device, login_device_type) "
		"values(%u, %u, %u, '%s', '%s', '%s')",
		dwRoleId, GetDBTime(), 0, strLoginIp.c_str(), strLoginDevice.c_str(), strLoginDeviceType.c_str()
	);

	QueryResult* result = m_db->PQuery( "select LAST_INSERT_ID()" );
	if ( result )
	{
		uint32 ret = result->Fetch()[0].GetUInt32();
		delete result;

		return ret;
	}
	else
	{
		IME_SQLERROR("Error When Insert Login Info");
		return 0;
	}
}

bool CenterDBCtrl::InsertLogoutInfo( uint32 dwAutoId )
{
	if ( dwAutoId == 0 ) return false;

	return m_db->PExecute(
		"update role_login_info set logout_time=%u where auto_id=%u",
		GetDBTime(), dwAutoId
	);
}

bool CenterDBCtrl::GetUnhandledCharge(
		std::list<STC_CHARGE_INFO>& vCharges )
{
	QueryResult* result = m_db->PQuery(
		"select auto_id, role_id, goods_id, goods_quantity, addition2, platform, addition5 from charge_info where state=%u",
		E_CHARGE_STATE_PAYED
	);

	vCharges.clear();

	if ( result )
	{
		uint64 cnt = result->GetRowCount();
		for (unsigned int i = 0; i < cnt; i++ )
		{
			Field* field = result->Fetch();

			STC_CHARGE_INFO ci;
			ci.dwAutoId		= field[0].GetUInt32();
			ci.dwRoleId		= field[1].GetUInt32();
			ci.dwGoodsId	= field[2].GetUInt32();
			ci.dwGoodsQuantity = field[3].GetUInt32();
			ci.strAddition2	= field[4].GetString();
			ci.dwPlatform	= field[5].GetUInt32();
			ci.strAddition5 = field[6].GetString();

			vCharges.push_back( ci );

			result->NextRow();
		}

		delete result;
	}

	return true;
}

bool CenterDBCtrl::ChargeHandled(
			uint32		dwAutoId,
			uint32		dwDiamondValue,
			std::string strIp,
			std::string strDevice,
			std::string strDeviceType,
			std::string strDeviceUid,
			uint32		dwDiamondPay )
{
	m_db->escape_string( strIp );
	m_db->escape_string( strDevice );
	m_db->escape_string( strDeviceType );
	m_db->escape_string( strDeviceUid );

	return m_db->PExecute(
		"update charge_info set virtual_value=%u, state=%u, distribute_time=%u, payment_ip='%s',"
		"payment_device='%s', payment_device_type='%s', payment_device_uid='%s', diamond_pay=%u where auto_id=%u",
		dwDiamondValue, E_CHARGE_STATE_DISTRIBUTED, GetDBTime(), strIp.c_str(), strDevice.c_str(), strDeviceType.c_str(),
		strDeviceUid.c_str(), dwDiamondPay, dwAutoId
	);

	return false;
}

uint32 CenterDBCtrl::CreateCharge(
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
		std::string	strClientOrderId )
{
	m_db->escape_string( strCurrency );
	m_db->escape_string( strInnerOrderId );
	m_db->escape_string( strPlatformOrderId );
	m_db->escape_string( strPlatformAccount );
	m_db->escape_string( strClientOrderId );

	bool succ = m_db->PExecute(
		"insert into charge_info(role_id, goods_id, goods_quantity, currency, value, type, inner_order_id,"
		"platform_order_id, platform_account_id, platform, platform_payment_type, payment_time, client_order_id) "
		"values(%u, %u, %u, '%s', %u, 0, '%s', '%s', '%s', %u, %u, %u, '%s')", dwRoleId, dwGoodsId, dwGoodsQty,
		strCurrency.c_str(), dwValue, strInnerOrderId.c_str(), strPlatformOrderId.c_str(), strPlatformAccount.c_str(),
		dwPlatform, wPaymentType, dwPaymentTime, strClientOrderId.c_str()  );

	if ( succ )
	{
		QueryResult* result = m_db->PQuery( "select LAST_INSERT_ID()" );
		if ( result )
		{
			uint32 ret = result->Fetch()[0].GetUInt32();
			delete result;

			return ret;
		}
		else
		{
			IME_SQLERROR("Error When Get Auto Id Of charge_info");
			return 0;
		}
	}
	else
	{
		IME_SQLERROR("Error When Insert charge_info");
		return 0;
	}
}

bool CenterDBCtrl::HasCharge( uint32 dwPlatform, std::string strPlatformOrderId )
{
	m_db->escape_string( strPlatformOrderId );

	QueryResult* result = m_db->PQuery(
		"select auto_id from charge_info where platform=%u and platform_order_id='%s'",
		dwPlatform, strPlatformOrderId.c_str()
	);

	if ( result )
	{
		delete result;
		return true;
	}
	return false;
}

uint32 CenterDBCtrl::InsertPurchaseInfo(
			uint32			dwRoleId,
			uint32			dwGoodsId,
			uint32			dwGoodsQuantity,
			uint32			dwValue,
			uint32			dwDiamondPaidUse,
			uint32			dwTime )
{
	return m_db->PExecute(
		"insert into purchase_info(role_id, goods_id, goods_quantity, value, diamond_paid_use, time) "
		"values(%u, %u, %u, %u, %u, %u)", dwRoleId, dwGoodsId, dwGoodsQuantity, dwValue, dwDiamondPaidUse, dwTime
	);
}

bool CenterDBCtrl::SetChargeStatus( std::vector< uint32 >& vecRoleId, uint32 dwStartTime, uint32 dwEndTime, std::string& strErrMsg )
{
    if ( dwEndTime < dwStartTime )
    {
        IME_SYSTEM_ERROR("SetChargeStatus", "Time Error" );
        strErrMsg = "Time Error";
        return false;
    }
    bool ret = false;
    for (unsigned int i = 0; i != vecRoleId.size(); ++i )
    {

         ret = m_db->PExecute(
            "update charge_info set state=%u where role_id=%u AND state = %u and distribute_time >= %u and distribute_time <= %u",
            E_CHARGE_STATE_PAYED, vecRoleId[i], E_CHARGE_STATE_DISTRIBUTED, dwStartTime, dwEndTime
        );
         if ( !ret )
         {
             IME_SYSTEM_ERROR("SetChargeStatus"," SetChargeStatus Error , RoleId = %u ", vecRoleId[i]);
             std::stringstream ss;
             ss << "<" << vecRoleId[i] << ">";
             strErrMsg = ss.str();
             continue;
         }
    }
    return true;
}

std::string CenterDBCtrl::GetNotice( NoticeUseType eUseType, NoticeConditionType eCondType, uint32 dwCondValue )
{
	QueryResult* result = m_db->PQuery(
		"select content, start_time, end_time from notice_info_v2 where use_type=%u and condition_type=%u and condition_value=%u",
		(uint8)eUseType, (uint8)eCondType, dwCondValue
	);

	std::string ret = "";
	uint32 dwCurTs = GetDBTime();

	if ( result )
	{
		uint64 cnt = result->GetRowCount();
		for (unsigned int i = 0; i < cnt; i++ )
		{
			Field* field = result->Fetch();

			std::string content = field[0].GetString();
			uint32 dwStartTs 	= field[1].GetUInt32();
			uint32 dwEndTs		= field[2].GetUInt32();

			if ( ( dwStartTs == 0 || dwCurTs >= dwStartTs ) &&
				 ( dwEndTs == 0 || dwCurTs <= dwEndTs ) )
			{
				ret.append( content );
			}
			result->NextRow();
		}
		delete result;
	}
	return ret;
}

bool CenterDBCtrl::GetGameNotice(uint32 dwServId, std::vector<LoginDBNoticeInfo> & vecNotice)
{
// 这是查询所有的可用公告，会形成冗余数据，所以不用
//	QueryResult* result = m_db->PQuery(
//		"select content, start_time, end_time, condition_type, condition_value, auto_id from notice_info_v2 "
//			" where (end_time=0 or end_time>unix_timestamp())"
//			" use_type=%u and ((condition_type=%u and condition_value=%u) "
//			" or condition_type=%u) order by auto_id",
//			(uint8)E_NOTICE_USE_TYPE_GAME, (uint8)E_NOTICE_CONDITION_TYPE_SERVER_ID, dwServId,
//		E_NOTICE_CONDITION_TYPE_PLATFORM);
	QueryResult* result;

	//优先查询对于该服务器的定向公告，如果没有则查询平台公告
	result = m_db->PQuery(
		"select content, start_time, end_time, condition_type, condition_value, auto_id from notice_info_v2 "
			" where (end_time=0 or end_time>unix_timestamp()) and"
			" use_type=%u and (condition_type=%u and condition_value=%u) "
			" order by auto_id",
			(uint8)E_NOTICE_USE_TYPE_GAME, (uint8)E_NOTICE_CONDITION_TYPE_SERVER_ID, dwServId);

	if (result != NULL)
	{
		if (result->GetRowCount() <= 0)
		{
			delete result;
			result = NULL;
		}
	}
	if (result == NULL)
	{
		result = m_db->PQuery(
				"select content, start_time, end_time, condition_type, condition_value, auto_id from notice_info_v2 "
					" where (end_time=0 or end_time>unix_timestamp()) and "
					" use_type=%u and condition_type=%u "
					" order by auto_id",
					(uint8)E_NOTICE_USE_TYPE_GAME, (uint8)E_NOTICE_CONDITION_TYPE_PLATFORM);
	}

	if (result == NULL)
		return false;

	LoginDBNoticeInfo notice;

	uint64 cnt = result->GetRowCount();
	for (unsigned int i = 0; i < cnt; i++ )
	{
		Field* field = result->Fetch();

		notice.strContent	= field[0].GetString();
		notice.dwStartTs 	= field[1].GetUInt32();
		notice.dwEndTs		= field[2].GetUInt32();

		uint32 dwCondType	= field[3].GetUInt32();
		notice.dwPlateFormId= field[4].GetUInt32();
		notice.dwAutoId		= field[5].GetUInt32();

		if (dwCondType == E_NOTICE_CONDITION_TYPE_SERVER_ID)
			notice.bIsPlateForm = false;
		else
			notice.bIsPlateForm = true;

		vecNotice.push_back(notice);

		result->NextRow();
	}
	delete result;

	return true;
}

uint64 CenterDBCtrl::UpdateRoleToken( uint32 dwRoleId, std::string strToken )
{
	m_db->escape_string( strToken );

	QueryResult* result = m_db->PQuery(
		"select passport_id from re_passport_player where role_id=%u", dwRoleId
	);

	if ( result )
	{
		uint64 ret = result->Fetch()[0].GetUInt64();
		delete result;

		m_db->PExecute(
			"update passport_info set token='%s' where passport_id=%llu", strToken.c_str(), ret
		);

		return ret;
	}
	else
	{
		return 0;
	}
}
uint8 CenterDBCtrl::GetGmAuthByPassportId( uint64 ddwPassportId )
{
	QueryResult* result  = m_db->PQuery( "select gm_auth from passport_info where passport_id=%llu",ddwPassportId  );
	uint8 byAuthRole = 0;
	if ( result )
	{
		byAuthRole = result->Fetch()[0].GetUInt8();
		delete result;
	}
	else
	{
		IME_SYSTEM_ERROR( "GetGmAuth", "PassportAuth Not Found, passport_id=%lu", ddwPassportId );
	}
	return byAuthRole;
}
uint8 CenterDBCtrl::GetGmAuth( uint32 dwRoleId )
{
	uint8 byAuthRole = 0, byAuthPassport = 0;
	QueryResult* result = m_db->PQuery(
		"select gm_auth from player_info where role_id=%u", dwRoleId
	);
	if ( result )
	{
		byAuthRole = result->Fetch()[0].GetUInt8();
		delete result;
	}
	else
	{
		IME_SYSTEM_ERROR( "GetGmAuth", "RoleAuth Not Found, roleId=%u", dwRoleId );
	}
	result = m_db->PQuery( "select gm_auth from passport_info where passport_id="
			"(select passport_id from re_passport_player where role_id=%u)", dwRoleId );
	if ( result )
	{
		byAuthPassport = result->Fetch()[0].GetUInt8();
		delete result;
	}
	else
	{
		IME_SYSTEM_ERROR( "GetGmAuth", "PassportAuth Not Found, roleId=%u", dwRoleId );
	}

	return std::max( byAuthRole, byAuthPassport );
}

uint32 CenterDBCtrl::GetPlatformId( uint32 dwRoleId )
{
	QueryResult* result = m_db->PQuery( "select platform from passport_info where passport_id="
			"(select passport_id from re_passport_player where role_id=%u)", dwRoleId );

	if ( result )
	{
		uint32 ret = result->Fetch()[0].GetUInt32();
		delete result;

		return ret;
	}
	else
	{
		return 0;
	}
}

uint32 CenterDBCtrl::GetRoleCreateTime( uint32 dwRoleId )
{
	uint32 ret = 0;
	QueryResult* result = m_db->PQuery(
		"select create_time from re_passport_player where role_id=%u", dwRoleId
	);
	if ( result )
	{
		ret = result->Fetch()[0].GetUInt32();
		delete result;
	}

	return ret;
}

std::string CenterDBCtrl::GetRoleName( uint32 dwRoleId )
{
	QueryResult* result = m_db->PQuery(
		"select name from player_info where role_id=%u", dwRoleId );

	if ( result )
	{
		std::string ret = result->Fetch()[0].GetString();
		delete result;

		return ret;
	}
	return "";
}

uint16 CenterDBCtrl::GetRoleServerId( uint32 dwRoleId )
{
	// get server_id after merged
	QueryResult* result = m_db->PQuery(
		"select server_id from re_passport_player where role_id=%u", dwRoleId
	);
	if ( result )
	{
		uint16 ret = result->Fetch()[0].GetUInt16();
		delete result;
		return ret;
	}
	return 0;
}

uint16 CenterDBCtrl::GetRoleServerIdOrigin( uint32 dwRoleId )
{
	// get server_id after merged
	QueryResult* result = m_db->PQuery(
		"select server_id_origin from re_passport_player where role_id=%u", dwRoleId
	);
	if ( result )
	{
		uint16 ret = result->Fetch()[0].GetUInt16();
		delete result;
		return ret;
	}
	return 0;
}

int CenterDBCtrl::GetRolePlatform( uint32 dwRoleId )
{
	QueryResult* result = m_db->PQuery(
		"select b.platform from re_passport_player a inner join passport_info b on (a.passport_id = b.passport_id) where role_id=%u",
		dwRoleId
	);

	if ( result )
	{
		uint16 ret = result->Fetch()[0].GetUInt16();
		delete result;
		return ret;
	}
	return -1;
}



bool CenterDBCtrl::SetRoleStatus( uint32 dwRoleId, uint8 byStatus )
{
	return m_db->PExecute(
		"update player_info set status=%u where role_id=%u", byStatus, dwRoleId
	);
}

uint8 CenterDBCtrl::GetRoleStatus( uint32 dwRoleId )
{
	QueryResult* result = m_db->PQuery(
		"select status from player_info where role_id=%u", dwRoleId
	);

	if ( result )
	{
		uint8 ret = result->Fetch()[0].GetUInt8();
		delete result;
		return ret;
	}
	return 0;
}

bool CenterDBCtrl::SetRoleLevel( uint32 dwRoleId,uint16 wLevel)
{
	return m_db->PExecute(
		"update re_passport_player set role_level=%u where role_id=%u", wLevel, dwRoleId
	);
}
void CenterDBCtrl::GetRoleLevel(uint64 dwPassportId, std::map<uint32/*serverId*/,uint32/*level*/>& mapServerLevel)
{
	QueryResult* result = m_db->PQuery(
			"select server_id_origin,role_level from re_passport_player where passport_id=%llu" , dwPassportId
	);
	if (result) {
		int nRows = result->GetRowCount();
		for (int i = 0; i < nRows; i++) {
			Field* field = result->Fetch();
			uint32 server_id = field[0].GetUInt32();
			uint32 level = field[1].GetUInt32();
			mapServerLevel[server_id] = level;
			result->NextRow();
		}
	}
	return;
}
uint32 CenterDBCtrl::GetPtrCount( uint32 dwRoleId )
{
	QueryResult* result = m_db->PQuery(
			"select count(*) from passport_info where passport like 'p%d%%'" , dwRoleId
	);

	if ( result )
	{
		uint32 ret = result->Fetch()[0].GetUInt32();
		delete result;
		return ret;
	}
	return 0;
}

bool CenterDBCtrl::SetLastLoginServer( uint64 dwPassportId, uint16 wServerId )
{
	return m_db->PExecute( "update passport_info set last_login_server=%u where passport_id=%llu",
			wServerId, dwPassportId );
}

bool CenterDBCtrl::IsTestServer( uint32 dwServerId )
{
	QueryResult* result = m_db->PQuery(
		"select is_test from gameserver_info where server_id=%u", dwServerId );

	if ( result )
	{
		uint8 byIsTest = result->Fetch()[0].GetUInt8();

		delete result;
		return byIsTest;
	}
	return false;
}


uint8 CenterDBCtrl::BindPassport( uint32 dwRoleId, LoginAuthType eAuthType, std::string strPassport,
			std::string strPassword, std::string strMail )
{
	if ( strPassport.empty() )
		return E_LOGIN_BIND_RESULT_TYPE_EMPTY_PASSPORT;

	if ( eAuthType == E_LOGIN_AUTH_TYPE_ACCOUNT && ( strPassword.size() < 6 || strMail.empty() ) )
		return E_LOGIN_BIND_RESULT_TYPE_INVALID_PWD_OR_MAIL;

	uint64 dwPassportId = GetPassportId( dwRoleId );
	if ( dwPassportId == 0 )
		return E_LOGIN_BIND_RESULT_TYPE_PASSPORT_NOT_EXIST;

	STC_PASSPORT_INFO stcInfo;
	GetPassportInfo( dwPassportId, stcInfo );

	m_db->escape_string( strPassport );
	m_db->escape_string( strMail );

	QueryResult* result = m_db->PQuery( "select passport_id from passport_info where auth_type=%u and platform=%u and passport='%s'",
			(uint8)eAuthType, stcInfo.dwPlatform, strPassport.c_str() );

	if ( result )
	{
		if ( result->Fetch()[0].GetUInt64() != dwPassportId )
		{
			delete result;
			return E_LOGIN_BIND_RESULT_TYPE_DUPLICATE_PASSPORT;
		}
		else
		{
			delete result;
			return E_LOGIN_BIND_RESULT_TYPE_SUCC;
		}
	}

	if ( eAuthType == E_LOGIN_AUTH_TYPE_ACCOUNT )
	{
		m_db->PExecute( "update passport_info set passport='%s', pwd='%s', mail='%s', auth_type=%u where passport_id=%llu",
				strPassport.c_str(), strPassword.c_str(), strMail.c_str(), eAuthType, dwPassportId );
		return E_LOGIN_BIND_RESULT_TYPE_SUCC;
	}
	else if ( eAuthType == E_LOGIN_AUTH_TYPE_PLATFORM )
	{
		m_db->PExecute( "update passport_info set passport='%s', auth_type=%u where passport_id=%llu",
				strPassport.c_str(), eAuthType, dwPassportId );
		return E_LOGIN_BIND_RESULT_TYPE_SUCC;
	}
	else
	{
		return E_LOGIN_BIND_RESULT_TYPE_INVALID_AUTH_TYPE;
	}
}

uint32 CenterDBCtrl::RegisterGuild( uint32 dwRoleId )
{
	if ( !m_db->PExecute(
		"insert into guild_reg( role_id, create_time ) values( %u, %u )",
		dwRoleId, GetDBTime() ) )
	{
		IME_SQLERROR("Error When Insert Guild Reg");
		return 0;
	}

	QueryResult* result = m_db->PQuery( "select LAST_INSERT_ID()" );

	if ( result )
	{
		uint32 ret = result->Fetch()[0].GetUInt32();
		delete result;

		return ret;
	}
	else
	{
		IME_SQLERROR("Error When Insert Guild Reg");
		return 0;
	}
}

bool CenterDBCtrl::InitGmCommand()
{
	QueryResult* result = m_db->PQuery(
		"select auto_id, opr, target_type, target_id, start_time, end_time, backup_value, status, params from gm_cmd"
		" where status = %u or status = %u or status = %u",
		E_GM_COMMAND_STATUS_NEW, E_GM_COMMAND_STATUS_PENDING, E_GM_COMMAND_STATUS_RUNNING
	);

	if ( result )
	{
		ReadCommands( result );
		delete result;
	}
	else
	{
		m_dwMaxHandledGmCommandId = 0;
	}
	return true;
}

bool CenterDBCtrl::ReadNewGmCommand()
{
	QueryResult* result = m_db->PQuery(
		"select auto_id, opr, target_type, target_id, start_time, end_time, backup_value, status, params from gm_cmd"
		" where auto_id > %u and status = %u",
		m_dwMaxHandledGmCommandId, E_GM_COMMAND_STATUS_NEW
	);

	if ( result )
	{
		ReadCommands( result );
		delete result;
	}
	return true;
}

bool CenterDBCtrl::UpdateGmCommand()
{
	uint32 dwTime = GetDBTime();

	if ( dwTime <= m_dwMaxRunningGmCommandTime )
	{
		IME_SYSTEM_ERROR( "UpdateGmCommand", "Invalid Update, max=%u, cur=%u", m_dwMaxRunningGmCommandTime, dwTime );
		return false;
	}

	std::vector<GmCommand*> vRemoveCmd;

	for ( GmCommandMapType::iterator it = m_mapGmCommandAll.begin();
			it != m_mapGmCommandAll.end(); it++ )
	{
		GmCommand* cmd = it->second;
		if (cmd->GetStartTime() == 0)
			cmd->SetStartTime(dwTime);
		if (cmd->GetEndTime() < cmd->GetStartTime())
			cmd->SetEndTime(cmd->GetStartTime());

		do
		{
			if ( cmd->GetStatus() == E_GM_COMMAND_STATUS_NEW )
			{
				IME_SYSTEM_LOG( "GmCommand", "Valid GM Command, id=%d, opr=%s", cmd->GetAutoId(), cmd->GetOpr().c_str() );
				if ( !cmd->Validate() )
				{
					cmd->SetStatus( E_GM_COMMAND_STATUS_ERROR );
					break;
				}
				IME_SYSTEM_LOG( "GmCommand", "GM Command Pending, id=%d, opr=%s", cmd->GetAutoId(), cmd->GetOpr().c_str() );
				cmd->SetStatus( E_GM_COMMAND_STATUS_PENDING );
			}

			if ( cmd->GetStatus() == E_GM_COMMAND_STATUS_PENDING )
			{
				if ( cmd->GetStartTime() == 0 || cmd->GetStartTime() < dwTime )
				{
					IME_SYSTEM_LOG( "GmCommand", "Trigger GM Command, id=%d, opr=%s", cmd->GetAutoId(), cmd->GetOpr().c_str() );
					cmd->SetStartTime( dwTime ); //
					if ( !cmd->Trigger() )
					{
						cmd->SetStatus( E_GM_COMMAND_STATUS_ERROR );
						break;
					}
					IME_SYSTEM_LOG( "GmCommand", "GM Command Running, id=%d, opr=%s", cmd->GetAutoId(), cmd->GetOpr().c_str() );
					cmd->SetStatus( E_GM_COMMAND_STATUS_RUNNING );
				}
			}

			if ( cmd->GetStatus() == E_GM_COMMAND_STATUS_RUNNING )
			{
				cmd->Tick();
				if ( cmd->GetEndTime() == 0 || cmd->GetEndTime() < dwTime )
				{
					IME_SYSTEM_LOG( "GmCommand", "Finish GM Command, id=%d, opr=%s", cmd->GetAutoId(), cmd->GetOpr().c_str() );
					if ( !cmd->Complete() )
					{
						cmd->SetStatus( E_GM_COMMAND_STATUS_ERROR );
						break;
					}
					cmd->SetStatus( E_GM_COMMAND_STATUS_COMPLETE );
					IME_SYSTEM_LOG( "GmCommand", "GM Command Complete, id=%d, opr=%s", cmd->GetAutoId(), cmd->GetOpr().c_str() );
				}
			}

		} while(0);
	}

	// loop again
	for ( GmCommandMapType::iterator it = m_mapGmCommandAll.begin();
			it != m_mapGmCommandAll.end(); it++ )
	{
		GmCommand* cmd = it->second;

		if ( cmd->Dirty() )
		{
			if ( !m_db->PExecute(
				"update gm_cmd set opr='%s', target_type=%u, target_id=%llu, start_time=%u, end_time=%u, "
				"backup_value='%s', status=%u where auto_id=%u",
				cmd->GetOpr().c_str(), cmd->GetTargetType(), cmd->GetTargetId(), cmd->GetStartTime(), cmd->GetEndTime(),
				cmd->GetBackup().c_str(),  cmd->GetStatus(), cmd->GetAutoId()
			) )
			{
				IME_SQLERROR( "Error Update GmCommand in 'gm_cmd', id=%u", cmd->GetAutoId() );
			}

			if ( cmd->GetStatus() == E_GM_COMMAND_STATUS_ERROR )
			{
				IME_SYSTEM_LOG( "GmCommand", "GM Command Error, id=%d, opr=%s, msg=%s",
						cmd->GetAutoId(), cmd->GetOpr().c_str(), cmd->GetErrorMsg().c_str() );
			}
			std::string strErrorMsg = cmd->GetErrorMsg();
			m_db->escape_string( strErrorMsg );

			if ( !m_db->PExecute("update gm_cmd set error_msg='%s' where auto_id=%u",
						strErrorMsg.c_str(), cmd->GetAutoId() ) )
			{
				IME_SQLERROR( "Error Update ErrorMsg in 'gm_cmd', id=%u", cmd->GetAutoId() );
			}
		}

		cmd->Reset();

		if ( cmd->GetStatus() == E_GM_COMMAND_STATUS_COMPLETE || cmd->GetStatus() == E_GM_COMMAND_STATUS_ERROR
				|| cmd->GetStatus() == E_GM_COMMAND_STATUS_CANCELED )
		{
			vRemoveCmd.push_back( cmd );
		}
	}

	for ( std::vector<GmCommand*>::iterator it = vRemoveCmd.begin();
			it != vRemoveCmd.end(); ++it )
	{
		RemoveGmCommand( *it );
	}
	m_dwMaxRunningGmCommandTime = dwTime;
	return true;
}

bool CenterDBCtrl::HasGmCommand( uint32 dwGmCommandId )
{
	return m_mapGmCommandAll.find( dwGmCommandId ) != m_mapGmCommandAll.end();
}

bool CenterDBCtrl::CancelGmCommand( uint32 dwGmCommandId )
{
	GmCommandMapType::iterator it = m_mapGmCommandAll.find( dwGmCommandId );
	if ( it != m_mapGmCommandAll.end() )
	{
		return it->second->Cancel();
	}
	return false;
}

const GmCommand* CenterDBCtrl::GetGmCommand( uint32 dwGmCommandId )
{
	GmCommandMapType::iterator it = m_mapGmCommandAll.find( dwGmCommandId );
	if ( it != m_mapGmCommandAll.end() )
	{
		return it->second;
	}
	return NULL;
}

uint32 CenterDBCtrl::HandlerGmCommandRole( void* pRole, uint32 dwLastCmdTime )
{
	if ( m_dwMaxRunningGmCommandTime <= dwLastCmdTime ) return dwLastCmdTime;

	for ( GmCommandMapType::iterator it = m_mapGmCommandAll.begin();
			it != m_mapGmCommandAll.end(); ++it )
	{
		if ( it->second->GetStatus() == E_GM_COMMAND_STATUS_RUNNING && dwLastCmdTime < it->second->GetStartTime() )
		{
			it->second->HandleRole( pRole );
		}
	}
	return m_dwMaxRunningGmCommandTime;
}

uint32 CenterDBCtrl::CreateSysGmCommand( std::string strOpr, std::string strParams, uint8 byTargetType,
			uint32 dwTargetId, uint32 dwStartTime, uint32 dwEndTime )
{
	m_db->escape_string( strOpr );
	m_db->escape_string( strParams );

	bool res = m_db->PExecute( "insert into gm_cmd (opr, params, target_type, target_id, start_time, end_time, backup_value,"
			"status, error_msg, create_time, author) values( '%s', '%s', %u, %u, %u, %u, '%s', %u, '%s', %u, '%s' )",
			strOpr.c_str(), strParams.c_str(), byTargetType, dwTargetId, dwStartTime, dwEndTime, "", 0, "", GetDBTime(), "sys" );

	if ( res )
	{
		QueryResult* result = m_db->PQuery( "select LAST_INSERT_ID()" );

		if ( result )
		{
			uint32 ret = result->Fetch()[0].GetUInt32();
			delete result;

			return ret;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

void CenterDBCtrl::ReadCommands( QueryResult* result )
{
	uint64 cnt = result->GetRowCount();
	for (unsigned int i = 0; i < cnt; i++ )
	{
		Field* field = result->Fetch();

		uint32 		dwAutoId	= field[0].GetUInt32();
		std::string strOpr	 	= field[1].GetString();
		uint8		byTargetType= field[2].GetUInt8();
		uint64		dwTargetId	= field[3].GetUInt64();
		uint32		dwStartTime = field[4].GetUInt32();
		uint32		dwEndTime	= field[5].GetUInt32();
		std::string strBackup	= field[6].GetString();
		uint8		byStatus	= field[7].GetUInt8();
		std::string strParams	= field[8].GetTextString();
		m_dwMaxHandledGmCommandId = std::max( m_dwMaxHandledGmCommandId, dwAutoId );

		// check if this game server should handle this command
		if ( GmCommandFactory::CheckTargetType( byTargetType, dwTargetId ) )
		{
			GmCommand* cmd = GmCommandFactory::Create( dwAutoId, strOpr, (GmCommandTargetType)byTargetType,
					dwTargetId, dwStartTime, dwEndTime, strBackup, (GmCommandStatus)byStatus, strParams );

			if ( cmd == NULL )
			{
				IME_SYSTEM_ERROR( "GmCommand", "Operation Not Support, opr=%s", strOpr.c_str() );

				m_db->PExecute( "update gm_cmd set status=%u, error_msg='%s' where auto_id=%u",
						E_GM_COMMAND_STATUS_ERROR, "Operation Not Support", dwAutoId );
			}
			else
			{
				AppendGmCommand( cmd );
			}
		}

		result->NextRow();
	}
}

bool CenterDBCtrl::AppendGmCommand( GmCommand* pCommand )
{
	IME_SYSTEM_LOG( "GmCommand", "Append GM Command, id=%d, opr=%s", pCommand->GetAutoId(), pCommand->GetOpr().c_str() );
	m_mapGmCommandAll.insert( std::make_pair( pCommand->GetAutoId(), pCommand ) );
	return true;
}

bool CenterDBCtrl::RemoveGmCommand( GmCommand* pCommand )
{
	IME_SYSTEM_LOG( "GmCommand", "Remove GM Command, id=%d, opr=%s", pCommand->GetAutoId(), pCommand->GetOpr().c_str() );
	m_mapGmCommandAll.erase( pCommand->GetAutoId() );
	delete pCommand;
	return true;
}

void CenterDBCtrl::InsertOrUpdateGoodsInfo( STC_GOODS_INFO& stcGood, uint32 dwGameServerId )
{
    m_db->PExecute("insert into goods_info(server_id, goods_id, shop_type, buy_type_id, "
            "buy_content_id, buy_count, cost_type_id, cost_content_id, cost_count, cost_count_old, "
            "status, limit_day, sort_idx ) values (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u)  "
            " on duplicate key update shop_type=%u, buy_type_id=%u,buy_content_id=%u,buy_count=%u,"
            "cost_type_id=%u,cost_content_id=%u, cost_count=%u, cost_count_old=%u, status=%u,limit_day=%u,sort_idx=%u  ",
            dwGameServerId,
            stcGood.dwGoodsId,
            stcGood.byShopType,
            stcGood.dwBuyTypeId,
            stcGood.dwBuyContentId,
            stcGood.dwBuyCount,
            stcGood.dwCostTypeId,
            stcGood.dwCostContentId,
            stcGood.dwCostCount,
            stcGood.dwCostCountOld,
            stcGood.byStatus,
            stcGood.dwLimitDay,
            stcGood.dwSortIdx,

            dwGameServerId,
            stcGood.dwGoodsId,
            stcGood.byShopType,
            stcGood.dwBuyTypeId,
            stcGood.dwBuyContentId,
            stcGood.dwBuyCount,
            stcGood.dwCostTypeId,
            stcGood.dwCostContentId,
            stcGood.dwCostCount,
            stcGood.dwCostCountOld,
            stcGood.byStatus,
            stcGood.dwLimitDay,
            stcGood.dwSortIdx
    );
}

void CenterDBCtrl::UpdateGoodsInfoOfGameServer( std::map<uint32, STC_GOODS_INFO>& vGoods, uint32 dwGameServerId )
{
	for ( std::map<uint32, STC_GOODS_INFO>::iterator it = vGoods.begin();
			it != vGoods.end(); ++it )
	{
		m_db->PExecute( "insert into goods_info(server_id, goods_id, shop_type, buy_type_id, "
				"buy_content_id, buy_count, cost_type_id, cost_content_id, cost_count, cost_count_old, "
				"status, limit_day, sort_idx, icon_id, goods_name, description, platform_goods_id, platform_type,"
				"discount, role_lv_req, bonus_buy_count, bonus_limit, bonus_reset_type, diamond_pay ) "
				"values(%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, '%s', '%s', '%s', '%s', %u, %u, %u, %u, %u, %u, %u)",
				dwGameServerId,
				it->second.dwGoodsId,
				it->second.byShopType,
				it->second.dwBuyTypeId,
				it->second.dwBuyContentId,
				it->second.dwBuyCount,
				it->second.dwCostTypeId,
				it->second.dwCostContentId,
				it->second.dwCostCount,
				it->second.dwCostCountOld,
				it->second.byStatus,
				it->second.dwLimitDay,
				it->second.dwSortIdx,
				it->second.strIcon.c_str(),
				it->second.strName.c_str(),
				it->second.strDescription.c_str(),
				it->second.strPlatformGoodsId.c_str(),
				it->second.dwPlatformType,
				it->second.dwDiscount,
				it->second.wRoleLvReq,
				it->second.dwBonusBuyCount,
				it->second.dwBonusLimit,
				it->second.byBonusResetType,
				it->second.dwDiamondPay );
	}
}

void CenterDBCtrl::CreateLoginToken( uint64 ddwPassportId, std::string strIp,bool bIsShadowLogin/*不记录留存*/ )
{
	m_db->PExecute( "update passport_info set token_ip='%s', token_time=%u, shadow_login=%u where passport_id=%llu",
			strIp.c_str(), GetDBTime(),bIsShadowLogin, ddwPassportId);
}

void CenterDBCtrl::ClearLoginToken( uint32 dwRoleId )
{

	m_db->PExecute( "update passport_info set token_ip='', token_time=0 where passport_id=%llu", GetPassportId( dwRoleId ) );
}

static bool CheckIp( std::string strIp, std::string strPattern )
{
	std::vector<std::string> vTarget, vPattern;
	CUtil::StrSplit( strIp, ".", vTarget );
	if ( vTarget.size() != 4 ) return false;
	CUtil::StrSplit( strPattern, ".", vPattern );
	if ( vPattern.size() != 4 ) return false;

	for ( int i = 0; i < 4; i++ )
	{
		if ( vPattern[i] == "*" )
		{
			continue;
		}
		else if ( vPattern[i].find('-') != std::string::npos )
		{
			std::vector<std::string> vRange;
			CUtil::StrSplit( vPattern[i], "-", vRange );
			if ( vRange.size() != 2 ) return false;
			int a = atoi( vTarget[i].c_str() ), low = atoi( vRange[0].c_str() ), high = atoi( vRange[1].c_str() );
			if ( low > high ) std::swap( low, high );
			if ( a < low || a > high ) return false;
		}
		else
		{
			if ( atoi( vTarget[i].c_str() ) != atoi( vPattern[i].c_str() ) ) return false;
		}
	}
	return true;
}

bool CenterDBCtrl::CheckLoginToken( uint32 dwRoleId, std::string strIp,bool& bIsShadowLogin )
{
	uint64 ddwPassportId = GetPassportId( dwRoleId );
	if ( ddwPassportId == 0 )
	{
		IME_USER_ERROR( "CheckLoginToken", dwRoleId, "Passport Not Found" );
		return false;
	}

	QueryResult* result = m_db->PQuery( "select token_ip, token_time,shadow_login from passport_info where passport_id=%llu",
			ddwPassportId );

	if ( result )
	{
		Field* field = result->Fetch();

		std::string strLoginIp 	= field[0].GetString();
		uint32		dwLoginTime	= field[1].GetUInt32();
		bIsShadowLogin	= field[2].GetUInt8();

		delete result;

		if ( GetDBTime() < dwLoginTime + DAY )
		{
			IME_USER_LOG( "CheckLoginToken", dwRoleId, "Success" );
			return true;
		}
		else
		{
			std::string strError = "Timeout";

			result = m_db->PQuery( "select value from login_strategy where strategy_id=255 and type=%u",
					E_LOGIN_STRATEGY_TYPE_IP );
			if ( result == NULL )
			{
				IME_USER_ERROR( "CheckLoginToken", dwRoleId, "%s", strError.c_str() );
				return false;
			}

			int cnt = result->GetRowCount();
			bool bIpInWhite = false;

			for ( int i = 0; i < cnt; i++ )
			{
				if ( CheckIp( strIp, result->Fetch()[0].GetString() ) )
				{
					bIpInWhite = true;
					break;
				}
				result->NextRow();
			}

			delete result;

			if ( bIpInWhite )
			{
				IME_USER_LOG( "CheckLoginToken", dwRoleId, "Success, IP In White List" );
				return true;
			}

			IME_USER_ERROR( "CheckLoginToken", dwRoleId, "%s", strError.c_str() );
			return false;
		}
	}
	else
	{
		IME_USER_ERROR( "CheckLoginToken", dwRoleId, "Token Not Found" );
		return false;
	}
	return false;
}

bool CenterDBCtrl::GetGiftCodeInfo( uint32 dwId, uint32& odwParam1, uint32& odwParam2, uint32& odwParam3,
		std::string& ostrReward, std::string& ostrServers, std::string& ostrPlatform, uint32& odwDeadTime,
		uint32& odwMaxUse, uint32& odwEveryUse )
{
	QueryResult* result = m_db->PQuery(
		"select param1, param2, param3, reward, server, platform, dead_time, use_max, use_every from gift_box_config where id=%u", dwId
	);

	if ( result )
	{
		Field* field = result->Fetch();

		odwParam1 	= field[0].GetUInt32();
		odwParam2 	= field[1].GetUInt32();
		odwParam3 	= field[2].GetUInt32();
		ostrReward	= field[3].GetString();
		ostrServers	= field[4].GetString();
		ostrPlatform= field[5].GetString();
		odwDeadTime	= field[6].GetUInt32();
		odwMaxUse	= field[7].GetUInt32();
		odwEveryUse = field[8].GetUInt32();

		delete result;
		return true;
	}
	else
	{
		return false;
	}
}

uint32 CenterDBCtrl::GetGiftCodeUsed( uint32 dwId, uint32 dwIdx )
{
	QueryResult* result = m_db->PQuery(
		"select count(1) from gift_box where id=%u and idx=%u", dwId, dwIdx );

	if ( result )
	{
		uint32 ret = result->Fetch()[0].GetUInt32();
		delete result;
		return ret;
	}
	return 0;
}

uint32 CenterDBCtrl::GetGiftCodeUsed( uint64 ddwRoleId, uint32 dwId, uint32 dwIdx )
{
    QueryResult* result = m_db->PQuery(
            "select * from gift_box where id=%u and idx=%u and role_id=%llu", dwId, dwIdx, ddwRoleId );
    if ( result )
    {
        return 1;
    }
    return 0;
}

uint32 CenterDBCtrl::GetGiftCodeSameTypeUsed( uint64 dwRoleId, uint32 dwId )
{
	QueryResult* result = m_db->PQuery(
		"select * from gift_box where id=%u and role_id=%llu", dwId, dwRoleId );
	if ( result )
	{
		uint32 ret = result->GetRowCount();
		delete result;
		return ret;
	}
	return 0;
}

bool CenterDBCtrl::InsertGiftCodeUse( uint64 dwRoleId, uint32 dwId, uint32 dwIdx )
{
	return m_db->PExecute( "insert into gift_box( id, idx, role_id, use_time ) values( %u, %u, %llu, %u )",
			dwId, dwIdx, dwRoleId, GetDBTime() );
}

void CenterDBCtrl::GetGoodsInfoOfGameServer( std::map< uint32, STC_GOODS_INFO>& vGoods, uint32 dwGameServerId )
{
	QueryResult* result = m_db->PQuery(
		"select goods_id, shop_type, buy_type_id, buy_content_id, buy_count, cost_type_id, cost_content_id, cost_count, cost_count_old,"
		"status, limit_day, sort_idx, icon_id, goods_name, description, platform_goods_id, platform_type, discount, role_lv_req,"
		"bonus_buy_count, bonus_limit, bonus_reset_type, diamond_pay "
		"from goods_info where server_id=%u", dwGameServerId
	);

	vGoods.clear();
	if ( result )
	{
		uint64 cnt = result->GetRowCount();
		for (unsigned int i = 0; i < cnt; i++ )
		{
			Field* field = result->Fetch();
			STC_GOODS_INFO t;

			int idx = 0;

			t.dwGoodsId			= field[idx++].GetUInt32();
			t.byShopType		= field[idx++].GetUInt8();
			t.dwBuyTypeId		= field[idx++].GetUInt32();
			t.dwBuyContentId	= field[idx++].GetUInt32();
			t.dwBuyCount		= field[idx++].GetUInt32();
			t.dwCostTypeId		= field[idx++].GetUInt32();
			t.dwCostContentId	= field[idx++].GetUInt32();
			t.dwCostCount		= field[idx++].GetUInt32();
			t.dwCostCountOld	= field[idx++].GetUInt32();
			t.byStatus			= (GoodsState)field[idx++].GetUInt8();
			t.dwLimitDay		= field[idx++].GetUInt32();
			t.dwSortIdx			= field[idx++].GetUInt32();
			t.strIcon			= field[idx++].GetString();
			t.strName			= field[idx++].GetString();
			t.strDescription	= field[idx++].GetString();
			t.strPlatformGoodsId= field[idx++].GetString();
			t.dwPlatformType	= field[idx++].GetUInt32();
			t.dwDiscount		= field[idx++].GetUInt32();
			t.wRoleLvReq		= field[idx++].GetInt16();
			t.dwBonusBuyCount	= field[idx++].GetUInt32();
			t.dwBonusLimit		= field[idx++].GetUInt32();
			t.byBonusResetType	= field[idx++].GetUInt8();
			t.dwDiamondPay		= field[idx++].GetUInt32();

			vGoods.insert( std::make_pair( t.dwGoodsId, t ) );
			result->NextRow();
		}
		delete result;
	}
}

void CenterDBCtrl::UpdateGoodsInfoOfGameServerAll( std::map<uint32, STC_GOODS_INFO>& vGoods, uint32 dwGameServerId )
{
	m_db->PExecute( "delete from goods_info where server_id=%u", dwGameServerId );
	UpdateGoodsInfoOfGameServer( vGoods, dwGameServerId );
}

bool CenterDBCtrl::GetServerName( uint32 dwServerId, std::string &strServerName)
{
	QueryResult* res = m_db->PQuery( "select server_name from gameserver_info where server_id=%u", dwServerId);

	if ( res )
	{
		strServerName = res->Fetch()[0].GetString();
		delete res;

		return true;
	}
	else
	{
		return false;
	}
}


bool CenterDBCtrl::GetServerLocalIp( uint32 dwServerId, std::string &strIp )
{
    QueryResult* res = m_db->PQuery( "select local_ip from gameserver_info where server_id=%u", dwServerId);

    if ( res )
    {
        strIp = res->Fetch()[0].GetString();
        delete res;

        return true;
    }
    else
    {
        return false;
    }
}

bool CenterDBCtrl::GetServerDbName( uint32 dwServerId, std::string &strDbName)
{
    QueryResult* res = m_db->PQuery( "select db_name from gameserver_info where server_id=%u", dwServerId);

    if ( res )
    {
        strDbName = res->Fetch()[0].GetString();
        delete res;

        return true;
    }
    else
    {
        return false;
    }
}

bool CenterDBCtrl::UpdateServerIsTest( uint32 dwServerId, uint32 dwIsTest )
{
    return m_db->PExecute( "update gameserver_info set is_test = %u where server_id = %u", dwIsTest, dwServerId );
}

}
