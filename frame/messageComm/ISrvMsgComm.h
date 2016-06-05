
/* author : limingfan
 * date : 2015.01.15
 * description : 服务消息通信开发接口
 */

#ifndef ISRV_MSG_COMM_H
#define ISRV_MSG_COMM_H

#include "base/CCfg.h"


using namespace NCommon;

namespace NMsgComm
{

// 提供给服务消息通信开发者的开发接口
class ISrvMsgComm
{
public:
	virtual ~ISrvMsgComm() {};

public:
    // 必须先初始化操作成功才能收发消息
	virtual int init() = 0;
	virtual void unInit() = 0;
	virtual CCfg* getCfg() = 0;
	virtual void reLoadCfg() = 0;
	
public:
    // 获取本服务名、各服务名对应的ID
	virtual const char* getSrvName() = 0;
	virtual unsigned int getSrvId(const char* srvName) = 0;
	
public:
    // 收发消息
	virtual int send(const unsigned int srvId, const char* data, const unsigned int len) = 0;
	virtual int recv(char* data, unsigned int& len) = 0;
	
public:
    // 直接从缓冲区空间收消息，避免拷贝数据，效率较高
	virtual int beginRecv(char*& data, int& len, void*& cb) = 0;
	virtual void endRecv(const char* data, const int len, void* cb) = 0;
};


// 创建&销毁ISrvMsgComm对象实例
// srvMsgCommCfgFile : 消息通信配置文件
// srvName : 通信的本服务名称，在消息通信配置文件中配置
ISrvMsgComm* createSrvMsgComm(const char* srvMsgCommCfgFile, const char* srvName);
void destroySrvMsgComm(ISrvMsgComm*& instance);

}

#endif // ISRV_MSG_COMM_H


