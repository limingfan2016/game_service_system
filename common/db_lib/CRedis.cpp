
/* author : liuxu
* date : 2015.01.30
* description : redis client api
*/

#include <sys/time.h>
#include <math.h>
#include <queue>
#include <unistd.h>
#include "CRedis.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "base/MacroDefine.h"

using namespace NCommon;

#define REDIS_NORMAL_JUDGE(reply) 					\
if (NULL == reply)									\
{													\
	return NErrorCode::ERedis::NetError;			\
}													\
if (REDIS_REPLY_ERROR == reply->type)				\
{													\
	freeReplyObject(reply);							\
	return NErrorCode::ERedis::NormalError;			\
}													\


#ifdef __cplusplus
extern "C" {
#endif
int __redisAppendCommand(redisContext *c, const char *cmd, size_t len);
#ifdef __cplusplus
}
#endif

/* Calculate the number of bytes needed to represent an integer as string. */
static int intlen(int i) {
	int len = 0;
	if (i < 0) {
		len++;
		i = -i;
	}
	do {
		len++;
		i /= 10;
	} while (i);
	return len;
}

/* Helper that calculates the bulk length given a certain string length. */
static size_t bulklen(size_t len) {
	return 1 + intlen(len) + 2 + len + 2;
}

namespace NDBOpt
{
	CRedis::CRedis()
	{
		m_nPort = 0;
		m_nTimeout = 0;
		m_predisContext = NULL;
	}

	CRedis::~CRedis()
	{

	}

	/*************************************************
	Function:  connectSvr
	Description: 连接redis服务
	Input: 
		ip：例 "192.168.1.2"
		port: 端口号 例 6379
		timeout:设置超时时间，单位 ms
	Output: 无
	Return: 成功返回true
	Others: 无
	*************************************************/
	bool CRedis::connectSvr(const char *ip, int port, unsigned int timeout)
	{
		m_strIP = ip;
		m_nPort = port;
		m_nTimeout = timeout;

		struct timeval tvTimeout;
		tvTimeout.tv_sec = timeout / 1000;
		tvTimeout.tv_usec = timeout % 1000 * 1000;

		m_predisContext = redisConnectWithTimeout(ip, port, tvTimeout);
		if (m_predisContext == NULL || m_predisContext->err)
		{
			if (m_predisContext)
			{
				ReleaseErrorLog("redis Connection ip: %s, port: %d error: %s", ip, port, m_predisContext->errstr);
				disconnectSvr();
			}
			else
			{
				ReleaseErrorLog("redis Connection error: can't allocate redis context, ip: %s, port: %d", ip, port);
			}
			return false;
		}

		return true;
	}

	/*************************************************
	Function:  disconnectSvr
	Description: 断开原有的redis连接
	Input:无
	Output: 无
	Return: 无
	Others: 无
	*************************************************/
	void CRedis::disconnectSvr()
	{
		redisFree(m_predisContext);
		m_predisContext = NULL;
	}

	/*************************************************
	Function:  asynSave
	Description: fork一个子进程，同步数据到磁盘，立即返回
	Input:无
	Output: 无
	Return: 成功返回0
	Others: 需使用 LASTSAVE 命令查看同步结果
	*************************************************/
	int CRedis::asynSave()
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("BGSAVE");
		REDIS_NORMAL_JUDGE(reply);

		freeReplyObject(reply);
		return 0;
	}

	/*************************************************
	Function:  save
	Description: 阻塞方式 同步数据到磁盘
	Input:无
	Output: 无
	Return: 成功返回0
	Others: 当key较多时，很慢
	*************************************************/
	int CRedis::save()
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("SAVE");
		REDIS_NORMAL_JUDGE(reply);

		if (0 == strncasecmp("ok", reply->str, 3))
		{
			freeReplyObject(reply);
			return 0;
		}

		freeReplyObject(reply);
		
		return NErrorCode::ERedis::NormalError;
	}

	/*************************************************
	Function:  command
	Description: redis原始接口,若失败，会尝试一次重连
	Input:格式化参数，接受可变参数
	Output: 无
	Return: 执行结果，一般为 redisReply*
	Others: 无
	*************************************************/
	void* CRedis::command(const char *format, ...)
	{
		va_list ap;
		void *reply = NULL;

		if (m_predisContext)
		{
			va_start(ap, format);
			reply = redisvCommand(m_predisContext, format, ap);
			va_end(ap);
		}

		if (NULL == reply)
		{
			if (reConnectSvr())
			{
				va_start(ap, format);
				reply = redisvCommand(m_predisContext, format, ap);
				va_end(ap);
			}
		}

		return reply;
	}

	void* CRedis::commandArgv(const std::vector<std::string> &vstrOper, const std::vector<std::string> &vstrParam)
	{
		char *cmd = NULL;
		int len = 0;
		void *reply = NULL;
		int pos;
		int totlen = 0;

		//格式化命令
		/* Calculate number of bytes needed for the command */
		totlen = 1 + intlen(vstrOper.size() + vstrParam.size()) + 2;
		for (unsigned int j = 0; j < vstrOper.size(); j++)
		{
			totlen += bulklen(vstrOper[j].length());
		}
		for (unsigned int j = 0; j < vstrParam.size(); j++)
		{
			totlen += bulklen(vstrParam[j].length());
		}

		/* Build the command at protocol level */
		cmd = (char *)malloc(totlen + 1);
		if (cmd == NULL)
			return NULL;
		
		pos = sprintf(cmd, "*%zu\r\n", vstrOper.size() + vstrParam.size());
		//push cmd
		for (unsigned int j = 0; j < vstrOper.size(); j++) {
			pos += sprintf(cmd + pos, "$%zu\r\n", vstrOper[j].length());
			memcpy(cmd + pos, vstrOper[j].c_str(), vstrOper[j].length());
			pos += vstrOper[j].length();
			cmd[pos++] = '\r';
			cmd[pos++] = '\n';
		}

		//push param
		for (unsigned int j = 0; j < vstrParam.size(); j++) {
			pos += sprintf(cmd + pos, "$%zu\r\n", vstrParam[j].length());
			memcpy(cmd + pos, vstrParam[j].c_str(), vstrParam[j].length());
			pos += vstrParam[j].length();
			cmd[pos++] = '\r';
			cmd[pos++] = '\n';
		}
		assert(pos == totlen);
		cmd[pos] = '\0';

		len = totlen;
		//执行命令
		reply = execCommand(cmd, len);
		if (NULL == reply)
		{
			if (reConnectSvr())
			{
				reply = execCommand(cmd, len);
			}
		}

		free(cmd);
		return reply;
	}

	void* CRedis::execCommand(const char *cmd, int len)
	{
		void *reply = NULL;
		if (NULL == m_predisContext)
		{
			return NULL;
		}

		//执行命令
		if (__redisAppendCommand(m_predisContext, cmd, len) != REDIS_OK)
		{
			return NULL;
		}

		//获取执行结果
		if (m_predisContext->flags & REDIS_BLOCK)
		{
			if (redisGetReply(m_predisContext, &reply) != REDIS_OK)
			{
				return NULL;
			}
		}

		return reply;
	}

	/*************************************************
	Function:  setKey
	Description: 设置一对key value,若已存在直接覆盖
	Input:
		lifeTime:生存时间(秒)，0代表永不删除 
	Output: 无
	Return: 成功返回0
	Others: 无
	*************************************************/
	int CRedis::setKey(const char *key, int keyLen, const char *value, int valueLen, unsigned int lifeTime)
	{
		redisReply *reply = NULL;
		if (0 == lifeTime)
		{
			reply = (redisReply *)command("SET %b %b", key, (size_t)keyLen, value, (size_t)valueLen);
		}
		else
		{
			reply = (redisReply *)command("SET %b %b EX %u", key, (size_t)keyLen, value, (size_t)valueLen, lifeTime);
		}
		REDIS_NORMAL_JUDGE(reply);

		freeReplyObject(reply);
		return 0;
	}

	/*************************************************
	Function:  setKeyRange
	Description: 用 value 参数覆写(overwrite)给定 key 所储存的字符串值，从偏移量 offset 开始。
	Input:
		offset: 偏移量，下标从0开始
	Output: 无
	Return: 修改后value的长度
	Others: 如果给定 key 原来储存的字符串长度比偏移量小，那么原字符和偏移量之间的空白将用零字节("\x00" )来填充。
	*************************************************/
	int CRedis::setKeyRange(const char *key, int keyLen, int offset, const char *value, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("SETRANGE %b %u %b", key, (size_t)keyLen, offset, value, (size_t)valueLen);
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->integer;
		freeReplyObject(reply);

		return len;
	}

	/*************************************************
	Function:  append
	Description: 在原有的key追加，若key不存在则相当于setKey
	Input:
	offset: 偏移量，下标从0开始
	Output: 无
	Return: 修改后value的长度
	Others: 如果给定 key 原来储存的字符串长度比偏移量小，那么原字符和偏移量之间的空白将用零字节("\x00" )来填充。
	*************************************************/
	int CRedis::append(const char *key, int keyLen, const char *value, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("APPEND %b %b", key, (size_t)keyLen, value, (size_t)valueLen);
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->integer;
		freeReplyObject(reply);

		return len;
	}

	/*************************************************
	Function:  setKeyLifeTime
	Description: 设置key的生存时间
	Input:
		time:单位秒，0代表永不删除
	Output: 无
	Return: 成功返回0
	Others: 无
	*************************************************/
	int CRedis::setKeyLifeTime(const char *key, int keyLen, unsigned int time)
	{
		redisReply *reply = NULL;
		if (0 == time)
		{
			reply = (redisReply *)command("PERSIST %b", key, (size_t)keyLen);
		}
		else
		{
			reply = (redisReply *)command("EXPIRE %b %u", key, keyLen, time);
		}
		REDIS_NORMAL_JUDGE(reply);

		int ret = 0;
		freeReplyObject(reply);

		return ret;
	}


	/*************************************************
	Function:  getKey
	Description: 获取key对应的value
	Input:
		valueLen:value的size
	Output: 
		value:用于接收 key对应的value，若存储空间不够，返回失败
	Return: 返回长度，0不存在
	Others: 无
	*************************************************/
	int CRedis::getKey(const char *key, int keyLen, char *value, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("GET %b", key, (size_t)keyLen);
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->len;
		if (len > valueLen) //空间不够
		{
			len = NErrorCode::ERedis::SpaceNotEnough;
		}
		else
		{
			memcpy(value, reply->str, len);
		}
		freeReplyObject(reply);

		return len;
	}

	/*************************************************
	Function:  getLen
	Description: 获取key对应的value的长度
	Input:
	Output:
	Return: 返回长度，0代表不存在
	Others: 无
	*************************************************/
	int CRedis::getLen(const char *key, int keyLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("STRLEN %b", key, (size_t)keyLen);
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->integer;
		freeReplyObject(reply);

		return len;
	}


	/*************************************************
	Function:  getKeyByRange
	Description: 获取key对应start end区间的数据
	Input:
		start:-1代表最后1个，-2代表倒数第2个.
		end: -1代表最后1个，-2代表倒数第2个
		valueLen:value的size
	Output:
		value：获取的数据
	Return: 返回长度，0代表key不存在或end在start前
	Others: 通过保证子字符串的值域(range)不超过实际字符串的值域来处理超出范围的值域请求。
	*************************************************/
	int CRedis::getKeyByRange(const char *key, int keyLen, int start, int end, char *value, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("GETRANGE %b %d %d", key, (size_t)keyLen, start, end);
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->len;

		if (len > valueLen)
		{
			len = NErrorCode::ERedis::SpaceNotEnough;
		}
		else
		{
			memcpy(value, reply->str, len);
		}
		freeReplyObject(reply);

		return len;
	}

	/*************************************************
	Function:  getKeyRemainLifeTime
	Description: 获取key的剩余时间
	Input:
	Output:
	Return: -2:key不存在，-1:永久，其它则返回剩余时间(秒)
	Others: 无
	*************************************************/
	int CRedis::getKeyRemainLifeTime(const char *key, int keyLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("TTL %b", key, (size_t)keyLen);
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->integer;
		freeReplyObject(reply);

		return len;
	}

	/*************************************************
	Function:  getKeyType
	Description: 获取key的类型
	Input:
		valueLen:valueType的size
	Output:
		valueType: none(key不存在) string(字符串)	list(列表)	set(集合) zset(有序集) hash(哈希表)
	Return: valueType size
	Others: 无
	*************************************************/
	int CRedis::getKeyType(const char *key, int keyLen, char *valueType, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("TYPE %b", key, (size_t)keyLen);
		REDIS_NORMAL_JUDGE(reply);

		int len = reply->len;
		if (reply->len + 1 > valueLen)
		{
			len = NErrorCode::ERedis::SpaceNotEnough;
		}
		else
		{
			memcpy(valueType, reply->str, reply->len + 1);
		}
		freeReplyObject(reply);

		return len;
	}

	/*************************************************
	Function:  delKey
	Description: 删除key
	Input:
	Output:
	Return: 删除成功返回1，不存在返回0
	Others: 无
	*************************************************/
	int CRedis::delKey(const char *key, int keyLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("DEL %b", key, (size_t)keyLen);
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	/*************************************************
	Function:  hasKey
	Description: 判断key是否存在
	Input:
	Output:
	Return: key存在返回1，不存在返回0
	Others: 无
	*************************************************/
	int CRedis::hasKey(const char *key, int keyLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("EXISTS %b", key, (size_t)keyLen);
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	/*************************************************
	Function:  incrByFloat
	Description: 对key做加法
	Input:
		addValue：需要相加的值。（可为负数）
	Output:
		retValue：用于接收相加后的结果
	Return: 执行成功返回0
	Others: 如果 key 不存在，那么会先将 key 的值设为 0 ，再执行加法操作。
	*************************************************/
	int CRedis::incrByFloat(const char *key, int keyLen, double addValue, double &retValue)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("INCRBYFLOAT %b %f", key, (size_t)keyLen, addValue);
		REDIS_NORMAL_JUDGE(reply);

		retValue = atof(reply->str);
		freeReplyObject(reply);

		return 0;
	}

	//返回0 成功
	int  CRedis::setMultiKey(const std::vector<std::string> &vstrKeyValue)
	{
		redisReply *reply = NULL;
		std::vector<std::string> vstrOper;
		vstrOper.push_back(std::string("MSET"));
		reply = (redisReply *)commandArgv(vstrOper, vstrKeyValue);
		REDIS_NORMAL_JUDGE(reply);

		freeReplyObject(reply);
		return 0;
	}

	int CRedis::getMultiKey(const std::vector<std::string> &vstrKey, std::vector<std::string> &vstrValue)
	{
		redisReply *reply = NULL;
		std::vector<std::string> vstrOper;
		vstrOper.push_back(std::string("MGET"));
		reply = (redisReply *)commandArgv(vstrOper, vstrKey);
		REDIS_NORMAL_JUDGE(reply);
		//获取结果
		vstrValue.clear();
		for (unsigned int i = 0; i < reply->elements; i++)
		{
			vstrValue.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
		}

		freeReplyObject(reply);
		return 0;
	}

	int CRedis::delMultiKey(const std::vector<std::string> &vstrKey)
	{
		redisReply *reply = NULL;
		std::vector<std::string> vstrOper;
		vstrOper.push_back(std::string("DEL"));
		reply = (redisReply *)commandArgv(vstrOper, vstrKey);
		REDIS_NORMAL_JUDGE(reply);

		int len = reply->integer;
		freeReplyObject(reply);
		return len;
	}

	int CRedis::setHField(const char *key, int keyLen, const char *field, int fieldLen, const char *value, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("HSET %b %b %b", key, (size_t)keyLen, field, (size_t)fieldLen, value, (size_t)valueLen);
		REDIS_NORMAL_JUDGE(reply);

		freeReplyObject(reply);

		return 0;
	}

	int CRedis::getHField(const char *key, int keyLen, const char *field, int fieldLen, char *value, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("HGET %b %b", key, (size_t)keyLen, field, (size_t)fieldLen);
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->len;
		if (len > valueLen) //空间不够
		{
			len = NErrorCode::ERedis::SpaceNotEnough;
		}
		else
		{
			memcpy(value, reply->str, len);
		}
		freeReplyObject(reply);

		return len;
	}

	int CRedis::delHField(const char *key, int keyLen, const char *field, int fieldLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("HDEL %b %b", key, (size_t)keyLen, field, (size_t)fieldLen);
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}


	int CRedis::hasHField(const char *key, int keyLen, const char *field, int fieldLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("HEXISTS %b %b", key, (size_t)keyLen, field, (size_t)fieldLen);
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	int CRedis::incrHByFloat(const char *key, int keyLen, const char *field, int fieldLen, double addValue, double retValue)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("HINCRBYFLOAT %b %b %f", key, (size_t)keyLen, field, (size_t)fieldLen, addValue);
		REDIS_NORMAL_JUDGE(reply);

		retValue = atof(reply->str);
		freeReplyObject(reply);

		return 0;
	}

	int CRedis::getHAll(const char *key, int keyLen, std::vector<std::string> &vstrFieldValue)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("HGETALL %b", key, (size_t)keyLen);
		REDIS_NORMAL_JUDGE(reply);
		//获取结果
		vstrFieldValue.clear();
		for (unsigned int i = 0; i < reply->elements; i++)
		{
			vstrFieldValue.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
		}

		freeReplyObject(reply);
		return 0;
	}

	int CRedis::getHFieldCount(const char *key, int keyLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("HLEN %b", key, (size_t)keyLen);
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	int CRedis::setMultiHField(const char *key, int keyLen, const std::vector<std::string> &vstrFieldValue)
	{
		redisReply *reply = NULL;
		std::vector<std::string> vstrOper;
		vstrOper.push_back(std::string("HMSET"));
		vstrOper.push_back(std::string(key, keyLen));
		reply = (redisReply *)commandArgv(vstrOper, vstrFieldValue);
		REDIS_NORMAL_JUDGE(reply);

		freeReplyObject(reply);
		return 0;
	}

	int CRedis::getMultiHField(const char *key, int keyLen, const std::vector<std::string> &vstrField, std::vector<std::string> &vstrValue)
	{
		redisReply *reply = NULL;
		std::vector<std::string> vstrOper;
		vstrOper.push_back(std::string("HMGET"));
		vstrOper.push_back(std::string(key, keyLen));
		reply = (redisReply *)commandArgv(vstrOper, vstrField);
		REDIS_NORMAL_JUDGE(reply);
		//获取结果
		vstrValue.clear();
		for (unsigned int i = 0; i < reply->elements; i++)
		{
			vstrValue.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
		}

		freeReplyObject(reply);
		return 0;
	}

	int CRedis::delMultiHField(const char *key, int keyLen, const std::vector<std::string> &vstrField)
	{
		redisReply *reply = NULL;
		std::vector<std::string> vstrOper;
		vstrOper.push_back(std::string("HDEL"));
		vstrOper.push_back(std::string(key, keyLen));
		reply = (redisReply *)commandArgv(vstrOper, vstrField);
		REDIS_NORMAL_JUDGE(reply);

		int len = reply->integer;
		freeReplyObject(reply);
		return len;
	}


	int CRedis::lpushList(const char *key, int keyLen, const char *value, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("LPUSH %b %b", key, (size_t)keyLen, value, (size_t)valueLen);
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	//返回value长度
	int CRedis::lpopList(const char *key, int keyLen, char *value, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("LPOP %b", key, (size_t)keyLen);
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->len;
		if (len > valueLen) //空间不够
		{
			len = NErrorCode::ERedis::SpaceNotEnough;
		}
		else
		{
			memcpy(value, reply->str, len);
		}
		freeReplyObject(reply);

		return len;
	}

	int CRedis::rpushList(const char *key, int keyLen, const char *value, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("RPUSH %b %b", key, (size_t)keyLen, value, (size_t)valueLen);
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	//返回value长度
	int CRedis::rpopList(const char *key, int keyLen, char *value, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("RPOP %b", key, (size_t)keyLen);
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->len;
		if (len > valueLen) //空间不够
		{
			len = NErrorCode::ERedis::SpaceNotEnough;
		}
		else
		{
			memcpy(value, reply->str, len);
		}
		freeReplyObject(reply);

		return len;
	}


	int CRedis::indexList(const char *key, int keyLen, int index, char *value, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("LINDEX %b %d", key, (size_t)keyLen, index);
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->len;
		if (len > valueLen) //空间不够
		{
			len = NErrorCode::ERedis::SpaceNotEnough;
		}
		else
		{
			memcpy(value, reply->str, len);
		}
		freeReplyObject(reply);

		return len;
	}

	int CRedis::lenList(const char *key, int keyLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("LLEN %b", key, (size_t)keyLen);
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	int CRedis::rangeList(const char *key, int keyLen, int start, int end, std::vector<std::string> &vstrList)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("LRANGE %b %d %d", key, (size_t)keyLen, start, end);
		REDIS_NORMAL_JUDGE(reply);
		//获取结果
		vstrList.clear();
		for (unsigned int i = 0; i < reply->elements; i++)
		{
			vstrList.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
		}
		freeReplyObject(reply);

		return 0;
	}

	int CRedis::setList(const char *key, int keyLen, int index, const char *value, int valueLen)
	{
		redisReply *reply = NULL;
		reply = (redisReply *)command("LSET %b %d %b", key, (size_t)keyLen, index, value, (size_t)valueLen);
		REDIS_NORMAL_JUDGE(reply);

		return 0;
	}

	bool CRedis::reConnectSvr()
	{
		disconnectSvr();
		return connectSvr(m_strIP.c_str(), m_nPort, m_nTimeout);
	}
}

