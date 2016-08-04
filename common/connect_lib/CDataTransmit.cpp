
/* author : limingfan
 * date : 2015.06.30
 * description : 纯网络数据读写，不做数据解析、心跳监测等其他额外操作
 */

#include "CDataTransmit.h"
#include "CDataHandler.h"
#include "CConnectManager.h"
#include "base/ErrorCode.h"


using namespace NCommon;
using namespace NErrorCode;

namespace NConnect
{

CDataTransmit::CDataTransmit(CConnectManager* connMgr, ILogicHandler* logicHandler) : m_connMgr(connMgr), m_logicHandler(logicHandler)
{
	m_curConn = NULL;
}

CDataTransmit::~CDataTransmit()
{
	m_curConn = NULL;
	m_connMgr = NULL;
	m_logicHandler = NULL;
}

// 由逻辑层主动收发消息
ReturnValue CDataTransmit::recv(Connect*& conn, char* data, unsigned int& len)
{
	// 1）先过滤掉无效的连接
	while (m_curConn != NULL && m_curConn->logicStatus == ConnectStatus::deleted) m_curConn = m_curConn->pNext;
	Connect* nextConn = (m_curConn != NULL) ? m_curConn->pNext : NULL;  // 必须在wait之前获取，wait之后如果m_curConn被移出队列则next已经为空了
	
	// 2）接着等待连接处理线程处理完
	m_connMgr->waitConnecter();
	if (m_curConn == NULL || m_curConn->readSocketTimes == 0)  // wait之后存在m_curConn可能被连接线程移出队列了
	{
		m_curConn = (nextConn != NULL) ? nextConn : m_connMgr->getMsgConnectList();  // 必须确保连接在队列中，否则会错误、无限循环
		if (m_curConn == NULL) return NotNetData;
	}

	unsigned int dataSize = 0;
	Connect* const beginConn = m_curConn;
	while (true)
	{
		if (m_curConn->connStatus == ConnectStatus::normal && m_curConn->logicStatus == ConnectStatus::normal)
		{
			dataSize = CDataHandler::getCanReadDataSize(m_curConn);  // 当前可以从连接读取的数据量
			if (dataSize > 0)
			{
				m_connMgr->addToMsgQueue(m_curConn, ConnectOpt::EAddToQueue);  // 正常则加入消息队列
				m_connMgr->setConnectNormal(m_curConn);                        // 有数据则填写活跃时间点
				
				if (dataSize > len) dataSize = len;
				len = CDataHandler::read(m_curConn, data, dataSize);
				
				conn = m_curConn;
				m_curConn = m_curConn->pNext;  // 切换到下一个连接，以便均衡遍历所有连接
				return OptSuccess;
			}
		}
		
		// 删除销毁已经关闭的连接
		else if (m_curConn->connStatus == ConnectStatus::closed && m_curConn->logicStatus != ConnectStatus::deleted)
		{
			CDataHandler::releaseWrtBuff(m_connMgr, m_curConn);
			m_curConn->logicStatus = ConnectStatus::deleted;  // 标志此连接为无效连接，可删除了
		}
		
		m_curConn = (m_curConn->pNext != NULL) ? m_curConn->pNext : m_connMgr->getMsgConnectList();
		if (m_curConn == beginConn) break;  // 回到循环起点了，无可处理的消息
	}
	
	return NotNetData;
}

ReturnValue CDataTransmit::send(Connect* conn, const char* data, const unsigned int len, bool isNeedWriteMsgHeader)
{
	if (conn->connStatus != ConnectStatus::normal || conn->logicStatus != ConnectStatus::normal)  // 无效连接的数据
	{
		close(conn);
		return ConnectException;
	}

	return CDataHandler::write(m_connMgr, conn, data, len) ? OptSuccess : SndDataFailed;        // 2) 再把逻辑层的数据写到连接里
}

void CDataTransmit::close(Connect* conn)
{
	if (conn == m_curConn) m_curConn = m_curConn->pNext;
	
	if (CDataHandler::getCanWriteDataSize(conn) > 0)
	{
		conn->logicStatus = ConnectStatus::sendClose;  // 连接里的数据发送完毕后关闭连接
	}
	else
	{
    	CDataHandler::releaseWrtBuff(m_connMgr, conn);
		conn->logicStatus = ConnectStatus::deleted;    // 标志此连接为无效连接，可删除了
	}
}

}

