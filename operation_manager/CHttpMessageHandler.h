
/* author : limingfan
 * date : 2017.09.27
 * description : Http 操作实现
 */

#ifndef CHTTP_MESSAGE_HANDLER_H
#define CHTTP_MESSAGE_HANDLER_H

#include "../http_base/CHttpSrv.h"
#include "BaseDefine.h"

using namespace NHttp;

class COptMsgHandler;

// Http消息协议处理对象
class CHttpMessageHandler : public NHttp::CHttpDataHandler
{
public:
	CHttpMessageHandler();
	~CHttpMessageHandler();

public:
	void init(COptMsgHandler* optMsgHandler);

private:
    // 服务启动时被调用
	virtual int onInit(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	// 服务停止时被调用
	virtual void onUnInit(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	// 服务配置更新时被调用
	virtual void onUpdateConfig();
	
private:
	void getRechargeRecordId(RecordIDType recordId);

// 服务收到的POST请求
private:
	bool chessHallManagerRecharge(const char* connId, Connect* conn, const RequestHeader& reqHeaderData, const HttpMsgBody& msgBody);

private:
    void managerRechargeReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    COptMsgHandler* m_msgHandler;


DISABLE_COPY_ASSIGN(CHttpMessageHandler);
};


#endif // CHTTP_MESSAGE_HANDLER_H
