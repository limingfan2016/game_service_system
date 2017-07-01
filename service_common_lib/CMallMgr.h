
/* author : limingfan
 * date : 2015.08.03
 * description : 商城数据
 */

#ifndef CMALL_MANAGER_H
#define CMALL_MANAGER_H

#include "service_common_module.pb.h"
#include "base/MacroDefine.h"
#include "SrvFrame/UserType.h"


namespace NProject
{

// 商城数据
struct MallConfigData
{
	char data[NFrame::MaxMsgLen];
	unsigned int len;

    // 0金币；1道具；2话费卡
	enum ItemType {Gold = 0, Item = 1, PhoneCard = 2,};
};


// 商城数据管理
class CMallMgr
{
public:
	CMallMgr();
	~CMallMgr();
	
public:
    bool setMallData(const char* data, const unsigned int len);

    bool getMallItemInfo(const int type, com_protocol::ItemInfo& itemInfo);
	bool getMallConfig(com_protocol::GetMallConfigRsp& mallCfgData);
	
private:
    MallConfigData m_mallCfgData;

	
DISABLE_COPY_ASSIGN(CMallMgr);
};

}

#endif // CMALL_MANAGER_H
