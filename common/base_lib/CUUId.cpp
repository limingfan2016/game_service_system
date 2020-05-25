
/* author : admin
 * date : 2014.11.19
 * description : UUID唯一生成管理实现
 */
 
#include <string.h>

#include "CUUId.h"
#include "MacroDefine.h"
#include "ErrorCode.h"


using namespace NErrorCode;

namespace NCommon
{

unsigned short CUUId::s_serverIdx = 0;
unsigned short CUUId::s_len = 0;
unsigned int* CUUId::s_type2Idx = NULL;


int CUUId::init(unsigned short serverIdx, unsigned int type2Idx[], unsigned short len)
{
	if (serverIdx < 1 || type2Idx == NULL || len < 1)
	{
		return InvalidParam;
	}
	
	NEW_ARRAY(s_type2Idx, unsigned int[len]);
	if (s_type2Idx == NULL)
	{
		return NoMemory;
	}
	
	s_serverIdx = serverIdx;
	s_len = len;
	memcpy(s_type2Idx, type2Idx, sizeof(unsigned int) * len);
	
	return Success;
}

void CUUId::unInit()
{
	s_serverIdx = 0;
	s_len = 0;
	DELETE_ARRAY(s_type2Idx);
}

uuid_type CUUId::generateId(IdTypeVal t_val)
{
	static uuid_type id = 0;  // 先简单实现
	++id;
	return id;
}

uuid_type CUUId::generateUserId()
{
	return generateId(user);
}

}

