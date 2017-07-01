
/* author : limingfan
 * date : 2016.12.27
 * description : 翻翻棋机器人实现
 */

#ifndef CROBOT_MANAGER_H
#define CROBOT_MANAGER_H

#include <string>
#include <unordered_map>
#include <map>
#include <vector>

#include "SrvFrame/CModule.h"
#include "SrvFrame/CDataContent.h"
#include "base/CMemManager.h"
#include "common/CommonType.h"
#include "TypeDefine.h"


using namespace std;
using namespace NProject;


namespace NPlatformService
{

class CSrvMsgHandler;


// 机器人管理
class CRobotManager : public NFrame::CHandler
{
public:
	CRobotManager();
	~CRobotManager();

public:
	bool load(CSrvMsgHandler* msgHandler);
	void unLoad();
	
	void updateConfig();
	
public:
	XQUserData* createRobotData(uint64_t gold, unsigned int playMode, unsigned int arenaId);
	void releaseRobotData(XQUserData* xqUD);
	
	const char* generateRobotId(const unsigned int index, char* robotId, const unsigned int len);
	bool isRobot(const char* id);
	XQUserData* getRobotData(const char* robotId);

public:
	// 机器人下棋走子
    void robotPlayChessPieces(const char* robotId, const XQChessPieces* xqCP);
	
	// 机器人下完一盘棋
	void finishPlayChess(const XQUserData* ud, const unsigned int winGold, const unsigned int lostGold = 0);
	
	// 是否需要作弊
	bool needCheat(const XQUserData* ud, const XQChessPieces* xqCP, const short cheatSide, const unsigned int baseRate);
	
	// 对手炮击下棋前作弊调整未翻开的棋子
	void opponentPaoHitCheatAdjust(short* chessPiecesValue, const short curOptSide, short& dstChessPiecesValue);
	
	// 对手下棋前作弊调整对手将要翻开的棋子
    void opponentOpenCPCheatAdjust(short* chessPiecesValue, const short curOptSide, const unsigned short curPosition, short& srcChessPiecesValue);
	
private:
    // 机器人下棋走子
    void robotMoveChessPieces(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	// 找到可以移动的棋子
    bool findCanMoveChessPieces(const XQChessPieces* xqCP, const SRobotMoveChessPiecesVector& ourChessPosition,
								SRobotMoveChessPieces& moveCPInfo, SRobotMoveChessPiecesVector& firstMoveCPVector, SRobotMoveChessPiecesVector& allMoveCPVector);
	
	// 找到可以翻开的棋子
    bool findCanOpenChessPieces(const XQChessPieces* xqCP, const ChessPiecesPositions& closedChessPosition,
								SRobotMoveChessPieces& moveCPInfo, SRobotMoveChessPiecesVector& firstOpenCPVector);
	
    // 找出占据优势的棋子移动位置
    bool findAdvantageChessPiecesPosition(const XQChessPieces* xqCP, const SRobotMoveChessPiecesVector& ourSideCP, const unsigned int maxBeEatValue, const unsigned int beEatCount,
	                                      com_protocol::ClientPlayChessPiecesNotify& plCPNotify, SRobotMoveChessPiecesVector* canMoveCPs = NULL);
    
	// 是否可以吃掉对方的棋子，currentIsOurSide是否当前下棋方为我方
    int eatChessPieces(const short* chessPiecesValue, const short curOptSide, const int opponentCurHp, const SRobotMoveChessPiecesVector& ourSideBeEatCP, const SRobotMoveChessPiecesVector& opponentSideBeEatCP,
					   const bool currentIsOurSide, unsigned int& index, const short newMvPos);
    bool eatChessPieces(const short* chessPiecesValue, const short curOptSide, const int opponentCurHp, const SRobotMoveChessPiecesVector& ourSideBeEatCP, const SRobotMoveChessPiecesVector& opponentSideBeEatCP, const short newMvPos);					   
					   
    // 是否可以吃掉该棋子
    bool canEatChessPieces(const short* chessPiecesValue, const short curOptSide, const int opponentCurHp, const SRobotMoveChessPieces& beEatChessPieces, short* afterEatChessPiecesValue, const unsigned int cpValueSize,
						   SRobotMoveChessPiecesVector& ourSideBeEatCP, SRobotMoveChessPiecesVector& opponentSideBeEatCP, SRobotMoveChessPiecesVector& ourChessPosition,
						   vector<short>& closedChessPosition, bool filterNaturalProtect = false);

    // 找出双方可以互相吃的棋子信息
    void findCanEatChessPieces(const short* chessPiecesValue, const short curOptSide,
							   SRobotMoveChessPiecesVector& ourSideBeEatCP, SRobotMoveChessPiecesVector& opponentSideBeEatCP,
							   SRobotMoveChessPiecesVector& ourChessPosition, vector<short>& closedChessPosition);
							   
	// 找出棋子可以移动的位置
    bool canMovePosition(const short* chessPiecesValue, const unsigned short curPosition, vector<short>& newPositions);
							   
	// 炮可以吃掉的棋子
    unsigned int paoEatChessPieces(const short* chessPiecesValue, const unsigned short opponentSide, const int opponentIdx, SRobotMoveChessPiecesVector& beEatCP, const bool eatClosed = false);
	
	// 棋子可以吃掉的棋子
    unsigned int cpEatChessPieces(const short ourMinValue, const short ourMaxValue, const short* chessPiecesValue, const int opponentIdx, const int opponentValue, SRobotMoveChessPiecesVector& beEatCP);
	
	// 在steps步之内是否可以成功躲避防止被对方吃掉
    bool canDodge(const int steps, const short* chessPiecesValue, const short curOptSide, const int opponentCurHp, const unsigned int maxBeEatValue, const unsigned int beEatCount,
				  const short ourCurrentPosition, const short ourLastPosition, const short opponentSrcPosition);
				  
	// 移动的棋子作为炮台之后，下一步却被对方的目标棋子（作为我方炮的目标棋子）吃掉
	bool isBatteryBeEat(const short* afterMoveValue, const short newMovePosistion, const SRobotMoveChessPieces& opBeEatCP, const SRobotMoveChessPiecesVector& ourSideBeEatCP);
				  
private:
	// 获取被吃棋子的数量，去掉重复被吃的棋子
	unsigned int getBeEatChessPiecesCount(const SRobotMoveChessPiecesVector& beEatCPs);
	
	// 在自然防御条件下，找出可以移动的最优棋子
    EFindMoveChessResult findCanMoveMostAdvantageCP(const XQChessPieces* xqCP, const SRobotMoveChessPiecesVector& canMoveCPs, SRobotMoveChessPieces& moveCPInfo);

	// 我方的炮是否可以吃掉我方的棋子
	bool paoCanEatOurSideChessPieces(const short ourValue);
	
	// 机器人下棋前作弊调整未翻开的棋子
	void robotPlayCheatAdjustCP(XQChessPieces* xqCP);
	
	// 获取机器人赢率百分比
	unsigned int getWinRatePercent(const XQUserData* ud);
	
private:
	void generateRobotInfo();
	
private:
	CSrvMsgHandler* m_msgHander;
	SRobotInfoVector m_freeRobots;
	RobotIdToData m_robotId2Data;
    PlayModeRobotWinLostRecord m_robotWinLostRecord;  // 机器人输赢记录
	
	
DISABLE_COPY_ASSIGN(CRobotManager);
};

}

#endif // CROBOT_MANAGER_H

