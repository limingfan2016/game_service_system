
/* author : limingfan
 * date : 2016.11.02
 * description : 统计数据
 */

#ifndef CSTATISTICS_DATA_H
#define CSTATISTICS_DATA_H

#include <unordered_map>


namespace NProject
{

// 统计的物品（目前只支持统计话费&积分券）
struct StatisticsGoods
{
	unsigned int phoneFare;                  // 话费
	unsigned int scoreVoucher;               // 积分券
	
	StatisticsGoods() : phoneFare(0), scoreVoucher(0) {};
};

typedef std::unordered_map<int, StatisticsGoods> StatisticsCategoryToGoodsMap;  // 统计类别ID到数据的映射


// 统计数据
class CStatisticsData
{
public:
	CStatisticsData();
	~CStatisticsData();
	
public:
    // propType 如果是非话费&积分券则不会统计
	void addStatGoods(int categoryId, int propType, unsigned int count);
	
	const StatisticsGoods& getStatGoods(int categoryId);  // 第一次获取不存在则自动添加
	void resetStatGoods(int categoryId, unsigned int value = 0);
	void removeStatGoods(int categoryId);
	
	const StatisticsCategoryToGoodsMap& getStatGoodsMap();
	void resetAllStatGoods(unsigned int value = 0);
	void removeAllStatGoods();
	
private:
    StatisticsCategoryToGoodsMap m_statGoodsMap;
};


typedef std::unordered_map<int, CStatisticsData> IdToStatisticsDataMap;  // 统计ID到数据映射

// 管理统计数据
class CStatisticsDataManager
{
public:
	CStatisticsDataManager();
	~CStatisticsDataManager();
	
public:
    // propType 如果是非话费&积分券则不会统计
    // id 默认为 0，表示给所有相关的统计数据都增加，不为 0，则只给指定的统计ID数据增加统计量
	void addStatGoods(int categoryId, int propType, unsigned int count, unsigned int id = 0);
	
	void addStatData(int id);
	CStatisticsData& getStatData(int id);  // 第一次获取不存在则自动添加
	void resetStatGoods(int id, int categoryId, unsigned int value = 0);
	void resetStatData(int id, unsigned int value = 0);
	void removeStatData(int id);
	
	const IdToStatisticsDataMap& getStatDataMap();
	void resetAllStatData(unsigned int value = 0);
	void removeAllStatData();
	
private:
    IdToStatisticsDataMap m_statDataMap;
};


}

#endif // CSTATISTICS_DATA_H
