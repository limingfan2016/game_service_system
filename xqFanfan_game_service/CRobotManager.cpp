
/* author : limingfan
 * date : 2016.12.27
 * description : 翻翻棋机器人实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "CRobotManager.h"
#include "CXiangQiSrv.h"
#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;


namespace NPlatformService
{

// 棋子值排序函数
static bool chessPiecesValueSort(const SRobotMoveChessPieces& cpVal1, const SRobotMoveChessPieces& cpVal2)
{
	const unordered_map<unsigned int, unsigned int> chessPiecesValueCfgMap = XiangQiConfig::config::getConfigValue().chess_pieces_value;
	return (chessPiecesValueCfgMap.at(cpVal1.beEatCP) > chessPiecesValueCfgMap.at(cpVal2.beEatCP));
}


CRobotManager::CRobotManager()
{
	m_msgHander = NULL;
}

CRobotManager::~CRobotManager()
{
	m_msgHander = NULL;
}

bool CRobotManager::load(CSrvMsgHandler* msgHandler)
{
	m_msgHander = msgHandler;
	
	generateRobotInfo();
	
	return true;
}

void CRobotManager::unLoad()
{
	
}

void CRobotManager::updateConfig()
{
	
}

XQUserData* CRobotManager::createRobotData(uint64_t gold, unsigned int playMode, unsigned int arenaId)
{
	const unsigned int robotCount = m_freeRobots.size();
	if (robotCount < 1) return NULL;
	
	XQUserData* rUd = m_msgHander->createUserData();
	memset(rUd, 0, sizeof(XQUserData));
	
	const unsigned int randIdx = getRandNumber(0, robotCount - 1);
	SRobotInfo& robotInfo = m_freeRobots[randIdx];
	strncpy(rUd->username, robotInfo.id, MaxConnectIdLen - 1);
	strncpy(rUd->nickname, robotInfo.name, MaxConnectIdLen - 1);
	rUd->usernameLen = strlen(rUd->username);
	
	// 目前头像ID随机0到9表示
	const unsigned int MaxPortraitId = 9;
	rUd->portrait_id = getRandNumber(0, MaxPortraitId);
	
	const XiangQiConfig::RobotConfig& robotCfg = XiangQiConfig::config::getConfigValue().robot_cfg;
	rUd->cur_gold = getRandNumber(robotCfg.min_player_gold_rate * gold, robotCfg.max_player_gold_rate * gold);
	
	rUd->playMode = playMode;
	rUd->arenaId = arenaId;
	
	if (robotCount > 1 && randIdx < (robotCount - 1))
	{
		// 移动最后一个到当前位置
		strncpy(robotInfo.id, m_freeRobots[robotCount - 1].id, MaxConnectIdLen - 1);
	    strncpy(robotInfo.name, m_freeRobots[robotCount - 1].name, MaxConnectIdLen - 1);
	}
	m_freeRobots.pop_back();  // 删除最后一个
	
	m_robotId2Data[rUd->username] = rUd;
	return rUd;
}

void CRobotManager::releaseRobotData(XQUserData* xqUD)
{
	m_freeRobots.push_back(SRobotInfo(xqUD->username, xqUD->nickname));
	m_robotId2Data.erase(xqUD->username);
	
	m_msgHander->destroyUserData(xqUD);
}

bool CRobotManager::isRobot(const char* id)
{
	return id != NULL && *id == 'R';  // 机器人ID前缀为 R
}

XQUserData* CRobotManager::getRobotData(const char* robotId)
{
	RobotIdToData::iterator it = m_robotId2Data.find(robotId);
	return (it != m_robotId2Data.end()) ? it->second : NULL;
}

// 机器人下完一盘棋
void CRobotManager::finishPlayChess(const XQUserData* ud, const unsigned int winGold, const unsigned int lostGold)
{
	SRobotWinLostRecord& rbtWLRecord = m_robotWinLostRecord[ud->playMode][ud->arenaId];
	
	rbtWLRecord.allGold += (winGold + lostGold);
	rbtWLRecord.winGold += winGold;

    map<unsigned int, XiangQiConfig::ChessArenaConfig>::const_iterator arenaCfgIt = XiangQiConfig::config::getConfigValue().classic_arena_cfg.find(ud->arenaId);
	if (arenaCfgIt != XiangQiConfig::config::getConfigValue().classic_arena_cfg.end())
	{
		const unsigned int winRatePercent = getWinRatePercent(ud);
		if (winRatePercent >= arenaCfgIt->second.win_rate_percent)
		{
			OptInfoLog("robot play chess record normal, play mode = %u, arena = %u, all gold = %lu, win gold = %lu, win rate percent = %u, config percent = %u",
			ud->playMode, ud->arenaId, rbtWLRecord.allGold, rbtWLRecord.winGold, winRatePercent, arenaCfgIt->second.win_rate_percent);
		}
		else
		{
			OptWarnLog("robot play chess record warn, play mode = %u, arena = %u, all gold = %lu, win gold = %lu, win rate percent = %u, config percent = %u",
			ud->playMode, ud->arenaId, rbtWLRecord.allGold, rbtWLRecord.winGold, winRatePercent, arenaCfgIt->second.win_rate_percent);
		}
	}
}

// 是否需要作弊
bool CRobotManager::needCheat(const XQUserData* ud, const XQChessPieces* xqCP, const short cheatSide, const unsigned int baseRate)
{
	map<unsigned int, XiangQiConfig::ChessArenaConfig>::const_iterator arenaCfgIt = XiangQiConfig::config::getConfigValue().classic_arena_cfg.find(ud->arenaId);
	if (arenaCfgIt == XiangQiConfig::config::getConfigValue().classic_arena_cfg.end() || getWinRatePercent(ud) < arenaCfgIt->second.win_rate_percent)  // 获取机器人赢率百分比
	{
		// 作弊概率=基础概率*(敌方血量/我方血量)
		const unsigned int cheatRate = baseRate * ((double)(xqCP->curLife[(cheatSide + 1) % EXQSideFlag::ESideFlagCount]) / (double)(xqCP->curLife[cheatSide % EXQSideFlag::ESideFlagCount]));
		return getPositiveRandNumber(1, PercentValue) <= cheatRate;
	}

	return false;
}

// 机器人下棋走子
void CRobotManager::robotPlayChessPieces(const char* robotId, const XQChessPieces* xqCP)
{
	if (!isRobot(robotId)) return;
	
	const XQUserData* xqUD = getRobotData(robotId);
	if (xqUD == NULL) return;
	
	const time_t curSecs = time(NULL);  // 当前时间秒数
	
	// 机器人的金币赢率如果低于配置值则启动作弊流程调整我方明炮的目标棋子
	const XiangQiConfig::RobotConfig& rbtCfg = XiangQiConfig::config::getConfigValue().robot_cfg;
	if (needCheat(xqUD, xqCP, xqCP->curOptSide, rbtCfg.cheat_cfg.our_pao_cheat_base_rate)) robotPlayCheatAdjustCP((XQChessPieces*)xqCP);

    // 找出双方可以互相吃的棋子信息
	SRobotMoveChessPiecesVector ourSideBeEatCP;       // 我方可以被对方吃掉的棋子的列表
	SRobotMoveChessPiecesVector opponentSideBeEatCP;  // 对方可以被我方吃掉的棋子的列表
	SRobotMoveChessPiecesVector ourChessPosition;     // 我方不能吃对方的棋子
	ChessPiecesPositions closedChessPosition;         // 未翻开的棋子
	SRobotMoveChessPiecesVector ourBeEatCanMoveCP;    // 我方被吃并且可以移动的棋子列表
	findCanEatChessPieces(xqCP->value, xqCP->curOptSide, ourSideBeEatCP, opponentSideBeEatCP, ourChessPosition, closedChessPosition);
	
	// 是否可以吃掉对方的棋子
	const unordered_map<unsigned int, unsigned int> chessPiecesValueCfgMap = XiangQiConfig::config::getConfigValue().chess_pieces_value;
	const int opponentCurHp = xqCP->curLife[(xqCP->curOptSide + 1) % EXQSideFlag::ESideFlagCount];
	const unsigned int maxBeEatValue = ourSideBeEatCP.empty() ? 0 : chessPiecesValueCfgMap.at(ourSideBeEatCP[0].beEatCP);
    const unsigned int ourSideBeEatCount = getBeEatChessPiecesCount(ourSideBeEatCP);  // 获取被吃棋子的数量，去掉重复被吃的棋子
	com_protocol::ClientPlayChessPiecesNotify plCPNotify;
	unsigned int eatOpponentIndex = 0;
    int moveOurSideOpt = eatChessPieces(xqCP->value, xqCP->curOptSide, opponentCurHp, ourSideBeEatCP, opponentSideBeEatCP, true, eatOpponentIndex, -1);
	if (moveOurSideOpt == EMoveChessPiecesOpt::EMoveBeEatCP)  // 先移动当前被吃掉的棋子，然后再检查移动之后是否会被吃掉
	{
		OptErrorLog("CanDodge Test find advantage our side be eat count = %d, maxBeEatValue = %d", ourSideBeEatCount, maxBeEatValue);
        findAdvantageChessPiecesPosition(xqCP, ourSideBeEatCP, maxBeEatValue, ourSideBeEatCount, plCPNotify, &ourBeEatCanMoveCP); // 先优先移动被吃的棋子，找出占据优势的棋子移动位置
		
		if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
		{
			OptErrorLog("Robot play chess error3, current side = %d, value = %d, src position = %d, dst position = %d",
			xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
		}
		
		// 接着再尝试移动其他未被吃的棋子
		if (!plCPNotify.has_src_position())
		{
			OptErrorLog("CanDodge Test find advantage our side can not eat count = %d, be eat count = %d, maxBeEatValue = %d", ourChessPosition.size(), ourSideBeEatCount, maxBeEatValue);
			findAdvantageChessPiecesPosition(xqCP, ourChessPosition, 0, ourSideBeEatCount, plCPNotify);
		}
		
		if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
		{
			OptErrorLog("Robot play chess error4, current side = %d, value = %d, src position = %d, dst position = %d",
			xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
		}
	}
	else if (moveOurSideOpt == EMoveChessPiecesOpt::EMoveOurCP)  // 移动棋子
	{
		OptErrorLog("CanDodge Test find advantage our side need move count = %d, be eat count = %d, maxBeEatValue = %d", ourChessPosition.size(), ourSideBeEatCount, maxBeEatValue);
		findAdvantageChessPiecesPosition(xqCP, ourChessPosition, 0, ourSideBeEatCount, plCPNotify); // 找出占据优势的棋子移动位置
		
		if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
		{
			OptErrorLog("Robot play chess error5, current side = %d, value = %d, src position = %d, dst position = %d",
			xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
		}
	}
	else
	{
		// 可以直接吃掉对方的棋子了
		plCPNotify.set_src_position(opponentSideBeEatCP[eatOpponentIndex].srcPosition);
		plCPNotify.set_dst_position(opponentSideBeEatCP[eatOpponentIndex].dstPosition);
		
		OptErrorLog("Robot Test Eat1, value = %d, src position = %d, dst position = %d", xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
		
		if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
		{
			OptErrorLog("Robot play chess error6, current side = %d, value = %d, src position = %d, dst position = %d",
			xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
		}
	}
	
	// 没有找到占据优势的棋子的移动位置则尝试移动其他棋子
	while (!plCPNotify.has_src_position())
	{
		// 1）ourSideBeEatCP，先考虑从被吃的棋子里重新找出优势可移动的棋子
		// 此次查找考虑了被吃棋子里的自然防御策略，排除掉自然防御的被吃棋子后重新运算看是否具备优势策略
		// 在自然防御条件下，找出可以移动的最优棋子
		SRobotMoveChessPieces moveCPInfo;               // 最优势的棋子
        EFindMoveChessResult fmcResult = findCanMoveMostAdvantageCP(xqCP, ourBeEatCanMoveCP, moveCPInfo);
		if (fmcResult == EFindMoveChessResult::ECanEatCP)
		{
			plCPNotify.set_src_position(moveCPInfo.srcPosition);
		    plCPNotify.set_dst_position(moveCPInfo.dstPosition);
			
			OptErrorLog("Robot Test Eat123, value = %d, src position = %d, dst position = %d", xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			
			if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
			{
				OptErrorLog("Robot play chess error123, current side = %d, value = %d, src position = %d, dst position = %d",
				xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			}
			
			break;
		}
		
		SRobotMoveChessPiecesVector allMoveCPVector;    // 所有可移动的棋子
		SRobotMoveChessPiecesVector firstMoveCPVector;  // 可以优先移动的棋子
		SRobotMoveChessPiecesVector firstOpenCPVector;  // 可以优先翻开的棋子
		
		// 2）找到可以移动的优势棋子
        if (findCanMoveChessPieces(xqCP, ourChessPosition, moveCPInfo, firstMoveCPVector, allMoveCPVector))
		{
			plCPNotify.set_src_position(moveCPInfo.srcPosition);
		    plCPNotify.set_dst_position(moveCPInfo.dstPosition);
			
			OptErrorLog("Robot Test First Move, value = %d, src position = %d, dst position = %d", xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			
			if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
			{
				OptErrorLog("Robot play chess error7, current side = %d, value = %d, src position = %d, dst position = %d",
				xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			}
			
			break;
		}
	
		// 3）找到可以翻开的优势棋子
        if (findCanOpenChessPieces(xqCP, closedChessPosition, moveCPInfo, firstOpenCPVector))
		{
			plCPNotify.set_src_position(moveCPInfo.srcPosition);
			
			/*
			if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
			{
				OptErrorLog("Robot play chess error8, current side = %d, value = %d, src position = %d, dst position = %d",
				xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			}
			*/
			break;
		}
		
		// 4）如果解除不了我方的大子（比如将、士）被吃，但同时也存在我方的棋子可以安全的吃掉对方的其他棋子，则此时直接吃掉对方的其他棋子
		if (!opponentSideBeEatCP.empty())
		{
			// 找出双方可以互相吃的棋子信息
			SRobotMoveChessPiecesVector tmpOurSideBeEatCP;       // 我方可以被对方吃掉的棋子的列表
			SRobotMoveChessPiecesVector tmpOpponentSideBeEatCP;  // 对方可以被我方吃掉的棋子的列表
			SRobotMoveChessPiecesVector tmpOurChessPosition;     // 我方不能吃对方的棋子
			vector<short> tmpClosedChessPosition;                // 未翻开的棋子
			short afterMoveValue[MaxChessPieces] = {0};          // 移动后的棋子布局
			const unsigned int cpValueSize = sizeof(afterMoveValue);

            const SRobotMoveChessPieces* canEatClosedCP = NULL;
			for (SRobotMoveChessPiecesVector::const_iterator osbeCPIt = opponentSideBeEatCP.begin(); osbeCPIt != opponentSideBeEatCP.end(); ++osbeCPIt)
			{
				// 吃掉对方棋子后再检查是否会被对方吃掉
				if (canEatChessPieces(xqCP->value, xqCP->curOptSide, opponentCurHp, *osbeCPIt, afterMoveValue, cpValueSize,
				                      tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition)  // 占据优势则返回可以吃
					|| (chessPiecesValueCfgMap.at(tmpOurSideBeEatCP[0].beEatCP) <= maxBeEatValue && getBeEatChessPiecesCount(tmpOurSideBeEatCP) <= ourSideBeEatCount))
				{
					if (osbeCPIt->isOpen)
					{
						// 优先吃掉已经翻开了的棋子
						plCPNotify.set_src_position(osbeCPIt->srcPosition);
						plCPNotify.set_dst_position(osbeCPIt->dstPosition);
						
						break;
					}
					
					if (canEatClosedCP == NULL) canEatClosedCP = &(*osbeCPIt);
				}
			}
			
			if (!plCPNotify.has_src_position() && canEatClosedCP != NULL)
			{
				plCPNotify.set_src_position(canEatClosedCP->srcPosition);
				plCPNotify.set_dst_position(canEatClosedCP->dstPosition);
			}
			
			if (plCPNotify.has_src_position())
			{
				OptErrorLog("Robot Test Eat2, value = %d, src position = %d, dst position = %d", xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
				break;
			}
		}
		
		// 5）都没有找到优势棋子则随机翻开 优先翻开的棋子
		if (!firstOpenCPVector.empty())
		{
			plCPNotify.set_src_position(firstOpenCPVector[getRandNumber(0, firstOpenCPVector.size() - 1)].srcPosition);
			
			break;
			
			/*
			if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
			{
				OptErrorLog("Robot play chess error9, current side = %d, value = %d, src position = %d, dst position = %d",
				xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			}
			*/ 
		}
		
		// 6）在自然防御条件下，找出可以移动的最优棋子，必须遍历棋子重新运算，找出最佳的可移动棋子
		fmcResult = findCanMoveMostAdvantageCP(xqCP, firstMoveCPVector, moveCPInfo);
		if (fmcResult != EFindMoveChessResult::ECanNotMoveCP)
		{
			plCPNotify.set_src_position(moveCPInfo.srcPosition);
			plCPNotify.set_dst_position(moveCPInfo.dstPosition);
			
			OptErrorLog("Robot Test Move1234, value = %d, src position = %d, dst position = %d, result = %d",
			xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position(), fmcResult);
			
			if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
			{
				OptErrorLog("Robot play chess error1234, current side = %d, value = %d, src position = %d, dst position = %d",
				xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			}
			
			break;
		}
		
		// 7）接着随机翻开 未翻开的棋子
		if (!closedChessPosition.empty())
		{
			plCPNotify.set_src_position(closedChessPosition[getRandNumber(0, closedChessPosition.size() - 1)]);
			
			break;
			
			/*
			if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
			{
				OptErrorLog("Robot play chess error11, current side = %d, value = %d, src position = %d, dst position = %d",
				xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			}
			*/ 
		}
		
		// 8）在自然防御条件下，找出可以移动的最优棋子，必须遍历棋子重新运算，找出最佳的可移动棋子
		fmcResult = findCanMoveMostAdvantageCP(xqCP, allMoveCPVector, moveCPInfo);
		if (fmcResult != EFindMoveChessResult::ECanNotMoveCP)
		{
			plCPNotify.set_src_position(moveCPInfo.srcPosition);
			plCPNotify.set_dst_position(moveCPInfo.dstPosition);
			
			OptErrorLog("Robot Test Move12345, value = %d, src position = %d, dst position = %d, result = %d",
			xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position(), fmcResult);
			
			if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
			{
				OptErrorLog("Robot play chess error12345, current side = %d, value = %d, src position = %d, dst position = %d",
				xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			}
			
			break;
		}
		
		// 9）接下来随机移动 优先移动的棋子
		if (!firstMoveCPVector.empty())
		{
			moveCPInfo = firstMoveCPVector[getRandNumber(0, firstMoveCPVector.size() - 1)];
			plCPNotify.set_src_position(moveCPInfo.srcPosition);
		    plCPNotify.set_dst_position(moveCPInfo.dstPosition);
			
			OptErrorLog("Robot Test Advantage Move, value = %d, src position = %d, dst position = %d", xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			
			if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
			{
				OptErrorLog("Robot play chess error10, current side = %d, value = %d, src position = %d, dst position = %d",
				xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			}
			
			break;
		}
		
		// 10）都还不行那只能随机移动 所有可移动的棋子
		if (!allMoveCPVector.empty())
		{
			moveCPInfo = allMoveCPVector[getRandNumber(0, allMoveCPVector.size() - 1)];
			plCPNotify.set_src_position(moveCPInfo.srcPosition);
		    plCPNotify.set_dst_position(moveCPInfo.dstPosition);
			
			OptErrorLog("Robot Test Rand Move, value = %d, src position = %d, dst position = %d", xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			
			if (plCPNotify.has_src_position() && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
			{
				OptErrorLog("Robot play chess error12, current side = %d, value = %d, src position = %d, dst position = %d",
				xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
			}
			
			break;
		}
		
		if (!plCPNotify.has_src_position())
		{
			// 居然没有找到可移动的棋子，靠崩盘了啊
			OptErrorLog("robot move chess pieces error, id = %s, side = %d, life = %d, opponent = %s",
			robotId, xqCP->curOptSide, xqCP->curLife[xqCP->curOptSide], xqCP->sideUserName[(xqCP->curOptSide + 1) % EXQSideFlag::ESideFlagCount]);
			
			return;  // 此情况只能等待超时结算了，机器人输定了
		}
		
		break;
	}
	
	if (plCPNotify.has_src_position() && xqCP->value[plCPNotify.src_position()] < com_protocol::EChessPiecesValue::ENoneQiZiClose
	    && !m_msgHander->isSameSide(xqCP->curOptSide, xqCP->value[plCPNotify.src_position()]))
	{
		OptErrorLog("Robot play chess error13, current side = %d, value = %d, src position = %d, dst position = %d",
		xqCP->curOptSide, xqCP->value[plCPNotify.src_position()], plCPNotify.src_position(), plCPNotify.dst_position());
	}
	
	if ((time(NULL) - curSecs) < rbtCfg.min_wait_play)
	{
		URobotMovePosition rbtMovePos;
		rbtMovePos.position.src = plCPNotify.src_position();
		rbtMovePos.position.dst = plCPNotify.has_dst_position() ? plCPNotify.dst_position() : -1;
		m_msgHander->setTimer(getRandNumber(rbtCfg.min_wait_play, rbtCfg.max_wait_play) * MillisecondUnit, (TimerHandler)&CRobotManager::robotMoveChessPieces,
		atoi(robotId + 1), (void*)rbtMovePos.positionId, 1, this);
		
		return;
	}

    int errorCode = Opt_Success;
	m_msgHander->userPlayChessPieces(xqUD, plCPNotify, errorCode);
	if (errorCode != Opt_Success)
	{
		OptErrorLog("robot play chess pieces error, id = %s, side = %d, life = %d, opponent = %s, code = %d",
		robotId, xqCP->curOptSide, xqCP->curLife[xqCP->curOptSide], xqCP->sideUserName[(xqCP->curOptSide + 1) % EXQSideFlag::ESideFlagCount], errorCode);
	}
}

// 机器人下棋走子
void CRobotManager::robotMoveChessPieces(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	char robotId[MaxConnectIdLen] = {0};
	generateRobotId(userId, robotId, MaxConnectIdLen - 1);  // 生成机器人ID
	
	const XQUserData* xqUD = getRobotData(robotId);
	if (xqUD == NULL) return;

	URobotMovePosition rbtMovePos;
	rbtMovePos.positionId = (long long)param;
	com_protocol::ClientPlayChessPiecesNotify plCPNotify;
	plCPNotify.set_src_position(rbtMovePos.position.src);
	if (rbtMovePos.position.dst >= MinChessPiecesPosition) plCPNotify.set_dst_position(rbtMovePos.position.dst);
	
	int errorCode = Opt_Success;
	m_msgHander->userPlayChessPieces(xqUD, plCPNotify, errorCode);
	if (errorCode != Opt_Success)
	{
		OptErrorLog("robot play chess pieces error, id = %s, code = %d", robotId, errorCode);
	}
}

// 找到可以移动的棋子
bool CRobotManager::findCanMoveChessPieces(const XQChessPieces* xqCP, const SRobotMoveChessPiecesVector& ourChessPosition,
                                           SRobotMoveChessPieces& moveCPInfo, SRobotMoveChessPiecesVector& firstMoveCPVector, SRobotMoveChessPiecesVector& allMoveCPVector)
{
	moveCPInfo.beEatCP = com_protocol::EChessPiecesValue::ENoneQiZiOpen;
	if (!ourChessPosition.empty())
	{
		const unordered_map<unsigned int, unsigned int>& cpGrades = XiangQiConfig::config::getConfigValue().chess_pieces_value;
		short afterMoveValue[MaxChessPieces] = {0};          // 移动后的棋子布局
		SRobotMoveChessPiecesVector tmpOurSideBeEatCP;       // 我方可以被对方吃掉的棋子的列表
		SRobotMoveChessPiecesVector tmpOpponentSideBeEatCP;  // 对方可以被我方吃掉的棋子的列表
		SRobotMoveChessPiecesVector tmpOurChessPosition;     // 我方不能吃对方的棋子
		vector<short> tmpClosedChessPosition;                // 未翻开的棋子

		const int opponetCurHp = xqCP->curLife[(xqCP->curOptSide + 1) % EXQSideFlag::ESideFlagCount];
		ChessPiecesPositions newDstPosition;
		unsigned int eatOpponentIndex = 0;
		unsigned int opponentSideBeEatCount = 0;
		unsigned int ourSideBeEatCount = MaxChessPieces;
		const SRestrictMoveInfo& restictMvInfo = xqCP->restrictMoveInfo;
		for (SRobotMoveChessPiecesVector::const_iterator ourSideCPIt = ourChessPosition.begin(); ourSideCPIt != ourChessPosition.end(); ++ourSideCPIt)
		{
		    if ((ourSideCPIt->dstPosition != restictMvInfo.srcPosition || restictMvInfo.stepCount < restictMvInfo.robotChaseStep)
			    && canMovePosition(xqCP->value, ourSideCPIt->dstPosition, newDstPosition))  // 该棋子可以移动的位置
			{
				// 检查移动之后是否占据优势
				memcpy(afterMoveValue, xqCP->value, sizeof(afterMoveValue));
				afterMoveValue[ourSideCPIt->dstPosition] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;  // 变成空位置
				for (ChessPiecesPositions::const_iterator newPosIt = newDstPosition.begin(); newPosIt != newDstPosition.end(); ++newPosIt)
				{
					afterMoveValue[*newPosIt] = ourSideCPIt->beEatCP;
					findCanEatChessPieces(afterMoveValue, xqCP->curOptSide, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition);
					if (eatChessPieces(afterMoveValue, xqCP->curOptSide, opponetCurHp, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, false, eatOpponentIndex, *newPosIt) == 0)  // 占据优势了
					{
						const unsigned int opponentCPValue = cpGrades.at(tmpOpponentSideBeEatCP[eatOpponentIndex].beEatCP);
						const unsigned int lastCPValue = cpGrades.at(moveCPInfo.beEatCP);
						if (opponentCPValue > lastCPValue  // 1）找到血量伤害更大的
							|| (opponentCPValue == lastCPValue
							    && (tmpOpponentSideBeEatCP.size() > opponentSideBeEatCount  // 2）血量相同时找到敌方棋子被伤害个数最多的
		                            || (tmpOpponentSideBeEatCP.size() == opponentSideBeEatCount && tmpOurSideBeEatCP.size() < ourSideBeEatCount)))  // 3）找到我方棋子被伤害个数最少的
						    )
						{
							moveCPInfo.srcPosition = ourSideCPIt->dstPosition;
							moveCPInfo.dstPosition = *newPosIt;
							moveCPInfo.beEatCP = tmpOpponentSideBeEatCP[eatOpponentIndex].beEatCP;
							opponentSideBeEatCount = tmpOpponentSideBeEatCP.size();
							ourSideBeEatCount = tmpOurSideBeEatCP.size();
						}
					}
					else if (moveCPInfo.beEatCP == com_protocol::EChessPiecesValue::ENoneQiZiOpen && tmpOurSideBeEatCP.empty())  // 移动之后我方没有被吃的棋子
					{
						firstMoveCPVector.push_back(SRobotMoveChessPieces(ourSideCPIt->dstPosition, *newPosIt, ourSideCPIt->beEatCP));
					}
					
					allMoveCPVector.push_back(SRobotMoveChessPieces(ourSideCPIt->dstPosition, *newPosIt, ourSideCPIt->beEatCP));
					
					afterMoveValue[*newPosIt] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;  // 还原成空位置
				}
			}
		}
	}
	
	return moveCPInfo.beEatCP != com_protocol::EChessPiecesValue::ENoneQiZiOpen;
}

// 找到可以翻开的棋子
bool CRobotManager::findCanOpenChessPieces(const XQChessPieces* xqCP, const ChessPiecesPositions& closedChessPosition,
										   SRobotMoveChessPieces& moveCPInfo, SRobotMoveChessPiecesVector& firstOpenCPVector)
{
	moveCPInfo.beEatCP = com_protocol::EChessPiecesValue::ENoneQiZiOpen;
	if (!closedChessPosition.empty())  // 移动棋子没有优势则选择翻开棋子
	{
		// plCPNotify.set_src_position(closedChessPosition[getRandNumber(0, closedChessPosition.size() - 1)]);  // 随机翻开一个棋子
		
		const unordered_map<unsigned int, unsigned int>& cpGrades = XiangQiConfig::config::getConfigValue().chess_pieces_value;
		short afterMoveValue[MaxChessPieces] = {0};          // 移动后的棋子布局
		SRobotMoveChessPiecesVector tmpOurSideBeEatCP;       // 我方可以被对方吃掉的棋子的列表
		SRobotMoveChessPiecesVector tmpOpponentSideBeEatCP;  // 对方可以被我方吃掉的棋子的列表
		SRobotMoveChessPiecesVector tmpOurChessPosition;     // 我方不能吃对方的棋子
		vector<short> tmpClosedChessPosition;                // 未翻开的棋子
		
		// 检查翻开该棋子之后是否占据优势（这里属于作弊行为啦！）
		const int opponetCurHp = xqCP->curLife[(xqCP->curOptSide + 1) % EXQSideFlag::ESideFlagCount];
		unsigned int eatOpponentIndex = 0;
		unsigned int opponentSideBeEatCount = 0;
		unsigned int ourSideBeEatCount = MaxChessPieces;
		memcpy(afterMoveValue, xqCP->value, sizeof(afterMoveValue));
		for (ChessPiecesPositions::const_iterator newPosIt = closedChessPosition.begin(); newPosIt != closedChessPosition.end(); ++newPosIt)
		{
			afterMoveValue[*newPosIt] -= ClosedChessPiecesValue;
			findCanEatChessPieces(afterMoveValue, xqCP->curOptSide, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition);
			if (eatChessPieces(afterMoveValue, xqCP->curOptSide, opponetCurHp, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, false, eatOpponentIndex, *newPosIt) == 0)  // 占据优势了
			{
				const unsigned int opponentCPValue = cpGrades.at(tmpOpponentSideBeEatCP[eatOpponentIndex].beEatCP);
				const unsigned int lastCPValue = cpGrades.at(moveCPInfo.beEatCP);
				
				if (opponentCPValue > lastCPValue  // 1）找到血量伤害更大的
					|| (opponentCPValue == lastCPValue
						&& (tmpOpponentSideBeEatCP.size() > opponentSideBeEatCount  // 2）血量相同时找到敌方棋子被伤害个数最多的
							|| (tmpOpponentSideBeEatCP.size() == opponentSideBeEatCount && tmpOurSideBeEatCP.size() < ourSideBeEatCount)))  // 3）找到我方棋子被伤害个数最少的
					)
				{
					moveCPInfo.srcPosition = *newPosIt;
					moveCPInfo.beEatCP = tmpOpponentSideBeEatCP[eatOpponentIndex].beEatCP;
					opponentSideBeEatCount = tmpOpponentSideBeEatCP.size();
					ourSideBeEatCount = tmpOurSideBeEatCP.size();
				}
			}
			else if (moveCPInfo.beEatCP == com_protocol::EChessPiecesValue::ENoneQiZiOpen && tmpOurSideBeEatCP.empty())  // 翻开之后我方没有被吃的棋子
			{
				firstOpenCPVector.push_back(SRobotMoveChessPieces(*newPosIt, -1, afterMoveValue[*newPosIt]));
			}
			
			afterMoveValue[*newPosIt] += ClosedChessPiecesValue;  // 还原成未翻开的棋子
		}
	}
	
	return moveCPInfo.beEatCP != com_protocol::EChessPiecesValue::ENoneQiZiOpen;
}

// 找出占据优势的棋子移动位置
bool CRobotManager::findAdvantageChessPiecesPosition(const XQChessPieces* xqCP, const SRobotMoveChessPiecesVector& ourSideCP, const unsigned int maxBeEatValue, const unsigned int beEatCount,
                                                     com_protocol::ClientPlayChessPiecesNotify& plCPNotify, SRobotMoveChessPiecesVector* canMoveCPs)
{
	if (ourSideCP.empty()) return false;
	
	// 找出双方可以互相吃的棋子信息
	SRobotMoveChessPiecesVector tmpOurSideBeEatCP;       // 我方可以被对方吃掉的棋子的列表
	SRobotMoveChessPiecesVector tmpOpponentSideBeEatCP;  // 对方可以被我方吃掉的棋子的列表
	SRobotMoveChessPiecesVector tmpOurChessPosition;     // 我方不能吃对方的棋子
	vector<short> tmpClosedChessPosition;                // 未翻开的棋子
	vector<short> newPositions;                          // 找出棋子可以移动的位置
	short afterMoveValue[MaxChessPieces] = {0};          // 移动后的棋子布局
	short dstValue = 0;                                  // 移动到新位置的棋子值
	
	const bool needCanMoveCP = (canMoveCPs != NULL);     // 搞效率优化
	const unordered_map<unsigned int, unsigned int>& cpGrades = XiangQiConfig::config::getConfigValue().chess_pieces_value;
	const int dodgeSteps = XiangQiConfig::config::getConfigValue().robot_cfg.dodge_steps;
	const unsigned short curOptSide = xqCP->curOptSide;
	const int opponetCurHp = xqCP->curLife[(curOptSide + 1) % EXQSideFlag::ESideFlagCount];
	unsigned int eatOpponentIndex = 0;
	SRobotMoveChessPieces moveCPInfo(-1, -1, com_protocol::EChessPiecesValue::ENoneQiZiOpen);
	unsigned int opponentSideBeEatCount = 0;
	unsigned int ourSideBeEatCount = MaxChessPieces;
	const SRestrictMoveInfo& restictMvInfo = xqCP->restrictMoveInfo;
	for (SRobotMoveChessPiecesVector::const_iterator ourSideCPIt = ourSideCP.begin(); ourSideCPIt != ourSideCP.end(); ++ourSideCPIt)
	{
		if ((ourSideCPIt->dstPosition != restictMvInfo.srcPosition || restictMvInfo.stepCount < restictMvInfo.robotChaseStep)
			&& canMovePosition(xqCP->value, ourSideCPIt->dstPosition, newPositions))  // 该棋子可以移动的位置
		{
			// 先检查被吃的棋子本身是不是已经具备了防御，比如对方如果吃掉该棋子，则对方也会被我方吃掉
			bool naturalProtect = false;
			if (ourSideCPIt->srcPosition >= 0)
			{
				memcpy(afterMoveValue, xqCP->value, sizeof(afterMoveValue));
				afterMoveValue[ourSideCPIt->dstPosition] = afterMoveValue[ourSideCPIt->srcPosition];        // 对方吃掉我方棋子
				afterMoveValue[ourSideCPIt->srcPosition] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;  // 变成空位置
				findCanEatChessPieces(afterMoveValue, curOptSide, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition);
				if (eatChessPieces(afterMoveValue, curOptSide, opponetCurHp, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, true, eatOpponentIndex, -1) == 0         // 是否可以吃掉对方的棋子
				    && cpGrades.at(tmpOpponentSideBeEatCP[eatOpponentIndex].beEatCP) >= cpGrades.at(ourSideCPIt->beEatCP)) naturalProtect = true;  // 本身具备了自然防护
			}
			
			memcpy(afterMoveValue, xqCP->value, sizeof(afterMoveValue));
			dstValue = afterMoveValue[ourSideCPIt->dstPosition];  // 移动到新位置的棋子值
			afterMoveValue[ourSideCPIt->dstPosition] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;  // 变成空位置
			
			
			OptErrorLog("CanDodge Test move our side chess pieces, value = %d, dstPosition = %d, srcPosition = %d, srcValue = %d, naturalProtect = %d",
			dstValue, ourSideCPIt->dstPosition, ourSideCPIt->srcPosition, (ourSideCPIt->srcPosition >= 0) ? afterMoveValue[ourSideCPIt->srcPosition] : -1, naturalProtect);
			
			
			for (vector<short>::const_iterator posIt = newPositions.begin(); posIt != newPositions.end(); ++posIt)
			{
				// 我方棋子移动之后重新运算
				afterMoveValue[*posIt] = dstValue;
				findCanEatChessPieces(afterMoveValue, curOptSide, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition);
				if (eatChessPieces(afterMoveValue, curOptSide, opponetCurHp, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, false, eatOpponentIndex, *posIt) == 0) // 是否可以吃掉对方的棋子
				{
					const unsigned int opponentCPValue = cpGrades.at(tmpOpponentSideBeEatCP[eatOpponentIndex].beEatCP);
					const unsigned int lastCPValue = cpGrades.at(moveCPInfo.beEatCP);
					
					if (opponentCPValue > lastCPValue  // 1）找到血量伤害更大的
						|| (opponentCPValue == lastCPValue
							&& (tmpOpponentSideBeEatCP.size() > opponentSideBeEatCount  // 2）血量相同时找到敌方棋子被伤害个数最多的
								|| (tmpOpponentSideBeEatCP.size() == opponentSideBeEatCount && tmpOurSideBeEatCP.size() < ourSideBeEatCount)))  // 3）找到我方棋子被伤害个数最少的
						)
					{
						moveCPInfo.srcPosition = ourSideCPIt->dstPosition;
						moveCPInfo.dstPosition = *posIt;
						moveCPInfo.beEatCP = tmpOpponentSideBeEatCP[eatOpponentIndex].beEatCP;
						opponentSideBeEatCount = tmpOpponentSideBeEatCP.size();
						ourSideBeEatCount = tmpOurSideBeEatCP.size();
						
						if (!m_msgHander->isSameSide(curOptSide, dstValue))
						{
							OptErrorLog("Robot play chess error1, current side = %d, value = %d, src position = %d, dst position = %d",
							curOptSide, dstValue, moveCPInfo.srcPosition, moveCPInfo.dstPosition);
						}
					}
				}
				
				// 第一步躲避防御成功则继续检查在dodgeSteps步之内是否可以成功躲避防止被对方吃掉
				else if (maxBeEatValue > 0 && !naturalProtect && !plCPNotify.has_src_position()
				    && (tmpOurSideBeEatCP.empty() || cpGrades.at(tmpOurSideBeEatCP[0].beEatCP) < maxBeEatValue || getBeEatChessPiecesCount(tmpOurSideBeEatCP) < beEatCount)
				    && canDodge(dodgeSteps, afterMoveValue, curOptSide, opponetCurHp, maxBeEatValue, beEatCount, *posIt, ourSideCPIt->dstPosition, ourSideCPIt->srcPosition))
				{
					plCPNotify.set_src_position(ourSideCPIt->dstPosition);
					plCPNotify.set_dst_position(*posIt);
					
					OptErrorLog("CanDodge Test move our side chess pieces Success, value = %d, after move new srcPosition = %d, dstPosition = %d",
			        dstValue, plCPNotify.src_position(), plCPNotify.dst_position());
					
					if (!m_msgHander->isSameSide(curOptSide, dstValue))
					{
						OptErrorLog("Robot play chess error2, current side = %d, value = %d, src position = %d, dst position = %d",
						curOptSide, dstValue, plCPNotify.src_position(), plCPNotify.dst_position());
					}
				}
				
				afterMoveValue[*posIt] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;  // 还原成空位置
				
				// 可移动的棋子
				if (needCanMoveCP) canMoveCPs->push_back(SRobotMoveChessPieces(ourSideCPIt->dstPosition, *posIt, ourSideCPIt->beEatCP));
			}
		}
	}
	
	if (moveCPInfo.beEatCP != com_protocol::EChessPiecesValue::ENoneQiZiOpen)
	{
		plCPNotify.set_src_position(moveCPInfo.srcPosition);
		plCPNotify.set_dst_position(moveCPInfo.dstPosition);
		
		return true;
	}
	
	return false;
}

// 是否可以吃掉对方的棋子
int CRobotManager::eatChessPieces(const short* chessPiecesValue, const short curOptSide, const int opponentCurHp, const SRobotMoveChessPiecesVector& ourSideBeEatCP, const SRobotMoveChessPiecesVector& opponentSideBeEatCP,
                                  const bool currentIsOurSide, unsigned int& index, const short newMvPos)
{
	index = 0;
	
	// 找出双方可以互相吃的棋子信息
	SRobotMoveChessPiecesVector tmpOurSideBeEatCP;       // 我方可以被对方吃掉的棋子的列表
	SRobotMoveChessPiecesVector tmpOpponentSideBeEatCP;  // 对方可以被我方吃掉的棋子的列表
	SRobotMoveChessPiecesVector tmpOurChessPosition;     // 我方不能吃对方的棋子
	vector<short> tmpClosedChessPosition;                // 未翻开的棋子
	short afterMoveValue[MaxChessPieces] = {0};          // 移动后的棋子布局
	const unsigned int cpValueSize = sizeof(afterMoveValue);
	
	const unordered_map<unsigned int, unsigned int>& cpGrades = XiangQiConfig::config::getConfigValue().chess_pieces_value;
	if (!currentIsOurSide)  // 当前的结果是我方下棋后的结果，下一步轮到对方下棋
	{
		// 如果可以吃子了则进一步检查是否满足以下条件
		if (!opponentSideBeEatCP.empty() && (ourSideBeEatCP.empty() || cpGrades.at(ourSideBeEatCP[0].beEatCP) < cpGrades.at(opponentSideBeEatCP[0].beEatCP)))
		{
		    // 1、得先检查吃掉对方棋子的我方棋子是否会被对方其他棋子优先吃掉
			if (!ourSideBeEatCP.empty())
			{
				for (SRobotMoveChessPiecesVector::const_iterator ourRbtIt = ourSideBeEatCP.begin(); ourRbtIt != ourSideBeEatCP.end(); ++ourRbtIt)
				{
					// 我方正准备吃掉对方的棋子，不料却优先被对方的棋子吃掉了，下一步棋是对方先走动
					// 此种情况有：1、兵准备吃将却被对方其他棋子吃掉；2、相同的棋子则优先被对方吃掉；3、想吃对方的棋子却被对方的炮优先打掉
					if (opponentSideBeEatCP[0].srcPosition == ourRbtIt->dstPosition) return EMoveChessPiecesOpt::EMoveBeEatCP;
				}
				
				// 移动的棋子作为炮台之后，下一步却被对方的目标棋子（作为我方炮的目标棋子）吃掉
	            if (isBatteryBeEat(chessPiecesValue, newMvPos, opponentSideBeEatCP[0], ourSideBeEatCP)) return EMoveChessPiecesOpt::EMoveBeEatCP;
			}
			
			// 2、可以吃则吃掉对方棋子后再检查是否会被对方吃掉
		    if (canEatChessPieces(chessPiecesValue, curOptSide, opponentCurHp, opponentSideBeEatCP[0], afterMoveValue, cpValueSize,
								  tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition)) return 0;  // 占据优势则返回可以吃
		}
		
	    return ourSideBeEatCP.empty() ? EMoveChessPiecesOpt::EMoveOurCP : EMoveChessPiecesOpt::EMoveBeEatCP;
	}

    unsigned int canEatClosedIndex = opponentSideBeEatCP.size();
	if (ourSideBeEatCP.empty())  // 我方没有被吃的棋子
	{
		for (SRobotMoveChessPiecesVector::const_iterator opponentRbtIt = opponentSideBeEatCP.begin(); opponentRbtIt != opponentSideBeEatCP.end(); ++opponentRbtIt)
		{
			// 吃掉对方棋子后再检查是否存在优势
			if (canEatChessPieces(chessPiecesValue, curOptSide, opponentCurHp, *opponentRbtIt, afterMoveValue, cpValueSize,
			                      tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition))
			{
				if (opponentRbtIt->isOpen) return 0;  // 优先吃掉已经翻开了的棋子
				
				if (index < canEatClosedIndex) canEatClosedIndex = index;  // 还未翻开的棋子则记录最优值
			}

			++index;
		}
		index = canEatClosedIndex;
		
		return (index < opponentSideBeEatCP.size()) ? 0 : EMoveChessPiecesOpt::EMoveOurCP;  // 移动我方的棋子
	}
	
	else if (!opponentSideBeEatCP.empty())
	{
		// 血量占优势则吃掉对方棋子后再检查是否会被对方吃掉
		const SRobotMoveChessPieces& opFirstBeEat = opponentSideBeEatCP[0];
		const bool isOpponentCP = opFirstBeEat.isOpen || !m_msgHander->isSameSide(curOptSide, opFirstBeEat.beEatCP);
		if (isOpponentCP && (int)XiangQiConfig::config::getConfigValue().chess_pieces_life.at(opFirstBeEat.beEatCP) >= opponentCurHp) return 0; // 吃掉对方该棋子就胜利了则直接吃掉

        // 如果我方第一个被吃的是炮则特殊处理
		const SRobotMoveChessPieces& ourFirstBeEat = ourSideBeEatCP[0];
		if (ourFirstBeEat.beEatCP == com_protocol::EChessPiecesValue::EHeiQiPao || ourFirstBeEat.beEatCP == com_protocol::EChessPiecesValue::EHongQiPao)
		{
			// 先检查下对方的棋子是否可以被我方棋子吃掉
			bool opponentIsBeEat = false;
			for (SRobotMoveChessPiecesVector::const_iterator opponentRbtIt = opponentSideBeEatCP.begin(); opponentRbtIt != opponentSideBeEatCP.end(); ++opponentRbtIt)
			{
				if (ourFirstBeEat.srcPosition == opponentRbtIt->dstPosition)
				{
					opponentIsBeEat = true;
					break;
				}
			}
			
			if (!opponentIsBeEat)
			{
				index = 0;
				unsigned int paoEatIndex = opponentSideBeEatCP.size();
				for (SRobotMoveChessPiecesVector::const_iterator opponentRbtIt = opponentSideBeEatCP.begin(); opponentRbtIt != opponentSideBeEatCP.end(); ++opponentRbtIt)
				{
					if (ourFirstBeEat.dstPosition == opponentRbtIt->srcPosition)  // 我方被吃的炮，同时也是可以吃掉对方棋子的棋子
					{
						if (canEatChessPieces(chessPiecesValue, curOptSide, opponentCurHp, *opponentRbtIt, afterMoveValue, cpValueSize,
											  tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition)) return 0;
											  
						if (index < paoEatIndex) paoEatIndex = index;
					}
					
					++index;
				}
				
				if (paoEatIndex < opponentSideBeEatCP.size())
				{
					index = paoEatIndex;
					return 0;
				}
			}
		}
		
		index = 0;
		const unsigned int maxBeEatValue = cpGrades.at(ourFirstBeEat.beEatCP);
        if (opFirstBeEat.isOpen && cpGrades.at(opFirstBeEat.beEatCP) >= maxBeEatValue
			&& canEatChessPieces(chessPiecesValue, curOptSide, opponentCurHp, opFirstBeEat, afterMoveValue, cpValueSize,
								 tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition)) return 0;  // 占据优势则返回可以吃									 
								 
		// 检查如果吃掉对方棋子后是否能形成优势的局面（比如此时马可以吃掉对方的兵，否则对方的兵会吃掉我方的将），则可以直接吃掉对方棋子
		for (SRobotMoveChessPiecesVector::const_iterator opponentRbtIt = opponentSideBeEatCP.begin(); opponentRbtIt != opponentSideBeEatCP.end(); ++opponentRbtIt)
		{
			// 吃掉对方棋子后再检查是否存在优势
			// 这里可能还得先看 ourSideBeEatCP[0].beEatCP 能不能躲避成功，如果能成功躲避则躲避之后的血量值大于被吃的血量值
			if (canEatChessPieces(chessPiecesValue, curOptSide, opponentCurHp, *opponentRbtIt, afterMoveValue, cpValueSize, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition)
			    || cpGrades.at(tmpOurSideBeEatCP[0].beEatCP) < maxBeEatValue)
			{
				if (opponentRbtIt->isOpen) return 0;  // 优先吃掉已经翻开了的棋子
				
				if (index < canEatClosedIndex) canEatClosedIndex = index;  // 还未翻开的棋子则记录最优值
			}
								  
			++index;
		}
		
		if (canEatClosedIndex < opponentSideBeEatCP.size())
		{
			index = canEatClosedIndex;
			return 0;
		}
		
		// 检查我方被吃的棋子是否可以吃掉对方棋子之后占据优势
		canEatClosedIndex = opponentSideBeEatCP.size();
		index = MaxChessPieces;
		unsigned int currentFindIdx = 0;
		for (SRobotMoveChessPiecesVector::const_iterator ourRbtIt = ourSideBeEatCP.begin(); ourRbtIt != ourSideBeEatCP.end(); ++ourRbtIt)
		{
			currentFindIdx = 0;
			for (SRobotMoveChessPiecesVector::const_iterator opponentRbtIt = opponentSideBeEatCP.begin(); opponentRbtIt != opponentSideBeEatCP.end(); ++opponentRbtIt)
			{
				if (ourRbtIt->dstPosition == opponentRbtIt->srcPosition)  // 我方被吃的棋子，同时也是可以吃掉对方棋子的棋子
				{
					// 吃掉对方棋子后再检查是否存在优势
					if (canEatChessPieces(chessPiecesValue, curOptSide, opponentCurHp, *opponentRbtIt, afterMoveValue, cpValueSize,
					                      tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition))
					{
						if (opponentRbtIt->isOpen)  // 优先吃掉已经翻开了的棋子
						{
							if (currentFindIdx < index) index = currentFindIdx;
						}
						else if (currentFindIdx < canEatClosedIndex)
						{
							canEatClosedIndex = currentFindIdx;  // 还未翻开的棋子则记录最优值
						}
					}
				}
				
				++currentFindIdx;
			}
		}
		
		if (index < MaxChessPieces) return 0;
		
		index = canEatClosedIndex;
		return (index < opponentSideBeEatCP.size()) ? 0 : EMoveChessPiecesOpt::EMoveBeEatCP;  // 优先移动我方被吃的棋子 ourSideBeEatCP
	}
	
	return EMoveChessPiecesOpt::EMoveBeEatCP;
}

// 是否可以吃掉对方的棋子
bool CRobotManager::eatChessPieces(const short* chessPiecesValue, const short curOptSide, const int opponentCurHp,
                                   const SRobotMoveChessPiecesVector& ourSideBeEatCP, const SRobotMoveChessPiecesVector& opponentSideBeEatCP, const short newMvPos)
{
	// 当前的结果是我方下棋后的结果，下一步轮到对方下棋
	// 如果可以吃子了则进一步检查是否满足以下条件
	const unordered_map<unsigned int, unsigned int>& cpGrades = XiangQiConfig::config::getConfigValue().chess_pieces_value;
	if (!opponentSideBeEatCP.empty() && (ourSideBeEatCP.empty() || cpGrades.at(ourSideBeEatCP[0].beEatCP) < cpGrades.at(opponentSideBeEatCP[0].beEatCP)))
	{
		// 1、得先检查吃掉对方棋子的我方棋子是否会被对方其他棋子优先吃掉
		if (!ourSideBeEatCP.empty())
		{
			for (SRobotMoveChessPiecesVector::const_iterator ourRbtIt = ourSideBeEatCP.begin(); ourRbtIt != ourSideBeEatCP.end(); ++ourRbtIt)
			{
				// 我方正准备吃掉对方的棋子，不料却优先被对方的棋子吃掉了，下一步棋是对方先走动
				// 此种情况有：1、兵准备吃将却被对方其他棋子吃掉；2、相同的棋子则优先被对方吃掉；3、想吃对方的棋子却被对方的炮优先打掉
				if (opponentSideBeEatCP[0].srcPosition == ourRbtIt->dstPosition) return false;
			}
			
			// 移动的棋子作为炮台之后，下一步却被对方的目标棋子（作为我方炮的目标棋子）吃掉
	        if (isBatteryBeEat(chessPiecesValue, newMvPos, opponentSideBeEatCP[0], ourSideBeEatCP)) return false;
		}

		SRobotMoveChessPiecesVector tmpOurSideBeEatCP;       // 我方可以被对方吃掉的棋子的列表
		SRobotMoveChessPiecesVector tmpOpponentSideBeEatCP;  // 对方可以被我方吃掉的棋子的列表
		SRobotMoveChessPiecesVector tmpOurChessPosition;     // 我方不能吃对方的棋子
		vector<short> tmpClosedChessPosition;                // 未翻开的棋子
		short afterMoveValue[MaxChessPieces] = {0};          // 移动后的棋子布局
		
		// 2、可以吃则吃掉对方棋子后再检查是否会被对方吃掉
		return canEatChessPieces(chessPiecesValue, curOptSide, opponentCurHp, opponentSideBeEatCP[0], afterMoveValue, sizeof(afterMoveValue),
							     tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition, true);
	}
	
	return false;
}

// 是否可以吃掉该棋子
bool CRobotManager::canEatChessPieces(const short* chessPiecesValue, const short curOptSide, const int opponentCurHp, const SRobotMoveChessPieces& beEatChessPieces, short* afterEatChessPiecesValue, const unsigned int cpValueSize,
					                  SRobotMoveChessPiecesVector& ourSideBeEatCP, SRobotMoveChessPiecesVector& opponentSideBeEatCP, SRobotMoveChessPiecesVector& ourChessPosition,
									  vector<short>& closedChessPosition, bool filterNaturalProtect)
{
	const bool isOpponentCP = beEatChessPieces.isOpen || !m_msgHander->isSameSide(curOptSide, beEatChessPieces.beEatCP);
	if (isOpponentCP && (int)XiangQiConfig::config::getConfigValue().chess_pieces_life.at(beEatChessPieces.beEatCP) >= opponentCurHp) return true;  // 吃掉对方该棋子就胜利了则直接吃掉
	
	// 吃掉对方棋子后再检查是否会被对方吃掉
	memcpy(afterEatChessPiecesValue, chessPiecesValue, cpValueSize);
	afterEatChessPiecesValue[beEatChessPieces.dstPosition] = afterEatChessPiecesValue[beEatChessPieces.srcPosition];
	afterEatChessPiecesValue[beEatChessPieces.srcPosition] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;  // 变成空位置
	findCanEatChessPieces(afterEatChessPiecesValue, curOptSide, ourSideBeEatCP, opponentSideBeEatCP, ourChessPosition, closedChessPosition);
	
	// 过滤掉我方被吃的棋子列表中具备自然防护的棋子
	const unordered_map<unsigned int, unsigned int>& cpGrades = XiangQiConfig::config::getConfigValue().chess_pieces_value;
	if (filterNaturalProtect && !ourSideBeEatCP.empty())
	{
		SRobotMoveChessPiecesVector npTmpOurSideBeEatCP;       // 我方可以被对方吃掉的棋子的列表
		SRobotMoveChessPiecesVector npTmpOpponentSideBeEatCP;  // 对方可以被我方吃掉的棋子的列表
		SRobotMoveChessPiecesVector npTmpOurChessPosition;     // 我方不能吃对方的棋子
		vector<short> npTmpClosedChessPosition;                // 未翻开的棋子
		
		// 找出被对方棋子吃，但存在自然防御的我方棋子
		const unsigned int ourSideFirstBeEatValue = cpGrades.at(ourSideBeEatCP[0].beEatCP);
		const unsigned int ourSideBeEatCount = ourSideBeEatCP.size();
		for (SRobotMoveChessPiecesVector::iterator ourSideBeEatCPIt = ourSideBeEatCP.begin(); ourSideBeEatCPIt != ourSideBeEatCP.end();)
		{
			afterEatChessPiecesValue[ourSideBeEatCPIt->dstPosition] = afterEatChessPiecesValue[ourSideBeEatCPIt->srcPosition];        // 对方吃掉我方棋子
			afterEatChessPiecesValue[ourSideBeEatCPIt->srcPosition] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;                 // 变成空位置
			findCanEatChessPieces(afterEatChessPiecesValue, curOptSide, npTmpOurSideBeEatCP, npTmpOpponentSideBeEatCP, npTmpOurChessPosition, npTmpClosedChessPosition);
			
			// 检查是否占据优势
			bool naturalProtect = false;
			if (npTmpOurSideBeEatCP.empty() || (cpGrades.at(npTmpOurSideBeEatCP[0].beEatCP) <= ourSideFirstBeEatValue && npTmpOurSideBeEatCP.size() <= ourSideBeEatCount))
			{
				for (SRobotMoveChessPiecesVector::iterator opponentSideBeEatCPIt = npTmpOpponentSideBeEatCP.begin(); opponentSideBeEatCPIt != npTmpOpponentSideBeEatCP.end(); ++opponentSideBeEatCPIt)
				{
					if (cpGrades.at(opponentSideBeEatCPIt->beEatCP) >= cpGrades.at(ourSideBeEatCPIt->beEatCP))
					{
						naturalProtect = true;
						break;
					}
				}
			}
			
			// 还原棋子
			afterEatChessPiecesValue[ourSideBeEatCPIt->srcPosition] = afterEatChessPiecesValue[ourSideBeEatCPIt->dstPosition];
			afterEatChessPiecesValue[ourSideBeEatCPIt->dstPosition] = ourSideBeEatCPIt->beEatCP;
			
			if (naturalProtect)  // 本身具备了自然防护
			{
				ourSideBeEatCPIt = ourSideBeEatCP.erase(ourSideBeEatCPIt);  // 本身具备了防护则先删除
			}
			else
			{
				++ourSideBeEatCPIt;
			}
		}
	}

	if (ourSideBeEatCP.empty() || (beEatChessPieces.isOpen && cpGrades.at(ourSideBeEatCP[0].beEatCP) <= cpGrades.at(beEatChessPieces.beEatCP))) return true;  // 占据优势则返回可以吃
	
	if (opponentSideBeEatCP.empty() || cpGrades.at(ourSideBeEatCP[0].beEatCP) >= cpGrades.at(opponentSideBeEatCP[0].beEatCP)) return false;
		
	for (SRobotMoveChessPiecesVector::const_iterator ourRbtIt = ourSideBeEatCP.begin(); ourRbtIt != ourSideBeEatCP.end(); ++ourRbtIt)
	{
		// 第一种情况：我方的炮打掉对方未翻开的棋子，打掉对方棋子后我方的炮被吃掉，此情况下返回不可吃掉对方棋子
		// 第二种情况：我方正准备吃掉对方的棋子，不料却优先被对方的棋子吃掉了，下一步棋是对方先走动
		// 这种情况有：1、兵准备吃将却被对方其他棋子吃掉；2、相同的棋子则优先被对方吃掉；3、想吃对方的棋子却被对方的炮优先打掉
		if (beEatChessPieces.dstPosition == ourRbtIt->dstPosition && cpGrades.at(ourRbtIt->beEatCP) > cpGrades.at(beEatChessPieces.beEatCP)) return false;
	}
	
	return true;
}

// 找出双方可以互相吃的棋子信息
void CRobotManager::findCanEatChessPieces(const short* chessPiecesValue, const short curOptSide,
                                          SRobotMoveChessPiecesVector& ourSideBeEatCP, SRobotMoveChessPiecesVector& opponentSideBeEatCP,
										  SRobotMoveChessPiecesVector& ourChessPosition, vector<short>& closedChessPosition)
{
	// 象棋棋子值定义
	//	enum EChessPiecesValue
	//	{
	//		EHongQiJiang = 0;                                      // 红棋 将
	//		EHongQiShi = 1;                                        // 红棋 士
	//		EHongQiXiang = 2;                                      // 红棋 象
	//		EHongQiChe = 3;                                        // 红棋 车
	//		EHongQiMa = 4;                                         // 红棋 马
	//		EHongQiPao = 5;                                        // 红棋 炮
	//		EHongQiBing = 6;                                       // 红棋 兵
	//		
	//		EHeiQiJiang = 7;                                       // 黑棋 将
	//		EHeiQiShi = 8;                                         // 黑棋 士
	//		EHeiQiXiang = 9;                                       // 黑棋 象
	//		EHeiQiChe = 10;                                        // 黑棋 车
	//		EHeiQiMa = 11;                                         // 黑棋 马
	//		EHeiQiPao = 12;                                        // 黑棋 炮
	//		EHeiQiBing = 13;                                       // 黑棋 兵
	//		
	//		ENoneQiZiClose = 14;                                   // 存在棋子，但没有翻开
	//		ENoneQiZiOpen = 15;                                    // 不存在棋子，空位置
	//	}

	// ----------------------------
	//       棋盘位置分布图
	//       0--08--16--24
	//       |   |   |   |
	//       1--09--17--25
	//       |   |   |   |
	//       2--10--18--26
	//       |   |   |   |
	//       3--11--19--27
	//       |   |   |   |
	//       4--12--20--28
	//       |   |   |   |
	//       5--13--21--29
	//       |   |   |   |
	//       6--14--22--30
	//       |   |   |   |
	//       7--15--23--31
	// ----------------------------

	short ourMinValue = com_protocol::EChessPiecesValue::EHeiQiJiang;
	short ourMaxValue = com_protocol::EChessPiecesValue::EHeiQiBing;
	short ourPaoValue = com_protocol::EChessPiecesValue::EHeiQiPao;
	
	unsigned short opponentSide = EXQSideFlag::ERedSide;
	short opponentMinValue = com_protocol::EChessPiecesValue::EHongQiJiang;
	short opponentMaxValue = com_protocol::EChessPiecesValue::EHongQiBing;
	short opponentPaoValue = com_protocol::EChessPiecesValue::EHongQiPao;
	if (curOptSide == EXQSideFlag::ERedSide)
	{
		ourMinValue = com_protocol::EChessPiecesValue::EHongQiJiang;
	    ourMaxValue = com_protocol::EChessPiecesValue::EHongQiBing;
		ourPaoValue = com_protocol::EChessPiecesValue::EHongQiPao;
	
		opponentSide = EXQSideFlag::EBlackSide;
		opponentMinValue = com_protocol::EChessPiecesValue::EHeiQiJiang;
	    opponentMaxValue = com_protocol::EChessPiecesValue::EHeiQiBing;
		opponentPaoValue = com_protocol::EChessPiecesValue::EHeiQiPao;
	}

    ourSideBeEatCP.clear();
	opponentSideBeEatCP.clear();
	ourChessPosition.clear();
	closedChessPosition.clear();
	
	short cpValue = 0;
	unsigned int eatCount = 0;
	for (int cpIdx = MinChessPiecesPosition; cpIdx <= MaxChessPieCesPosition; ++cpIdx)
	{
		// 只检查双方都已经翻开了的棋子
		cpValue = chessPiecesValue[cpIdx];
		if (cpValue >= opponentMinValue && cpValue <= opponentMaxValue)  // 对方棋子
		{
			if (cpValue == opponentPaoValue)  // 对方的炮
			{
				paoEatChessPieces(chessPiecesValue, opponentSide, cpIdx, ourSideBeEatCP);
			}
			else
			{
				cpEatChessPieces(ourMinValue, ourMaxValue, chessPiecesValue, cpIdx, cpValue, ourSideBeEatCP);
			}
		}
		
		else if (cpValue >= ourMinValue && cpValue <= ourMaxValue)  // 我方棋子
		{
			if (cpValue == ourPaoValue)  // 我方的炮
			{
				eatCount = paoEatChessPieces(chessPiecesValue, curOptSide, cpIdx, opponentSideBeEatCP, true);
			}
			else
			{
				eatCount = cpEatChessPieces(opponentMinValue, opponentMaxValue, chessPiecesValue, cpIdx, cpValue, opponentSideBeEatCP);
			}
			
			if (eatCount < 1) ourChessPosition.push_back(SRobotMoveChessPieces(-1, cpIdx, cpValue));
		}
		
		else if (cpValue >= ClosedChessPiecesValue)  // 未翻开的棋子
		{
			closedChessPosition.push_back(cpIdx);
		}
	}
	
	// 排序
	std::sort(ourSideBeEatCP.begin(), ourSideBeEatCP.end(), chessPiecesValueSort);
	std::sort(opponentSideBeEatCP.begin(), opponentSideBeEatCP.end(), chessPiecesValueSort);
	
	// 删除我方列表中被吃掉的棋子
	if (!ourSideBeEatCP.empty())
	{
		for (SRobotMoveChessPiecesVector::iterator ourChessPositionIt = ourChessPosition.begin(); ourChessPositionIt != ourChessPosition.end();)
		{
			if (std::find(ourSideBeEatCP.begin(), ourSideBeEatCP.end(), *ourChessPositionIt) != ourSideBeEatCP.end())
			{
				ourChessPositionIt = ourChessPosition.erase(ourChessPositionIt);
			}
			else
			{
				++ourChessPositionIt;
			}
		}
	}
	std::sort(ourChessPosition.begin(), ourChessPosition.end(), chessPiecesValueSort);
}

// 找出棋子可以移动的位置
bool CRobotManager::canMovePosition(const short* chessPiecesValue, const unsigned short curPosition, vector<short>& newPositions)
{
	newPositions.clear();
	
    const short cpValue = chessPiecesValue[curPosition];
	if (cpValue > com_protocol::EChessPiecesValue::EHeiQiBing
	    || cpValue == com_protocol::EChessPiecesValue::EHongQiPao
		|| cpValue == com_protocol::EChessPiecesValue::EHeiQiPao) return false;  // 非翻开的棋子，炮等不可移动

	// 1）向左边移动
	short moveIndex = curPosition - MaxDifferenceValue;
	if (moveIndex >= MinChessPiecesPosition && chessPiecesValue[moveIndex] == com_protocol::EChessPiecesValue::ENoneQiZiOpen) newPositions.push_back(moveIndex);
	
	// 2）向右边移动
	moveIndex = curPosition + MaxDifferenceValue;
	if (moveIndex <= MaxChessPieCesPosition && chessPiecesValue[moveIndex] == com_protocol::EChessPiecesValue::ENoneQiZiOpen) newPositions.push_back(moveIndex);
	
	// 3）向上移动
	// 最上一行棋子  0--08--16--24
	moveIndex = curPosition - 1;
	if ((curPosition % MaxDifferenceValue) != 0 && chessPiecesValue[moveIndex] == com_protocol::EChessPiecesValue::ENoneQiZiOpen) newPositions.push_back(moveIndex);
	
	// 4）向下移动
	// 最下一行棋子  7--15--23--31
	moveIndex = curPosition + 1;
	if ((moveIndex % MaxDifferenceValue) != 0 && chessPiecesValue[moveIndex] == com_protocol::EChessPiecesValue::ENoneQiZiOpen) newPositions.push_back(moveIndex);
	
	return !newPositions.empty();
}

// 炮可以吃掉的棋子
unsigned int CRobotManager::paoEatChessPieces(const short* chessPiecesValue, const unsigned short opponentSide, const int opponentIdx, SRobotMoveChessPiecesVector& beEatCP, const bool eatClosed)
{
	#define CHECK_PAO_EAT_CHESS_PIECES(moveIndex)                                                                              \
		ourValue = chessPiecesValue[moveIndex];                                                                                \
	    if ((ourValue < com_protocol::EChessPiecesValue::ENoneQiZiClose || (eatClosed && ourValue >= ClosedChessPiecesValue))  \
		    && m_msgHander->checkMovePaoIsOK(opponentIdx, moveIndex, opponentSide, chessPiecesValue))                          \
		{                                                                                                                      \
		    cpIsOpen = true;                                                                                                   \
		    if (ourValue >= ClosedChessPiecesValue)                                                                            \
			{                                                                                                                  \
				ourValue -= ClosedChessPiecesValue;                                                                            \
				if (m_msgHander->isSameSide(opponentSide, ourValue) && !paoCanEatOurSideChessPieces(ourValue)) break;          \
				cpIsOpen = false;                                                                                              \
			}                                                                                                                  \
			                                                                                                                   \
			beEatCP.push_back(SRobotMoveChessPieces(opponentIdx, moveIndex, ourValue, cpIsOpen));                              \
			++eatCount;                                                                                                        \
			break;                                                                                                             \
		}
		
	
	// 1）左边扫描看看有没有可以吃的棋子
	unsigned int eatCount = 0;
	short ourValue = 0;
	bool cpIsOpen = true;
	int moveIndex = opponentIdx - (MaxDifferenceValue * 2);
	while (moveIndex >= MinChessPiecesPosition)
	{
		CHECK_PAO_EAT_CHESS_PIECES(moveIndex);
		
		moveIndex -= MaxDifferenceValue;
	}
	
	// 2）右边扫描看看有没有可以吃的棋子
	moveIndex = opponentIdx + (MaxDifferenceValue * 2);
	while (moveIndex <= MaxChessPieCesPosition)
	{
		CHECK_PAO_EAT_CHESS_PIECES(moveIndex);
		
		moveIndex += MaxDifferenceValue;
	}
	
	// 3）向上扫描看看有没有可以吃的棋子
	moveIndex = opponentIdx - 2;
	while (moveIndex >= MinChessPiecesPosition && (opponentIdx - moveIndex) < MaxDifferenceValue)
	{
		CHECK_PAO_EAT_CHESS_PIECES(moveIndex);
		
		--moveIndex;
	}
	
	// 4）向下扫描看看有没有可以吃的棋子
	moveIndex = opponentIdx + 2;
	while (moveIndex <= MaxChessPieCesPosition && (moveIndex - opponentIdx) < MaxDifferenceValue)
	{
		CHECK_PAO_EAT_CHESS_PIECES(moveIndex);
		
		++moveIndex;
	}
	
	return eatCount;
}

// 棋子可以吃掉的棋子
unsigned int CRobotManager::cpEatChessPieces(const short ourMinValue, const short ourMaxValue, const short* chessPiecesValue, const int opponentIdx, const int opponentValue, SRobotMoveChessPiecesVector& beEatCP)
{
	#define CHECK_EAT_CHESS_PIECES(moveIndex)                                                                                    \
	    ourValue = chessPiecesValue[moveIndex];                                                                                  \
	    if (ourValue <= ourMaxValue && ourValue >= ourMinValue && m_msgHander->canEatOtherChessPieces(opponentValue, ourValue))  \
		{                                                                                                                        \
			beEatCP.push_back(SRobotMoveChessPieces(opponentIdx, moveIndex, ourValue));                                          \
			++eatCount;                                                                                                          \
		}
		

	// 1）左边扫描看看有没有可以吃的棋子
	unsigned int eatCount = 0;
	short ourValue = 0;
	int moveIndex = opponentIdx - MaxDifferenceValue;
	if (moveIndex >= MinChessPiecesPosition)
	{
		CHECK_EAT_CHESS_PIECES(moveIndex);  // 我方棋子是否可以被敌方吃掉
	}
	
	// 2）右边扫描看看有没有可以吃的棋子
	moveIndex = opponentIdx + MaxDifferenceValue;
	if (moveIndex <= MaxChessPieCesPosition)
	{
		CHECK_EAT_CHESS_PIECES(moveIndex);  // 我方棋子是否可以被敌方吃掉
	}
	
	// 3）向上扫描看看有没有可以吃的棋子
	// 最上一行棋子  0--08--16--24
	moveIndex = opponentIdx - 1;
	if ((opponentIdx % MaxDifferenceValue) != 0)
	{
		CHECK_EAT_CHESS_PIECES(moveIndex);  // 我方棋子是否可以被敌方吃掉
	}
	
	// 4）向下扫描看看有没有可以吃的棋子
	// 最下一行棋子  7--15--23--31
	moveIndex = opponentIdx + 1;
	if ((moveIndex % MaxDifferenceValue) != 0)
	{
		CHECK_EAT_CHESS_PIECES(moveIndex);  // 我方棋子是否可以被敌方吃掉
	}
	
	return eatCount;
}

// 在steps步之内是否可以成功躲避防止被对方吃掉
bool CRobotManager::canDodge(const int steps, const short* chessPiecesValue, const short curOptSide, const int opponentCurHp, const unsigned int maxBeEatValue, const unsigned int beEatCount, 
                             const short ourCurrentPosition, const short ourLastPosition, const short opponentSrcPosition)
{
	if (opponentSrcPosition < 0 || steps < 1)
	{
		OptErrorLog("CanDodge Test return true 0, steps = %d, maxBeEatValue = %d, ourCurrentPosition = %d, ourLastPosition = %d, opponentSrcPosition = %d, opponentValue = %d",
		steps, maxBeEatValue, ourCurrentPosition, ourLastPosition, opponentSrcPosition, (opponentSrcPosition < 0) ? -1 : chessPiecesValue[opponentSrcPosition]);
		
		return true;  // 表示没有敌方的棋子追击 或者 步数已经递归运算完毕了
	}
	
	// 对方是炮则走法不同，特殊处理
	if (chessPiecesValue[opponentSrcPosition] == com_protocol::EChessPiecesValue::EHongQiPao || chessPiecesValue[opponentSrcPosition] == com_protocol::EChessPiecesValue::EHeiQiPao)
	{
		// 先看下炮是否可以吃掉我方棋子
		if (m_msgHander->checkMovePaoIsOK(opponentSrcPosition, ourCurrentPosition, (curOptSide + 1) % EXQSideFlag::ESideFlagCount, chessPiecesValue))
		{
			OptErrorLog("CanDodge Test return false 0, steps = %d, maxBeEatValue = %d, ourCurrentPosition = %d, ourLastPosition = %d, opponentSrcPosition = %d, beEatCount = %d",
					    steps, maxBeEatValue, ourCurrentPosition, ourLastPosition, opponentSrcPosition, beEatCount);
	        return false;
		}
		
		OptErrorLog("CanDodge Test return true 1, steps = %d, maxBeEatValue = %d, ourCurrentPosition = %d, ourLastPosition = %d, opponentSrcPosition = %d, opponentValue = %d",
		steps, maxBeEatValue, ourCurrentPosition, ourLastPosition, opponentSrcPosition, chessPiecesValue[opponentSrcPosition]);
		
		return true;  // 敌方的棋子是炮则不能沿着我方棋子的轨迹追击
	}

	short afterMoveValue[MaxChessPieces] = {0};          // 移动后的棋子布局
	memcpy(afterMoveValue, chessPiecesValue, sizeof(afterMoveValue));
	afterMoveValue[ourLastPosition] = afterMoveValue[opponentSrcPosition];                 // 对方的棋子沿着我方棋子移动的轨迹追击我方棋子
	afterMoveValue[opponentSrcPosition] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;  // 变成空位置
	
	// 找出双方可以互相吃的棋子信息
	unsigned int eatOpponentIndex = 0;
	SRobotMoveChessPiecesVector tmpOurSideBeEatCP;       // 我方可以被对方吃掉的棋子的列表
	SRobotMoveChessPiecesVector tmpOpponentSideBeEatCP;  // 对方可以被我方吃掉的棋子的列表
	SRobotMoveChessPiecesVector tmpOurChessPosition;     // 我方不能吃对方的棋子
	vector<short> tmpClosedChessPosition;                // 未翻开的棋子
	findCanEatChessPieces(afterMoveValue, curOptSide, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition);
	if (eatChessPieces(afterMoveValue, curOptSide, opponentCurHp, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, true, eatOpponentIndex, -1) == 0)
	{
		OptErrorLog("CanDodge Test return true 2, steps = %d, maxBeEatValue = %d, ourCurrentPosition = %d, ourLastPosition = %d, opponentSrcPosition = %d, beEatCount = %d",
		steps, maxBeEatValue, ourCurrentPosition, ourLastPosition, opponentSrcPosition, beEatCount);
		
		return true;  // 是否可以吃掉对方的棋子
	}
	
	vector<short> newPositions;
	if (canMovePosition(afterMoveValue, ourCurrentPosition, newPositions))  // 找出棋子可以移动的位置
	{
		const unordered_map<unsigned int, unsigned int>& cpGrades = XiangQiConfig::config::getConfigValue().chess_pieces_value;
		const short dstValue = afterMoveValue[ourCurrentPosition];  // 移动到新位置的棋子值
		afterMoveValue[ourCurrentPosition] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;  // 变成空位置
		for (vector<short>::const_iterator posIt = newPositions.begin(); posIt != newPositions.end(); ++posIt)
		{
			// 我方棋子移动之后重新运算
			afterMoveValue[*posIt] = dstValue;
			findCanEatChessPieces(afterMoveValue, curOptSide, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition);
			if (eatChessPieces(afterMoveValue, curOptSide, opponentCurHp, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, false, eatOpponentIndex, *posIt) == 0)
			{
				OptErrorLog("CanDodge Test return true 3, steps = %d, maxBeEatValue = %d, ourCurrentPosition = %d, ourLastPosition = %d, opponentSrcPosition = %d, beEatCount = %d",
		        steps, maxBeEatValue, ourCurrentPosition, ourLastPosition, opponentSrcPosition, beEatCount);
		
				return true;  // 是否可以吃掉对方的棋子
			}
			
			// 躲避防御成功则继续递归检查
			if (maxBeEatValue > 0 && (tmpOurSideBeEatCP.empty() || cpGrades.at(tmpOurSideBeEatCP[0].beEatCP) < maxBeEatValue || getBeEatChessPiecesCount(tmpOurSideBeEatCP) < beEatCount)
				&& canDodge(steps - 1, afterMoveValue, curOptSide, opponentCurHp, maxBeEatValue, beEatCount, *posIt, ourCurrentPosition, ourLastPosition))
			{
				OptErrorLog("CanDodge Test return true 4, steps = %d, maxBeEatValue = %d, ourCurrentPosition = %d, ourLastPosition = %d, opponentSrcPosition = %d, beEatCount = %d",
		        steps, maxBeEatValue, ourCurrentPosition, ourLastPosition, opponentSrcPosition, beEatCount);
		
				return true;
			}

			afterMoveValue[*posIt] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;  // 还原成空位置
		}
	}
	
	OptErrorLog("CanDodge Test return false 1, steps = %d, maxBeEatValue = %d, ourCurrentPosition = %d, ourLastPosition = %d, opponentSrcPosition = %d, beEatCount = %d",
	steps, maxBeEatValue, ourCurrentPosition, ourLastPosition, opponentSrcPosition, beEatCount);
		
	return false;
}

// 移动的棋子作为炮台之后，下一步却被对方的目标棋子（作为我方炮的目标棋子）吃掉
bool CRobotManager::isBatteryBeEat(const short* afterMoveValue, const short newMovePosistion, const SRobotMoveChessPieces& opBeEatCP, const SRobotMoveChessPiecesVector& ourSideBeEatCP)
{
	if (ourSideBeEatCP.empty() || !opBeEatCP.isOpen || newMovePosistion < 0) return false;  // 我方没有被吃的棋子  ||  对方被吃的棋子是未翻开的

	const short eatCPValue = afterMoveValue[opBeEatCP.srcPosition];
	if (eatCPValue != com_protocol::EChessPiecesValue::EHeiQiPao && eatCPValue != com_protocol::EChessPiecesValue::EHongQiPao) return false;  // 该棋子不是炮
	
	for (SRobotMoveChessPiecesVector::const_iterator ourBeEatIt = ourSideBeEatCP.begin(); ourBeEatIt != ourSideBeEatCP.end(); ++ourBeEatIt)
	{
		if (newMovePosistion == ourBeEatIt->dstPosition && opBeEatCP.dstPosition == ourBeEatIt->srcPosition) return true;
	}
	
	return false;
}

// 获取被吃棋子的数量，去掉重复被吃的棋子
unsigned int CRobotManager::getBeEatChessPiecesCount(const SRobotMoveChessPiecesVector& beEatCPs)
{
	unsigned int count = 0;
	unordered_map<short, short> beEatCPMap;
	for (SRobotMoveChessPiecesVector::const_iterator it = beEatCPs.begin(); it != beEatCPs.end(); ++it)
	{
		if (beEatCPMap.find(it->dstPosition) == beEatCPMap.end())
		{
			++count;
			beEatCPMap[it->dstPosition] = 0;
		}
	}
	
	return count;
}

// 在自然防御条件下，找出可以移动的最优棋子
EFindMoveChessResult CRobotManager::findCanMoveMostAdvantageCP(const XQChessPieces* xqCP, const SRobotMoveChessPiecesVector& canMoveCPs, SRobotMoveChessPieces& moveCPInfo)
{
	if (canMoveCPs.empty()) return EFindMoveChessResult::ECanNotMoveCP;

	SRobotMoveChessPieces optimalMoveCPInfo(-1, -1, com_protocol::EChessPiecesValue::EHongQiJiang);  // 无对方棋子可吃时，最优可移动的棋子
	unsigned int optimalOpponentBeEatCount = 0;
	unsigned int optimalOurBeEatCount = MaxChessPieces;
	
	moveCPInfo.beEatCP = com_protocol::EChessPiecesValue::ENoneQiZiOpen;
	unsigned int opponentSideBeEatCount = 0;
	unsigned int ourSideBeEatCount = MaxChessPieces;
	
	// 找出双方可以互相吃的棋子信息
	SRobotMoveChessPiecesVector tmpOurSideBeEatCP;       // 我方可以被对方吃掉的棋子的列表
	SRobotMoveChessPiecesVector tmpOpponentSideBeEatCP;  // 对方可以被我方吃掉的棋子的列表
	SRobotMoveChessPiecesVector tmpOurChessPosition;     // 我方不能吃对方的棋子
	vector<short> tmpClosedChessPosition;                // 未翻开的棋子
	short afterMoveValue[MaxChessPieces] = {0};          // 移动后的棋子布局
	
	const unordered_map<unsigned int, unsigned int>& cpGrades = XiangQiConfig::config::getConfigValue().chess_pieces_value;
	const unsigned short curOptSide = xqCP->curOptSide;
	const int opponetCurHp = xqCP->curLife[(curOptSide + 1) % EXQSideFlag::ESideFlagCount];
	unsigned int eatOpponentIndex = 0;
	
	SRobotMoveChessPiecesVector npTmpOurSideBeEatCP;       // 我方可以被对方吃掉的棋子的列表
	SRobotMoveChessPiecesVector npTmpOpponentSideBeEatCP;  // 对方可以被我方吃掉的棋子的列表
	SRobotMoveChessPieces naturalProtectCP;
	memcpy(afterMoveValue, xqCP->value, sizeof(afterMoveValue));
	for (SRobotMoveChessPiecesVector::const_iterator canMoveCPIt = canMoveCPs.begin(); canMoveCPIt != canMoveCPs.end(); ++canMoveCPIt)
	{
		afterMoveValue[canMoveCPIt->dstPosition] = afterMoveValue[canMoveCPIt->srcPosition];        // 移动棋子
		afterMoveValue[canMoveCPIt->srcPosition] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;  // 变成空位置

		// 我方棋子移动之后重新运算
		findCanEatChessPieces(afterMoveValue, curOptSide, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition);
		
		// 先找出被对方棋子吃，但存在自然防御的我方棋子
		for (SRobotMoveChessPiecesVector::iterator ourSideBeEatCPIt = tmpOurSideBeEatCP.begin(); ourSideBeEatCPIt != tmpOurSideBeEatCP.end();)
		{
			naturalProtectCP = *ourSideBeEatCPIt;
			afterMoveValue[ourSideBeEatCPIt->dstPosition] = afterMoveValue[ourSideBeEatCPIt->srcPosition];        // 对方吃掉我方棋子
			afterMoveValue[ourSideBeEatCPIt->srcPosition] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;       // 变成空位置
			findCanEatChessPieces(afterMoveValue, curOptSide, npTmpOurSideBeEatCP, npTmpOpponentSideBeEatCP, tmpOurChessPosition, tmpClosedChessPosition);
			
			if (eatChessPieces(afterMoveValue, curOptSide, opponetCurHp, npTmpOurSideBeEatCP, npTmpOpponentSideBeEatCP, true, eatOpponentIndex, -1) == 0  // 是否可以吃掉对方的棋子
				&& cpGrades.at(npTmpOpponentSideBeEatCP[eatOpponentIndex].beEatCP) >= cpGrades.at(ourSideBeEatCPIt->beEatCP))  // 本身具备了自然防护
			{
				ourSideBeEatCPIt = tmpOurSideBeEatCP.erase(ourSideBeEatCPIt);  // 本身具备了防护则先删除
			}
			else
			{
				++ourSideBeEatCPIt;
			}
			
			// 还原棋子
			afterMoveValue[naturalProtectCP.srcPosition] = afterMoveValue[naturalProtectCP.dstPosition];
			afterMoveValue[naturalProtectCP.dstPosition] = naturalProtectCP.beEatCP;
		}
		
		// 是否可以吃掉对方的棋子
		if (eatChessPieces(afterMoveValue, curOptSide, opponetCurHp, tmpOurSideBeEatCP, tmpOpponentSideBeEatCP, canMoveCPIt->dstPosition))
		{
			const unsigned int opponentCPValue = cpGrades.at(tmpOpponentSideBeEatCP[0].beEatCP);
			const unsigned int lastCPValue = cpGrades.at(moveCPInfo.beEatCP);
			
			if (opponentCPValue > lastCPValue  // 1）找到血量伤害更大的
				|| (opponentCPValue == lastCPValue
					&& (tmpOpponentSideBeEatCP.size() > opponentSideBeEatCount  // 2）血量相同时找到敌方棋子被伤害个数最多的
						|| (tmpOpponentSideBeEatCP.size() == opponentSideBeEatCount && tmpOurSideBeEatCP.size() < ourSideBeEatCount)))  // 3）找到我方棋子被伤害个数最少的
				)
			{
				moveCPInfo.srcPosition = canMoveCPIt->srcPosition;
				moveCPInfo.dstPosition = canMoveCPIt->dstPosition;
				moveCPInfo.beEatCP = tmpOpponentSideBeEatCP[0].beEatCP;
				opponentSideBeEatCount = tmpOpponentSideBeEatCP.size();
				ourSideBeEatCount = tmpOurSideBeEatCP.size();
			}
		}
		
		// 移动棋子之后存在最大优势则记录，不能吃对方的棋子则选择最好的防御
		else if (moveCPInfo.beEatCP == com_protocol::EChessPiecesValue::ENoneQiZiOpen)
		{
			const unsigned int ourCPValue = tmpOurSideBeEatCP.empty() ? 0 : cpGrades.at(tmpOurSideBeEatCP[0].beEatCP);
			const unsigned int lastCPValue = cpGrades.at(optimalMoveCPInfo.beEatCP);

			if (ourCPValue < lastCPValue  // 1）找到血量伤害更小的
				|| (ourCPValue == lastCPValue
					&& (tmpOurSideBeEatCP.size() < optimalOurBeEatCount  // 2）血量相同时找到我方棋子被伤害个数最少的
						|| (tmpOurSideBeEatCP.size() == optimalOurBeEatCount && tmpOpponentSideBeEatCP.size() > optimalOpponentBeEatCount)))  // 3）找到敌方棋子被伤害个数最多的
				)
			{
				optimalMoveCPInfo.srcPosition = canMoveCPIt->srcPosition;
				optimalMoveCPInfo.dstPosition = canMoveCPIt->dstPosition;
				optimalMoveCPInfo.beEatCP = tmpOurSideBeEatCP.empty() ? com_protocol::EChessPiecesValue::ENoneQiZiOpen : tmpOurSideBeEatCP[0].beEatCP;
				optimalOpponentBeEatCount = tmpOpponentSideBeEatCP.size();
				optimalOurBeEatCount = tmpOurSideBeEatCP.size();
			}
		}
		
		// 还原位置
		afterMoveValue[canMoveCPIt->srcPosition] = afterMoveValue[canMoveCPIt->dstPosition];
		afterMoveValue[canMoveCPIt->dstPosition] = com_protocol::EChessPiecesValue::ENoneQiZiOpen;
	}
	
	if (moveCPInfo.beEatCP != com_protocol::EChessPiecesValue::ENoneQiZiOpen) return EFindMoveChessResult::ECanEatCP;
	
	if (optimalMoveCPInfo.srcPosition >= MinChessPiecesPosition && optimalMoveCPInfo.dstPosition >= MinChessPiecesPosition)
	{
		moveCPInfo = optimalMoveCPInfo;
		
		return EFindMoveChessResult::ECanMoveCP;
	}
	
	return EFindMoveChessResult::ECanNotMoveCP;
}

void CRobotManager::generateRobotInfo()
{
	unsigned int idx = 0;
	const vector<string>& robotNickName = XiangQiConfig::config::getConfigValue().robot_cfg.nick_name;
	for (vector<string>::const_iterator it = robotNickName.begin(); it != robotNickName.end(); ++it)
	{
		m_freeRobots.push_back(SRobotInfo());
		SRobotInfo& robotInfo = m_freeRobots.back();
		generateRobotId(++idx, robotInfo.id, MaxConnectIdLen - 1);  // 生成机器人ID
		strncpy(robotInfo.name, it->c_str(), MaxConnectIdLen - 1);
	}
}

const char* CRobotManager::generateRobotId(const unsigned int index, char* robotId, const unsigned int len)
{
	const unsigned int MaxRobotIdLen = 10;
	if (robotId == NULL || len < MaxRobotIdLen) return NULL;
	
	snprintf(robotId, len - 1, "R%05d", index);  // 生成机器人ID，机器人ID前缀为 R
	
	return robotId;
}

// 获取机器人赢率百分比
unsigned int CRobotManager::getWinRatePercent(const XQUserData* ud)
{
	const SRobotWinLostRecord& rbtWLRecord = m_robotWinLostRecord[ud->playMode][ud->arenaId];
	return (rbtWLRecord.allGold > 0) ? (unsigned int)(((double)rbtWLRecord.winGold / (double)rbtWLRecord.allGold) * PercentValue) : PercentValue;
}

// 我方的炮是否可以吃掉我方的棋子
bool CRobotManager::paoCanEatOurSideChessPieces(const short ourValue)
{
	const XiangQiConfig::RobotConfig& robotCfg = XiangQiConfig::config::getConfigValue().robot_cfg;
	return (getPositiveRandNumber(1, PercentValue) <= robotCfg.pao_eat_our_rate
		    && XiangQiConfig::config::getConfigValue().chess_pieces_value.at(ourValue) <= robotCfg.pao_eat_our_max_value);
	
	/*
	if (getWinRatePercent() >= robotCfg.win_rate_percent)  // 正常情况
	{
		if (getPositiveRandNumber(1, PercentValue) <= robotCfg.pao_eat_our_rate
		    && XiangQiConfig::config::getConfigValue().chess_pieces_value.at(ourValue) <= robotCfg.pao_eat_our_max_value) return true;
	}
	else  // 作弊情况
	{
		if (getPositiveRandNumber(1, PercentValue) <= robotCfg.cheat_pao_eat_our_rate
		    && XiangQiConfig::config::getConfigValue().chess_pieces_value.at(ourValue) <= robotCfg.cheat_pao_eat_our_max_value) return true;
	}

	return false;
	*/ 
}

// 机器人下棋前作弊调整未翻开的棋子
void CRobotManager::robotPlayCheatAdjustCP(XQChessPieces* xqCP)
{
	short* chessPiecesValue = xqCP->value;
	const unsigned short curOptSide = xqCP->curOptSide;
	short ourPaoValue = com_protocol::EChessPiecesValue::EHeiQiPao;
	short opponentMinValue = com_protocol::EChessPiecesValue::EHongQiJiang;
	short opponentMaxValue = com_protocol::EChessPiecesValue::EHongQiBing;
	if (curOptSide == EXQSideFlag::ERedSide)
	{
		ourPaoValue = com_protocol::EChessPiecesValue::EHongQiPao;
		opponentMinValue = com_protocol::EChessPiecesValue::EHeiQiJiang;
	    opponentMaxValue = com_protocol::EChessPiecesValue::EHeiQiBing;
	}

    SRobotMoveChessPiecesVector opponentBePaoEatCPs;          // 对方被我方炮吃的棋子列表
    SRobotMoveChessPiecesVector opponentClosedCPs;            // 未翻开的对方棋子
	
	// 优先查找炮，如果存在我方的明炮，则调整敌方最大值的棋子对准我方的炮口，接着我方的炮会直接打掉调整好的对方棋子
	short cpValue = 0;
	short openValue = 0;
	for (int cpIdx = MinChessPiecesPosition; cpIdx <= MaxChessPieCesPosition; ++cpIdx)
	{
		cpValue = chessPiecesValue[cpIdx];
		if (cpValue >= ClosedChessPiecesValue)  // 未翻开的棋子
		{
			openValue = cpValue - ClosedChessPiecesValue;
			if (openValue >= opponentMinValue && openValue <= opponentMaxValue)  // 对方棋子
			{
				opponentClosedCPs.push_back(SRobotMoveChessPieces(cpIdx, cpIdx, openValue));
			}
		}
		else if (cpValue == ourPaoValue)  // 我方的明炮
		{
			paoEatChessPieces(chessPiecesValue, curOptSide, cpIdx, opponentBePaoEatCPs, true);
		}
	}
	
	if (!opponentBePaoEatCPs.empty() && !opponentClosedCPs.empty())
	{
		// 排序后随机挑选其中之一
		std::sort(opponentClosedCPs.begin(), opponentClosedCPs.end(), chessPiecesValueSort);  // 先排序
		
		unsigned int randIndex = XiangQiConfig::config::getConfigValue().robot_cfg.cheat_cfg.our_pao_cheat_eat_count;
		if (randIndex > opponentClosedCPs.size()) randIndex = opponentClosedCPs.size();
		const SRobotMoveChessPieces& opponentClosedBeEatCP = opponentClosedCPs[getPositiveRandNumber(1, randIndex) - 1];
		
		// 找出双方可以互相吃的棋子信息
		const int opponentCurHp = xqCP->curLife[(curOptSide + 1) % EXQSideFlag::ESideFlagCount];
		unsigned int eatOpponentIndex = 0;
		SRobotMoveChessPiecesVector ourSideBeEatCP;       // 我方可以被对方吃掉的棋子的列表
		SRobotMoveChessPiecesVector opponentSideBeEatCP;  // 对方可以被我方吃掉的棋子的列表
		SRobotMoveChessPiecesVector ourChessPosition;     // 我方不能吃对方的棋子
		ChessPiecesPositions closedChessPosition;         // 未翻开的棋子

		const unordered_map<unsigned int, unsigned int> chessPiecesValueCfgMap = XiangQiConfig::config::getConfigValue().chess_pieces_value;
		for (SRobotMoveChessPiecesVector::const_iterator it = opponentBePaoEatCPs.begin(); it != opponentBePaoEatCPs.end(); ++it)
		{
			if (!it->isOpen && (m_msgHander->isSameSide(curOptSide, it->beEatCP)  // 未翻开的棋子
			    || chessPiecesValueCfgMap.at(it->beEatCP) < chessPiecesValueCfgMap.at(opponentClosedBeEatCP.beEatCP)))
			{
				// 执行替换再判断是否占据优势
				chessPiecesValue[it->dstPosition] = opponentClosedBeEatCP.beEatCP + ClosedChessPiecesValue;
				chessPiecesValue[opponentClosedBeEatCP.dstPosition] = it->beEatCP + ClosedChessPiecesValue;
				
				findCanEatChessPieces(chessPiecesValue, curOptSide, ourSideBeEatCP, opponentSideBeEatCP, ourChessPosition, closedChessPosition);
				if (eatChessPieces(chessPiecesValue, curOptSide, opponentCurHp, ourSideBeEatCP, opponentSideBeEatCP, true, eatOpponentIndex, -1) == 0)  // 是否可以吃掉对方的棋子
				{
					OptErrorLog("Only for test robot play cheat adjust CP, src cp index = %d, value = %d, be replace cp index = %d, value = %d",
		            it->dstPosition, it->beEatCP, opponentClosedBeEatCP.dstPosition, opponentClosedBeEatCP.beEatCP);
					break;
				}
				
				// 还原棋子位置
				chessPiecesValue[it->dstPosition] = it->beEatCP + ClosedChessPiecesValue;
				chessPiecesValue[opponentClosedBeEatCP.dstPosition] = opponentClosedBeEatCP.beEatCP + ClosedChessPiecesValue;
			}
		}
	}
}

// 对手炮击下棋前作弊调整未翻开的棋子
void CRobotManager::opponentPaoHitCheatAdjust(short* chessPiecesValue, const short curOptSide, short& dstChessPiecesValue)
{
	if (dstChessPiecesValue < ClosedChessPiecesValue) return;
	
	short ourMinValue = com_protocol::EChessPiecesValue::EHeiQiJiang;
	short ourMaxValue = com_protocol::EChessPiecesValue::EHeiQiBing;
	short opponentMinValue = com_protocol::EChessPiecesValue::EHongQiJiang;
	short opponentMaxValue = com_protocol::EChessPiecesValue::EHongQiBing;
	if (curOptSide == EXQSideFlag::ERedSide)
	{
		ourMinValue = com_protocol::EChessPiecesValue::EHongQiJiang;
	    ourMaxValue = com_protocol::EChessPiecesValue::EHongQiBing;
		opponentMinValue = com_protocol::EChessPiecesValue::EHeiQiJiang;
	    opponentMaxValue = com_protocol::EChessPiecesValue::EHeiQiBing;
	}
	
	short cpValue = 0;
	short openValue = 0;
	SRobotMoveChessPiecesVector ourClosedCPs;            // 未翻开的我方棋子
	SRobotMoveChessPiecesVector opponentClosedCPs;       // 未翻开的敌方棋子
	for (int cpIdx = MinChessPiecesPosition; cpIdx <= MaxChessPieCesPosition; ++cpIdx)
	{
		cpValue = chessPiecesValue[cpIdx];
		if (cpValue >= ClosedChessPiecesValue)  // 未翻开的棋子
		{
			openValue = cpValue - ClosedChessPiecesValue;
			if (openValue >= ourMinValue && openValue <= ourMaxValue)  // 我方棋子
			{
				ourClosedCPs.push_back(SRobotMoveChessPieces(cpIdx, cpIdx, openValue));
			}
			else if (openValue >= opponentMinValue && openValue <= opponentMaxValue)  // 对方棋子
			{
				opponentClosedCPs.push_back(SRobotMoveChessPieces(cpIdx, cpIdx, openValue));
			}
		}
	}
	
	const short needReplaceCPValue = dstChessPiecesValue - ClosedChessPiecesValue;
	const unordered_map<unsigned int, unsigned int> chessPiecesValueCfgMap = XiangQiConfig::config::getConfigValue().chess_pieces_value;
	if (!ourClosedCPs.empty())  // 先考虑吃掉自己一方的棋子
	{
		// 排序后随机挑选其中之一
		std::sort(ourClosedCPs.begin(), ourClosedCPs.end(), chessPiecesValueSort);  // 先排序
		
		unsigned int randIndex = XiangQiConfig::config::getConfigValue().robot_cfg.cheat_cfg.opponent_pao_cheat_eat_count;
		if (randIndex > ourClosedCPs.size()) randIndex = ourClosedCPs.size();
		const SRobotMoveChessPieces& ourClosedBeEatCP = ourClosedCPs[getPositiveRandNumber(1, randIndex) - 1];
		if (!m_msgHander->isSameSide(curOptSide, needReplaceCPValue)  // 不是我方的棋子
		    || chessPiecesValueCfgMap.at(needReplaceCPValue) < chessPiecesValueCfgMap.at(ourClosedBeEatCP.beEatCP))
		{
			// 执行替换
			chessPiecesValue[ourClosedBeEatCP.dstPosition] = dstChessPiecesValue;
			dstChessPiecesValue = ourClosedBeEatCP.beEatCP + ClosedChessPiecesValue;
			
			OptErrorLog("Only for test opponent pao hit cheat adjust1, be replace cp index = %d, value = %d, new dst value = %d",
		                ourClosedBeEatCP.dstPosition, chessPiecesValue[ourClosedBeEatCP.dstPosition], dstChessPiecesValue);
			
			return;
		}
	}
	
	if (!m_msgHander->isSameSide(curOptSide, needReplaceCPValue) && !opponentClosedCPs.empty())  // 再考虑吃掉对方最小的棋子
	{
		std::sort(opponentClosedCPs.begin(), opponentClosedCPs.end(), chessPiecesValueSort);                   // 先排序
		const SRobotMoveChessPieces& opponentClosedBeEatCP = opponentClosedCPs[opponentClosedCPs.size() - 1];  // 取最小值
		if (chessPiecesValueCfgMap.at(needReplaceCPValue) > chessPiecesValueCfgMap.at(opponentClosedBeEatCP.beEatCP))
		{
			// 执行替换
			chessPiecesValue[opponentClosedBeEatCP.dstPosition] = dstChessPiecesValue;
			dstChessPiecesValue = opponentClosedBeEatCP.beEatCP + ClosedChessPiecesValue;
			
			OptErrorLog("Only for test opponent pao hit cheat adjust2, be replace cp index = %d, value = %d, new dst value = %d",
		                opponentClosedBeEatCP.dstPosition, chessPiecesValue[opponentClosedBeEatCP.dstPosition], dstChessPiecesValue);
			
			return;
		}
	}
}

// 对手下棋前作弊调整对手将要翻开的棋子
void CRobotManager::opponentOpenCPCheatAdjust(short* chessPiecesValue, const short curOptSide, const unsigned short curPosition, short& srcChessPiecesValue)
{
	if (srcChessPiecesValue < ClosedChessPiecesValue) return;

	SRobotMoveChessPiecesVector ourOpenCPs;            // 翻开的我方棋子，玩家
	SRobotMoveChessPiecesVector opponentOpenCPs;       // 翻开的敌方棋子，机器人

    // 1）左边，2）右边，3）向上（最上一行棋子  0--08--16--24），4）向下（最下一行棋子  7--15--23--31）
    const short positionIndex[] = {-MaxDifferenceValue, MaxDifferenceValue, -1, 1};
	short robotPaoValue = (curOptSide == EXQSideFlag::ERedSide) ? com_protocol::EChessPiecesValue::EHeiQiPao : com_protocol::EChessPiecesValue::EHongQiPao;
	short moveIndex = 0;
	short cpValue = 0;
	for (unsigned int idx = 0; idx < (sizeof(positionIndex) / sizeof(positionIndex[0])); ++idx)
	{
		moveIndex = curPosition + positionIndex[idx];
		if (moveIndex >= MinChessPiecesPosition && moveIndex <= MaxChessPieCesPosition)
		{
			cpValue = chessPiecesValue[moveIndex];
			if (cpValue < com_protocol::EChessPiecesValue::ENoneQiZiClose)
			{
				if (m_msgHander->isSameSide(curOptSide, cpValue)) ourOpenCPs.push_back(SRobotMoveChessPieces(moveIndex, moveIndex, cpValue));
				else if (cpValue != robotPaoValue) opponentOpenCPs.push_back(SRobotMoveChessPieces(moveIndex, moveIndex, cpValue));
			}
		}
	}
	
	std::sort(ourOpenCPs.begin(), ourOpenCPs.end(), chessPiecesValueSort);  // 先排序
	std::sort(opponentOpenCPs.begin(), opponentOpenCPs.end(), chessPiecesValueSort);  // 先排序
	
	const short srcCPValue = srcChessPiecesValue - ClosedChessPiecesValue;
	short openValue = 0;
	if (!ourOpenCPs.empty())
	{
		if (!m_msgHander->isSameSide(curOptSide, srcCPValue) && srcCPValue != robotPaoValue && m_msgHander->canEatOtherChessPieces(srcCPValue, ourOpenCPs[0].beEatCP)) return;
		
		for (int cpIdx = MinChessPiecesPosition; cpIdx <= MaxChessPieCesPosition; ++cpIdx)
		{
			cpValue = chessPiecesValue[cpIdx];
			if (cpIdx != curPosition && cpValue >= ClosedChessPiecesValue)  // 未翻开的棋子
			{
				openValue = cpValue - ClosedChessPiecesValue;
				if (!m_msgHander->isSameSide(curOptSide, openValue) && openValue != robotPaoValue && m_msgHander->canEatOtherChessPieces(openValue, ourOpenCPs[0].beEatCP))
				{
					chessPiecesValue[cpIdx] = srcChessPiecesValue;
					srcChessPiecesValue = cpValue;
					
					OptErrorLog("Only for test opponent open CP cheat adjust1, be replace cp index = %d, value = %d, new src value = %d",
		                        cpIdx, chessPiecesValue[cpIdx], cpValue);
					
					return;
				}
			}
		}
	}
	
	else if (!opponentOpenCPs.empty())
	{
		if (m_msgHander->isSameSide(curOptSide, srcCPValue) && m_msgHander->canEatOtherChessPieces(opponentOpenCPs[0].beEatCP, srcCPValue)) return;
		
		for (int cpIdx = MinChessPiecesPosition; cpIdx <= MaxChessPieCesPosition; ++cpIdx)
		{
			cpValue = chessPiecesValue[cpIdx];
			if (cpIdx != curPosition && cpValue >= ClosedChessPiecesValue)  // 未翻开的棋子
			{
				openValue = cpValue - ClosedChessPiecesValue;
				if (m_msgHander->isSameSide(curOptSide, openValue) && m_msgHander->canEatOtherChessPieces(opponentOpenCPs[0].beEatCP, openValue))
				{
					chessPiecesValue[cpIdx] = srcChessPiecesValue;
					srcChessPiecesValue = cpValue;
					
					OptErrorLog("Only for test opponent open CP cheat adjust2, be replace cp index = %d, value = %d, new src value = %d",
		                        cpIdx, chessPiecesValue[cpIdx], cpValue);
					
					return;
				}
			}
		}
	}
}

}

