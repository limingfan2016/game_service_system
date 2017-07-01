
/* author : limingfan
 * date : 2016.11.02
 * description : 统计数据
 */

#include "CommonType.h"
#include "CStatisticsData.h"


using namespace NCommon;

namespace NProject
{

// 统计数据
CStatisticsData::CStatisticsData()
{
}

CStatisticsData::~CStatisticsData()
{
}

// propType 如果是非话费&积分券则不会统计
void CStatisticsData::addStatGoods(int categoryId, int propType, unsigned int count)
{
	if (propType == EPropType::PropScores)
	{
		m_statGoodsMap[categoryId].scoreVoucher += count;
	}
	else if (propType == EPropType::PropPhoneFareValue)
	{
		m_statGoodsMap[categoryId].phoneFare += count;
	}
}

const StatisticsGoods& CStatisticsData::getStatGoods(int categoryId)  // 第一次获取不存在则自动添加
{
	return m_statGoodsMap[categoryId];
}

void CStatisticsData::resetStatGoods(int categoryId, unsigned int value)
{
	StatisticsGoods& goods = m_statGoodsMap[categoryId];
	goods.phoneFare = value;
	goods.scoreVoucher = value;
}

void CStatisticsData::removeStatGoods(int categoryId)
{
	m_statGoodsMap.erase(categoryId);
}

const StatisticsCategoryToGoodsMap& CStatisticsData::getStatGoodsMap()
{
	return m_statGoodsMap;
}

void CStatisticsData::resetAllStatGoods(unsigned int value)
{
	for (StatisticsCategoryToGoodsMap::iterator it = m_statGoodsMap.begin(); it != m_statGoodsMap.end(); ++it)
	{
		it->second.phoneFare = value;
		it->second.scoreVoucher = value;
	}
}

void CStatisticsData::removeAllStatGoods()
{
	m_statGoodsMap.clear();
}


// 管理统计数据
CStatisticsDataManager::CStatisticsDataManager()
{
}

CStatisticsDataManager::~CStatisticsDataManager()
{
}

// propType 如果是非话费&积分券则不会统计
// id 默认为 0，表示给所有相关的统计数据都增加，不为 0，则只给指定的统计ID数据增加统计量
void CStatisticsDataManager::addStatGoods(int categoryId, int propType, unsigned int count, unsigned int id)
{
	if (id == 0)
	{
		for (IdToStatisticsDataMap::iterator it = m_statDataMap.begin(); it != m_statDataMap.end(); ++it) it->second.addStatGoods(categoryId, propType, count);
	}
	else
	{
		m_statDataMap[id].addStatGoods(categoryId, propType, count);
	}
}

void CStatisticsDataManager::addStatData(int id)
{
	if (id != 0) m_statDataMap[id] = CStatisticsData();
}

CStatisticsData& CStatisticsDataManager::getStatData(int id)  // 第一次获取不存在则自动添加
{
	return m_statDataMap[id];
}

void CStatisticsDataManager::resetStatGoods(int id, int categoryId, unsigned int value)
{
	m_statDataMap[id].resetStatGoods(categoryId, value);
}

void CStatisticsDataManager::resetStatData(int id, unsigned int value)
{
	m_statDataMap[id].resetAllStatGoods(value);
}

void CStatisticsDataManager::removeStatData(int id)
{
	m_statDataMap.erase(id);
}

const IdToStatisticsDataMap& CStatisticsDataManager::getStatDataMap()
{
	return m_statDataMap;
}

void CStatisticsDataManager::resetAllStatData(unsigned int value)
{
	for (IdToStatisticsDataMap::iterator it = m_statDataMap.begin(); it != m_statDataMap.end(); ++it) it->second.resetAllStatGoods(value);
}

void CStatisticsDataManager::removeAllStatData()
{
	m_statDataMap.clear();
}

}

