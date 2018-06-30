
/* author : limingfan
 * date : 2017.09.18
 * description : 各服务公共接口操作API
 */

#ifndef __CSERVICE_OPERATION_H__
#define __CSERVICE_OPERATION_H__


#include "base/MacroDefine.h"
#include "base/CMemManager.h"
#include "SrvFrame/UserType.h"
#include "SrvFrame/CLogicHandler.h"
#include "CommonDataDefine.h"
#include "protocol/appsrv_common_module.pb.h"

#include "_NCommonConfig_.h"
#include "_DBConfig_.h"
#include "_MallConfigData_.h"
#include "_ServiceCommonConfig_.h"
#include "_NCattlesBaseConfig_.h"
#include "_NGoldenFraudBaseConfig_.h"
#include "_NFightLandlordBaseConfig_.h"


namespace NProject
{

class CServiceOperation : public NFrame::CHandler
{
public:
	CServiceOperation();
	~CServiceOperation();

public:
    // 调用其他任何API之前，必须先成功初始化
	bool init(NFrame::CModule* msgHander, const char* xmlCommonCfgFile);
	
	// 配置文件信息
public:
	// 刷新公共配置信息
	bool updateCommonConfig(const char* xmlCommonCfgFile);

    // 公共配置信息
	const NCommonConfig::CommonCfg& getCommonCfg();

    // DB配置信息
	const DBConfig::config& getDBCfg(bool isReset = false);
	
	// 服务公共配置信息
	const ServiceCommonConfig::SrvCommonCfg& getServiceCommonCfg(bool isReset = false);

    // 商城配置信息
	const MallConfigData::MallData& getMallCfg(bool isReset = false);

    // 牛牛游戏基础配置信息
	const NCattlesBaseConfig::CattlesBaseConfig& getCattlesBaseCfg(bool isReset = false);
	
	// 炸金花游戏基础配置信息
	const NGoldenFraudBaseConfig::GoldenFraudBaseConfig& getGoldenFraudBaseCfg(bool isReset = false);
	
	// 斗地主游戏基础配置信息
	const NFightLandlordBaseConfig::FightLandlordBaseConfig& getFightLandlordBaseCfg(bool isReset = false);

public:
	// 检查服务数据日志配置信息并创建对应的日志对象
	bool createDataLogger(const char* cfgName, const char* fileItem, const char* sizeItem, const char* backupCountItem);
	void destroyDataLogger();
	
	// 获取数据日志对象
    NCommon::CLogger* getDataLogger();

public:
    // 停止定时器
    void stopTimer(unsigned int& timerId);
	
	// 获取唯一记录ID
    unsigned int getRecordId(RecordIDType recordId);
	
public:
	// 消息编解码
	bool parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info = "");
	const char* serializeMsgToBuffer(const ::google::protobuf::Message& msg, const char* info = "");
	const char* serializeMsgToBuffer(const ::google::protobuf::Message& msg, char* msgBuffer, unsigned int& msgBufferLen, const char* info = "");
	
public:
	// 向服务发送消息包
	int sendPkgMsgToService(const char* username, unsigned int usernameLen, ServiceType srvType, const ::google::protobuf::Message& msg,
	                        const unsigned short protocolId, const char* info = "", int userFlag = 0, unsigned short handleProtocolId = 0);
							
	int sendPkgMsgToService(const ::google::protobuf::Message& msg, const unsigned int srvId, const unsigned short protocolId,
	                        const char* info = "", unsigned short handleProtocolId = 0);
							
	int sendPkgMsgToService(const char* userData, unsigned int userDataLen, const ::google::protobuf::Message& msg, const unsigned int srvId,
	                        const unsigned short protocolId, const char* info = "", int userFlag = 0, unsigned short handleProtocolId = 0);

	int sendPkgMsgToDbProxy(const ::google::protobuf::Message& msg, unsigned short protocolId, const char* username, const unsigned int usernameLen,
							const char* info = "", int userFlag = 0, unsigned short handleProtocolId = 0);

public:
    // 向服务发送数据包
	int sendMsgToService(const char* username, unsigned int usernameLen, ServiceType srvType, const char* data, const unsigned int len,
						 const unsigned short protocolId, int userFlag = 0, unsigned short handleProtocolId = 0);
	
	int sendMsgToService(const char* data, const unsigned int len, const unsigned int srvId, const unsigned short protocolId, unsigned short handleProtocolId = 0);
						 				
	int sendMsgToService(const char* userData, unsigned int userDataLen, const char* data, const unsigned int len, const unsigned int srvId,
						 const unsigned short protocolId, int userFlag = 0, unsigned short handleProtocolId = 0);

public:
	// 服务启动&停止通知中心服务
	int sendServiceStatusNotify(unsigned int serviceId, com_protocol::EServiceStatus serviceStatus,
	                            unsigned int serviceType = 0, const char* serviceName = "");

    // 玩家上线&下线通知中心服务
	int sendUserOnlineNotify(const char* username, unsigned int serviceId, const char* onlineIP = "",
	                         const char* hallId = "", const char* roomId = "", unsigned int roomRate = 0, int seatId = -1);
	int sendUserOfflineNotify(const char* username, unsigned int serviceId, const char* hallId = "", const char* roomId = "",
	                          int code = 0, const char* info = "", unsigned int onlineTime = 0, int seatId = -1);
							  
public:
    // 转发消息到消息中心，由消息中心路由发往目标服务
    int forwardMessage(const char* username, const char* data, const unsigned int len, unsigned int dstSrvType,
					   unsigned short dstProtocolId, unsigned int srcSrvId, unsigned short srcProtocol, const char* info = "");

public:
	// 修改房间数据同步通知
    int changeRoomDataNotify(const char* hallId, const char* roomId, int gameType, com_protocol::EHallRoomStatus status,
		   					 const com_protocol::ChessHallRoomBaseInfo* addRoomInfo,
						     const com_protocol::ClientRoomPlayerInfo* addPlayerInfo,
							 const char* delUsername, const char* info = "");
							 
    // 修改房间的使用状态
    int changeHallRoomStatus(const char* roomId, com_protocol::EHallRoomStatus status, const char* info = "");
					   
	// 房间数据变更同步通知到大厅
    int updateRoomDataNotifyHall(const char* hallId, const char* roomId, int gameType, com_protocol::EHallRoomStatus status,
		   					     const com_protocol::ChessHallRoomBaseInfo* addRoomInfo,
								 const com_protocol::ClientRoomPlayerInfo* addPlayerInfo,
								 const char* delUsername, const char* info = "");

	// 变更游戏房间里玩家的座位号
    int changeRoomPlayerSeat(const char* roomId, const char* username, int seatId, const char* info = "");
	
public:
	// 设置最近一起游戏的玩家信息
    int setLastPlayersNotify(const char* hallId, const ConstCharPointerVector& usernames, const char* info = "");

private:
	NFrame::CModule* m_msgHandler;
	
	NCommon::CLogger* m_dataLogger;


	DISABLE_COPY_ASSIGN(CServiceOperation);
};



// 挂接在连接代理上的用户数据
struct ConnectProxyUserData
{
	unsigned int timeSecs;                   // 连接时间点
	unsigned short status;                   // 连接代理状态
	unsigned short repeatTimes;              // 重复连接次数

	char userName[IdMaxLen];                 // 用户唯一ID
	unsigned int userNameLen;                // 用户唯一ID长度
};


class CServiceOperationEx : public CServiceOperation
{
public:
    // userDataSize 上层应用用户数据长度，用户数据结构必须要从 ConnectProxyUserData 继承
	CServiceOperationEx(unsigned int userDataSize);
	~CServiceOperationEx();

public:
    // 调用其他任何API之前，必须先成功初始化
	bool initEx(NFrame::CLogicHandler* msgHander, const char* xmlCommonCfgFile);
	
	// 设置相同用户ID重复连接通知协议
	void setCloseRepeateNotifyProtocol(int listenProtocolId, unsigned int clientNotifyProtocolId);


	// 连接代理相关操作处理
public:
    // 创建初始化连接代理用户数据
    ConnectProxyUserData* createConnectProxyUserData(NFrame::ConnectProxy* conn);
	
	// 销毁连接代理用户数据
    void destroyConnectProxyUserData(ConnectProxyUserData* cud);
	
	// 连接代理验证成功
    ConnectProxyUserData* checkConnectProxySuccess(const string& usreName, NFrame::ConnectProxy* conn);
	
	// 是否是验证成功的连接代理
    bool isCheckSuccessConnectProxy(const ConnectProxyUserData* cud);
	
	// 检查同一条连接代理是否重复验证
    bool isRepeateCheckConnectProxy(unsigned int forceQuitProtocol);
	
	// 检查发送消息的连接是否已成功通过验证，防止玩家验证成功之前胡乱发消息
    bool checkConnectProxyIsSuccess(unsigned int forceQuitProtocol);
	
	// 获取连接代理上的用户数据，userName为NULL则默认获取当前上下文连接代理，否则获取userName对应的连接代理
	ConnectProxyUserData* getConnectProxyUserData(const char* userName = NULL);
	
	// 用户是否在线，获取当前上下文中的userName，根据userName获取连接代理
	NFrame::ConnectProxy* getConnectProxyInfo(const char*& userName, const char* info);
	
	// 关闭相同用户ID对应的重复连接代理
    void closeRepeatConnectProxy(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

public:
	// 服务启动时清理连接代理
	void cleanUpConnectProxy(CCfg* srvMsgCommCfg);


    // 向网络客户端发送数据包、消息包
public:
    int sendMsgToProxyEx(const char* data, const unsigned int len, unsigned short protocolId, const char* userName);
	
	int sendMsgPkgToProxy(const ::google::protobuf::Message& pkg, unsigned short protocolId, const char* info = "", const char* username = NULL);
	int sendMsgPkgToProxy(const ::google::protobuf::Message& pkg, unsigned short protocolId, NFrame::ConnectProxy* conn, const char* info = "");
	

private:
	NCommon::CMemManager m_memForUserData;
	
	NFrame::CLogicHandler* m_msgHandler;
	
	unsigned int m_closeRepeateNotifyProtocolId;


	DISABLE_COPY_ASSIGN(CServiceOperationEx);
};

}


#endif // __CSERVICE_OPERATION_H__
