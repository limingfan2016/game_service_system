
/* author : limingfan
 * date : 2017.01.23
 * description : 翻翻棋相关逻辑处理
 */
 
#ifndef _CFFCHESS_LOGIC_HANDLER_H_
#define _CFFCHESS_LOGIC_HANDLER_H_

#include "base/MacroDefine.h"
#include "SrvFrame/CModule.h"
#include "MessageDefine.h"
#include "_DbproxyCommonConfig_.h"
#include "_MallConfigData_.h"


class CSrvMsgHandler;

using namespace NCommon;
using namespace NFrame;
using namespace NErrorCode;


class CFFChessLogicHandler : public NFrame::CHandler
{
public:
	CFFChessLogicHandler();
	~CFFChessLogicHandler();

public:
	int init(CSrvMsgHandler* msgHandler);
	void unInit();
	
	void updateConfig();
	
private:
    int getFFChessUserBaseInfo(const char* username, SFFChessUserBaseinfo* baseInfo);
	
	// 增加翻翻棋用户数据到DB，调用该函数成功后必须调用DB接口mysqlCommit()执行提交修改确认
	int addFFChessUserBaseInfo(const char* username, const SFFChessUserBaseinfo& baseInfo, const bool needCommit = false);
	
	// 刷新翻翻棋用户数据到DB，调用该函数成功后必须调用DB接口mysqlCommit()执行提交修改确认
	int updateFFChessUserBaseInfo(const char* username, const SFFChessUserBaseinfo& baseInfo, const bool needCommit = false);
	
	// 发送消息包
    int sendPkgMsgToService(const ::google::protobuf::Message& msg, const unsigned int srcSrvId, const unsigned short protocolId, const char* info);

private:
    // 添加新用户基本数据
	void addUserBaseInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取用户数据
	void getUserBaseInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 设置翻翻棋用户棋局比赛结果
	void setUserMatchResult(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	CSrvMsgHandler* m_msgHandler;
	

DISABLE_COPY_ASSIGN(CFFChessLogicHandler);
};

#endif // _CFFCHESS_LOGIC_HANDLER_H_
