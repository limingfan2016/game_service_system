
/* author : admin
 * date : 2014.11.19
 * description : UUID唯一生成管理实现
 */
 
#ifndef CUUID_H
#define CUUID_H

#include "Type.h"


namespace NCommon
{

// UUID 类型字段，各个类型值定义
enum IdTypeVal
{
	user = 0,         // 用户类型
};


// UUID 工具类
class CUUId
{
public:
	static int init(unsigned short serverIdx, unsigned int type2Idx[], unsigned short len);
	static void unInit();
	
public:
	static uuid_type generateId(IdTypeVal t_val);
	static uuid_type generateUserId();

private:
    // 禁止创建CUUId对象
	CUUId();
	~CUUId();
	CUUId(const CUUId&);
	CUUId& operator =(const CUUId&);

private:
	static unsigned short s_serverIdx;
	static unsigned short s_len;
	static unsigned int* s_type2Idx;
};

}

#endif // CUUID_H
