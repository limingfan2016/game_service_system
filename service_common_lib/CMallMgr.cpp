
/* author : limingfan
 * date : 2015.08.03
 * description : 商城数据
 */

#include <string.h>
#include "CMallMgr.h"


using namespace NCommon;

namespace NProject
{

CMallMgr::CMallMgr()
{
	memset(&m_mallCfgData, 0, sizeof(m_mallCfgData));
}

CMallMgr::~CMallMgr()
{
    memset(&m_mallCfgData, 0, sizeof(m_mallCfgData));
}

bool CMallMgr::setMallData(const char* data, const unsigned int len)
{
	if (data != NULL && len > 1 && len < NFrame::MaxMsgLen)
	{
		memcpy(m_mallCfgData.data, data, len);
		m_mallCfgData.len = len;

		return true;
	}
	
	return false;
}

bool CMallMgr::getMallItemInfo(const int type, com_protocol::ItemInfo& itemInfo)
{
	com_protocol::GetMallConfigRsp mallCfgData;
	if (!getMallConfig(mallCfgData)) return false;

	if (type == MallConfigData::ItemType::Gold)
	{
		for (int i = 0; i < mallCfgData.gold_list_size(); ++i)
		{
			const com_protocol::ItemInfo& iInfo = mallCfgData.gold_list(i).gold();
			if (iInfo.id() == itemInfo.id())
			{
				itemInfo.set_buy_price(iInfo.buy_price());
				itemInfo.set_num(iInfo.num());
				itemInfo.set_flag(iInfo.flag());
				return true;
			}
		}
	}
	else if (type == MallConfigData::ItemType::Item)
	{
		for (int i = 0; i < mallCfgData.item_list_size(); ++i)
		{
			const com_protocol::ItemInfo& iInfo = mallCfgData.item_list(i).item();
			if (iInfo.id() == itemInfo.id())
			{
				itemInfo.set_buy_price(iInfo.buy_price());
				itemInfo.set_num(iInfo.num());
				itemInfo.set_flag(iInfo.flag());
				return true;
			}
		}
	}
	
	OptWarnLog("can not find the item info, type = %d, id = %d", type, itemInfo.id());
	
	return false;
}

bool CMallMgr::getMallConfig(com_protocol::GetMallConfigRsp& mallCfgData)
{
	if (mallCfgData.ParseFromArray(m_mallCfgData.data, m_mallCfgData.len)) return true;

	OptErrorLog("CMallMgr::getMallConfig, ParseFromArray mall config message error, data len = %d", m_mallCfgData.len);
	return false;
}

}

