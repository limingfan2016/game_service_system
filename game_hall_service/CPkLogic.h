
/* author : zhangyang
 * date : 2016.05.26
 * description : PK场逻辑服务实现
 */

#ifndef CPK_LOGIC_H
#define CPK_LOGIC_H

#include <string>
#include <unordered_map>

#include "base/MacroDefine.h"
#include "SrvFrame/CGameModule.h"
#include "SrvFrame/CDataContent.h"
#include "common/CClientVersionManager.h"
#include "TypeDefine.h"
#include "CLogicHandler.h"
#include "_HallConfigData_.h"
#include "common/CommonType.h"
using namespace NProject;

namespace NPlatformService
{

	//报名类型
	enum SignUpType
	{
		SignUpNone = -1,
		SignUp,				//报名
		QuickStart,			//快速开始
	};

	enum ETableStatus
	{
		StatusIdle = 0,			//空闲状态
		StatusMatching,			//匹配状态
		StatusPlaying,			//游戏中
	};

	enum EPlayeStatus
	{
		StatusReady = 0,		//准备状态
		StatusNotReady,			//未准备
	};

	enum 
	{
		ETimerId = 0,			//定时器默认初始值
	};

	enum EStarPlayCode
	{
		EStarPlaySuccess = 0,			//游戏成功开始
		EPlayInsufficientNumOfPeople,	//游戏人数不足
		EPlayerNotReady,				//玩家未准备
		EPlaying,						//游戏中
	};
	
	// PK场玩家操作
	enum EPKPlayerOpt
	{
		EEntranceSignUp = 1,          // 入场报名
		EFastSignUp = 2,              // 快速报名
		ESignUpFailed = 3,            // 报名失败
		EOnMatching = 4,              // 匹配对手中
		ECancelSignUp = 5,            // 取消报名
		EBeginPlay = 6,               // 开始PK对战
	};
	

	//选手信息
	struct  PlayerInfo
	{
		PlayerInfo(SignUpType _nSignUp, EPKFieldID _nQuickStartFieldID, EPlayeStatus _nPlayeStatus, uint32_t _lv)
		: nPlayeStatus(_nPlayeStatus), nSignUp(_nSignUp), nQuickStartFieldID(_nQuickStartFieldID), lv(_lv)
		{
		}

		PlayerInfo() : nPlayeStatus(EPlayeStatus::StatusReady), nSignUp(SignUpType::SignUpNone), nQuickStartFieldID(NProject::FieldIDNone), lv(0)
		{

		}

		EPlayeStatus				nPlayeStatus;			//选手状态
		SignUpType					nSignUp;				//选手报名方式	
		EPKFieldID					nQuickStartFieldID;		//优先开始的场次
		uint32_t					lv;						//用户等级(报名时去到该数据，用于避免匹配成功后在次查询)
		std::vector<EPKFieldID>		alternativeFieldID;		//备选的报名场次
		std::vector<std::string>	cheatList;				//防作弊列表	
		std::map<int, int>			ticket;				//key表示场次， value表示使用门票
	};

	//桌子
	struct TableInfo
	{
		TableInfo() : timerId(ETimerId), nStatus(StatusIdle)
		{
		}

		unsigned int							timerId;		//定时器ID
		ETableStatus							nStatus;		//桌子状态
		std::map<std::string, PlayerInfo>		playerList;		//选手列表	
	//	
	};

	//场次信息
	struct FieldInfo
	{
		FieldInfo() : nFieldID(EPKFieldID::FieldIDNone), bOpen(false)
		{	}
		EPKFieldID					nFieldID;			//场次ID
		bool									bOpen;				//开启标志
		vector<TableInfo>						tableList;			//桌子列表
	};

	//游戏记录
	struct PlayRecord
	{
		uint32_t				 ip;
		uint32_t				 port;
		uint32_t				 srv_id;		//服务ID
		time_t					 playEndTime;	//该局游戏结束时间
		EPKFieldID				 play_id;
	};

	// PK逻辑服务
	class CPkLogic : public NFrame::CHandler
	{
	public:

		CPkLogic();

		~CPkLogic();

		void load(CSrvMsgHandler* msgHandler);

		void unLoad(CSrvMsgHandler* msgHandler);

		void onLine(ConnectUserData* ud, ConnectProxy* conn);

		void offLine(ConnectUserData* ud);

		void updateConfig();

		//处理客户端请求获取PK场信息 应答
		void handleClientGetPkPlayInfoRsp(const com_protocol::GetUserOtherInfoRsp &userOtherInfoRsp, const std::string &userName);

		//处理客户端 报名(快速开始) 应答
		void handleClientSignUpRsp(com_protocol::GetUserPropAndPreventionCheatRsp &db_rsp);

		//开始游戏
		void onStartPlay(com_protocol::GetManyUserOtherInfoRsp &rsp);

private:

		//PK场主动引导
		void onActiveGuide(unsigned int timerId, int userId, void* param, unsigned int remainCount);

		//判断筛选是否完成
		bool isFilterFinish(int *pkPlay, int nLen);

		// PK游戏结束玩家离开PK场消息
		void handlePkPlayEndUserQuit(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);


		//获取用户道具与防作弊列表
		void getUserPropAndPreventionCheat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

		//初始化PK场
		void initPkPlay();

		//打开指定PK场
		void onOpenPKPlay(unsigned int timerId, int userId, void* param, unsigned int remainCount);

		//关闭指定PK场
		void onClosePKPlay(unsigned int timerId, int userId, void* param, unsigned int remainCount);

		//处理客户端请求获取PK场信息
		void handleClientGetPkPlayInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

		//处理客户端请求获取 详细规则信息
		void handleClientGetPkPlayRuleInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

		//处理客户端 报名(快速开始)请求
		void handleClientSignUpReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

		//处理客户端 取消报名 请求
		void handleClientCancelSignUpReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

		//处理客户端 快速开始选手 准备请求
		void handleClientReadyReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

		//处理客户端 断线重连 请求
		void handleClientlReconnectionReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
		
		//PK开始消息应答
		void playerStartPKReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

		void playerWaiverRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

		//PK场游戏结束通知
		void playEndPkNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

		//
		void getGameDataForSignUp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

		//匹配每个PK场的选手
		void matchingPlayer(unsigned int timerId, int userId, void* param, unsigned int remainCount);

		//匹配 快速匹配选手
		void onMatchingQuick(FieldInfo &field, bool bQuick);

		//剩余的快速匹配选手进行匹配
		void surplusQuickPlayerMatching();

		//匹配完成
		void matchingFinish();
		
		//开始游戏之前
		EStarPlayCode onBefStartPlay(TableInfo &table);

		//通知客户端PK场状态
		void notifyClientPKPlayStatus(EPKFieldID nFieldID, bool bStatus);

		//结束 等待玩家准备
		void waitEndPlayerReady(unsigned int timerId, int userId, void* param, unsigned int remainCount);

	private:

		//获取用户制定道具的数量
		template<typename T>bool getPropNum(uint32_t propType, uint32_t &num, T &db_rsp)
		{
			for (int i = 0; i < db_rsp.other_info_size(); ++i)
			{
				if (db_rsp.other_info(i).info_flag() == propType)
				{
					num = db_rsp.other_info(i).int_value();
					return true;
				}
			}

			OptErrorLog("CPkLogic getPropNum, prop type:%d", propType);
			num = 0;
			return false;
		}

		//获取指定场次的配置信息
		const HallConfigData::PkPlay* getPkPlayCfg(const HallConfigData::config& cfg, int nFieldID);

		//获取当前时间的秒数(距当天 0:0:0 的秒数)
		uint32_t getCurTimeSec();

		//获取指定的场次
		FieldInfo* getField(EPKFieldID nFieldID);

		//根据玩家获得桌子和场次
		bool getTableAndField(const std::string &user, vector<TableInfo>::iterator &table, std::vector<FieldInfo>::iterator &field);

		//判断玩家是否已经报名
		bool isUserSignUp(const std::string &user);

		//普通报名
		ProtocolReturnCode signUp(EPKFieldID nFieldID, SignUpType nSignUpType, uint32_t lv, const std::string &user, const com_protocol::GetUserPropAndPreventionCheatRsp &db_rsp);
		
		//快速报名
		ProtocolReturnCode quickStart(EPKFieldID nFieldID, SignUpType nSignUpType, uint32_t lv, const std::string &user, const com_protocol::GetUserPropAndPreventionCheatRsp &db_rsp);

		//判断是否符合入场条件
		ProtocolReturnCode isIntoField(EPKFieldID nFieldID, uint32_t lv, const com_protocol::GetUserPropAndPreventionCheatRsp &db_rsp, int &nTicket);

		//进入场次
		bool intoField(FieldInfo* signUpField, const std::string &user, const PlayerInfo &player, bool bQuickStart = false);

		//进入桌子
		bool intoTable(const std::string &user, const PlayerInfo &player, TableInfo &tableInfo);

		//判断能否进入桌子
		bool isIntoTable(const std::string &user, const TableInfo &tableInfo);

		//
		bool isAlternativeField(const PlayerInfo &player, EPKFieldID nFieldID);

		//快速开始选手 退出该桌子
		void quickStarPlayerEscTable(TableInfo &table, bool bNotice = false);

		//扫描桌上没有准备的选手
		void scanTablePlayerNotReady(TableInfo &table, std::map<std::string, PlayerInfo> &result);

		//取消桌上选手报名状态
		bool cancelTablePlayerSignUp(TableInfo &table, const std::string &user, ProtocolReturnCode &code);

		//扫描该场次优先的选手
		void scanFirstField(EPKFieldID nFieldID, const std::map<std::string, PlayerInfo> &player, std::map<std::string, PlayerInfo> &result);

		//扫描该场次的选手
		void scanField(EPKFieldID nFieldID, const std::map<std::string, PlayerInfo> &player, std::map<std::string, PlayerInfo> &result);

		//将快速开始选手加入到场次中
		void quickStartPlayerIntoField(FieldInfo &field, std::map<std::string, PlayerInfo> &intoPlayer, std::map<std::string, PlayerInfo> &quickStartList);

		//清空桌子信息
		void clearTable(TableInfo &table);

		//写游戏记录
		bool writeRecord(const TableInfo* pTable, uint32_t ip, uint32_t port, uint32_t srvId, uint32_t playTime, EPKFieldID nPkFieldId);
		
		// 生成PK场游戏记录ID
		unsigned int generatePKRecordId(char* recordId, unsigned int len);
		
		// 玩家操作记录
		void writePlayerOptRecord(const char* username, const unsigned int uLen, unsigned int optFlag, const char* remarks = "");

	private:
		
		//获取PK排行榜的清空次数
		void getPKPlayRankingClearNumRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

		//获取大厅的数据(更新玩家的排名信息)
		void getOnLineUserHallDataUpPKRangkingInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

		//更新玩家最佳PK积分
		bool updateUserMaxPkScore(const string &username, uint32_t pkScore, uint64_t nClearNum, uint32_t &maxSCore);
		bool updateUserMaxPkScore(com_protocol::HallLogicData* hallLogicData, uint32_t pkScore, uint64_t nClearNum, uint32_t &maxSCore);

	private:

		typedef std::map<std::string, PlayerInfo>::value_type PLAYER_VALUE;

	private:

		CSrvMsgHandler							*m_pMsgHandler;
		unsigned int							m_tableID;
		std::vector<FieldInfo>					m_field;									//每个场次
		std::map<std::string, PlayerInfo>		m_quickStartList;							//快速开始选手列表
		std::map<std::string, PlayerInfo>		m_tempSignUpPlayer;							//临时存储报名选手信息 (用于异步查询用户信息回来后做逻辑处理)
		std::unordered_map<std::string, PlayRecord>		m_playRecord;								//游戏记录
		DISABLE_COPY_ASSIGN(CPkLogic);
	};

}

#endif // CPK_LOGIC_H
