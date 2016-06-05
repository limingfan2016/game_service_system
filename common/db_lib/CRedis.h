
/* author : liuxu
 * date : 2015.01.30
 * description :  redis client api
 */
 
#ifndef __CREDIS_H__
#define __CREDIS_H__

#include <hiredis/hiredis.h>
#include <string>
#include <vector>
#include "base/ErrorCode.h"

/* redis 接口注意事项
 * 1、使用get接口时，当返回结果为0（长度）时，该key有可能不存在，也有可能key对应的值为空，若想确定是否为空，需调用对应的 has接口，
      除非你的业务能保证不存在非空值的value.
 * 2、使用has接口时，当返回结果 大于等于1 时，才说明该Key存在，因为当出现网络错误时会返回负数，切记不要这样if(has())，得这样使用if（has() > 0）
 */
namespace NDBOpt
{

	class CRedis
	{
	public:
		CRedis();
		~CRedis();

		//服务器连接
		bool connectSvr(const char *ip, int port, unsigned int timeout = 1500); //timeout 单位ms
		void disconnectSvr();

		//同步数据到磁盘
		int asynSave();//异步，会立即返回
		int save();//同步，耗时长，慎用

		//redis 原始接口
		void* command(const char *format, ...);
		void* commandArgv(const std::vector<std::string> &vstrOper, const std::vector<std::string> &vstrParam);

		//redis基本操作（string)（数值类型得格式化成string）
		int setKey(const char *key, int keyLen, const char *value, int valueLen, unsigned int lifeTime = 0);//lifeTime,生存时间(秒)，0代表永不删除
		int setKeyRange(const char *key, int keyLen, int offset, const char *value, int valueLen);//offset为偏移量，下标从0开始
		int append(const char *key, int keyLen, const char *value, int valueLen);//在原有的key追加，若不存在则相当于setKey
		int setKeyLifeTime(const char *key, int keyLen, unsigned int time = 0); //time 单位秒，=0代表永不删除

		int getKey(const char *key, int keyLen, char *value, int valueLen);//结果存value,返回长度，0不存在
		int getLen(const char *key, int keyLen);//返回结果：0代表不存在
		int getKeyByRange(const char *key, int keyLen, int start, int end, char *value, int valueLen);//start,end, -1代表最后1个，-2代表倒数第2个
		int getKeyRemainLifeTime(const char *key, int keyLen);//返回结果：-2:key不存在，-1:永久，其它则返回剩余时间(秒)
		int getKeyType(const char *key, int keyLen, char *valueType, int valueLen);//返回结果在valueType

		int delKey(const char *key, int keyLen);//删除成功返回1，不存在返回0
		int hasKey(const char *key, int keyLen);//key存在返回1，不存在返回0
		int incrByFloat(const char *key, int keyLen, double addValue, double &retValue);//执行成功返回0

		int setMultiKey(const std::vector<std::string> &vstrKeyValue);//返回0 成功
		int getMultiKey(const std::vector<std::string> &vstrKey, std::vector<std::string> &vstrValue);//返回0 成功,结果在vstrValue中
		int delMultiKey(const std::vector<std::string> &vstrKey);//返回删除成功的个数

		//hash表
		int setHField(const char *key, int keyLen, const char *field, int fieldLen, const char *value, int valueLen);//返回0 成功
		int getHField(const char *key, int keyLen, const char *field, int fieldLen, char *value, int valueLen);//返回长度
		int delHField(const char *key, int keyLen, const char *field, int fieldLen);//删除成功返回1，不存在返回0

		int hasHField(const char *key, int keyLen, const char *field, int fieldLen);//key存在返回1，不存在返回0
		int incrHByFloat(const char *key, int keyLen, const char *field, int fieldLen, double addValue, double retValue);//执行成功返回0
		int getHAll(const char *key, int keyLen, std::vector<std::string> &vstrFieldValue);//执行成功返回0
		int getHFieldCount(const char *key, int keyLen);//返回field个数

		int setMultiHField(const char *key, int keyLen, const std::vector<std::string> &vstrFieldValue);//返回0 成功
		int getMultiHField(const char *key, int keyLen, const std::vector<std::string> &vstrField, std::vector<std::string> &vstrValue);//返回0 成功
		int delMultiHField(const char *key, int keyLen, const std::vector<std::string> &vstrField);//返回删除成功的个数
		
		//列表
		int lpushList(const char *key, int keyLen, const char *value, int valueLen);//返回列表长度
		int lpopList(const char *key, int keyLen, char *value, int valueLen);//返回value长度
		int rpushList(const char *key, int keyLen, const char *value, int valueLen);//返回列表长度
		int rpopList(const char *key, int keyLen, char *value, int valueLen);//返回value长度

		int indexList(const char *key, int keyLen, int index, char *value, int valueLen);//返回value长度
		int lenList(const char *key, int keyLen);//成功长度
		int rangeList(const char *key, int keyLen, int start, int end, std::vector<std::string> &vstrList);//成功返回0
		int setList(const char *key, int keyLen, int index, const char *value, int valueLen);//成功返回0，set效率为线性的，一般只用做首尾的修改（也就是index为0或-1）

	private:
		// 禁止拷贝、赋值
		CRedis(const CRedis&);
		CRedis& operator =(const CRedis&);
		bool reConnectSvr();
		void *execCommand(const char *cmd, int len);

	private:
		redisContext *m_predisContext;
	
		std::string m_strIP;
		int m_nPort;
		unsigned int m_nTimeout;
	};

}

#endif // CREDIS_H


