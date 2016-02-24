#pragma once

enum TimeConstants
{
    MINUTE = 60,
    HOUR   = MINUTE*60,
    DAY    = HOUR*24,
    WEEK   = DAY*7,
    MONTH  = DAY*30,
    YEAR   = MONTH*12,
    IN_MILLISECONDS = 1000
};

/******************** ~{3#A?EdVC~} ********************/
const int iONE_DAY_SECS = DAY;
const int iMaxVIP = 10;
//VIP~{PhR*5D3dV56n~}
const int iVIP_recharge[] =
{
    100,
    500,
    1000,
    2000,
    5000,
    10000,
    20000,
    50000,
    100000,
    200000
};
const int iMaxLevel = 130;
const int iMaxHeroStar = 5;
const int iMaxGoldCostConfirm = 100;
const int iMaxQuality = 6;
const int iMaxMapid = 9;
const int iCombatSilverMax = 20000;

//~{44=(=GI+J15DM<F,J}A?~}
const int maxCharactorSpic = 4;
//~{@.0HF55@7"QTO{7Q~}
const int iHornChatGoldCost = 5;

const int iTmpVip = 3;
const int iTmpVipEndLevel = 25;

/******************** ~{6%@89&D\?*7E~} ******************/
const int iSignOpenLevel = 5;
const int iOnlineLibaoOpenLevel = 5;
const int iFirstRechargeOpenLevel = 6;
const int iTimelimitActionOpenLevel = 7;
const int iRechargeActionOpenLevel = 8;
const int iTreasureOpenLevel = 10;
const int iDailyScoreOpenLevel = 13;
const int iDailyTaskOpenLevel = 16;
const int iPrestigeOpenLevel = 17;
const int iLotteryActionOpenLevel = 17;

const int iBossOpenLevel = 11;
const int iBankOpenLevel = 9;

/******************** ~{3G1$~} ********************/
//~{UPD<@dH4J1<d~}
const int iRecruitCD = 3600;
//~{>SCq<S3I~}(~{9%;w7@SyK0JU~}%)
const int iResidentBuff[3] = {15,10,10};
//~{G?VFK0JUO{7Q=p1R~}
const int iLevyCost = 10;
const int iLevyGoldTime[iMaxVIP+1] = {10,20,30,40,50,70,85,100,150,200,300};

/******************** ~{IL3G~} ********************/
//~{K"PBU[?[ILF7@dH4J1<d~}
const int iMallRefreshCD = 2*iONE_DAY_SECS;
const int iMallGoldFac[iMaxVIP+1] = {100,100,100,100,100,100,100,100,85,85,85};

/******************** ~{S"P[~}********************/
//~{PG<66TS&3I3$BJ~}
const double iHeroStarAdd[iMaxHeroStar] = {1.0,1.2,1.5,2.0,3.0};
//~{:O3IPG<63oBk~}
const double iCompoundHeroSilver[iMaxHeroStar-1] = {7200,54000,216000,864000};
//~{:O3IPG<6J}A?6TS&5D8EBJ~}
const double iCompoundHeroPer[iMaxHeroStar-1][8] =
{
    {0, 35, 70, 100, 0, 0, 0, 0},
    {0, 18, 36, 54, 72, 90, 0, 0},
    {0, 9, 18, 36, 54, 72, 90, 0},
    {0, 4.5, 9, 18, 36, 54, 72, 90}
};
//~{:O3IPG<6J}A?6TS&5D=p1RO{:D:M<S3I~}
const double iCompoundHeroGoldPer[iMaxHeroStar-1][8][2] =
{
    {{0, 0}, {10, 10.5}, {20, 21}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {20, 5.4}, {40, 10.8}, {60, 16.2}, {80, 21.6}, {100, 10}, {0, 0}, {0, 0}},
    {{0, 0}, {30, 2.7}, {60, 5.4}, {90, 8.1}, {120, 10.8}, {150, 13.5}, {180, 16.2}, {0, 0}},
    {{0, 0}, {40, 1.4}, {80, 2.8}, {120, 4.2}, {160, 5.6}, {200, 7}, {240, 8.4}, {280, 9.8}}
};
//~{7V=bPG<63oBk~}
const double iDecomposeHeroSilver[iMaxHeroStar] = {1500,4500,13500,40500,121500};
//~{H[A6Wn4s6SAPJ}~}
const int iSmeltMaxCnt = 6;
//~{H[A6K"PB@dH4J1<d~}
const double iSmeltRefreshCD = 7200;
//~{H[A6G?VFK"PB~}
const int iSmeltRefreshGold = 30;
//~{5c=pPG<63oBk~}
const double iGoldenHeroSilver[iMaxHeroStar] = {620000,620000,620000,1160000,1160000};
//~{5c=pS"P[5H<[3oBk~}
const double iGoldenHeroCostSub = 1500;

/******************** ~{W018~}********************/
//~{F7VJ6TS&3I3$BJ~}
const double iEquiptQualityAdd[iMaxQuality] = {0.2,0.6,1.0,1.5,2.0,2.2};
//~{:O3IF7VJJ}A?6TS&5D8EBJ~}
const double iCompoundEquiptPer[iMaxQuality-1][8] =
{
    {0, 35, 70, 100, 0, 0, 0, 0},
    {0, 18, 36, 54, 72, 90, 0, 0},
    {0, 9, 18, 36, 54, 72, 90, 0},
    {0, 4.5, 9, 18, 36, 54, 72, 90},
    {0, 4.5, 9, 18, 36, 54, 72, 90}
};

//~{:O3IF7VJJ}A?6TS&5D=p1RO{:D:M<S3I~}
const double iCompoundEquiptGoldPer[iMaxQuality-1][8][2] =
{
    {{0, 0}, {10, 10.5}, {20, 21}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {20, 5.4}, {40, 10.8}, {60, 16.2}, {80, 21.6}, {100, 10}, {0, 0}, {0, 0}},
    {{0, 0}, {30, 2.7}, {60, 5.4}, {90, 8.1}, {120, 10.8}, {150, 13.5}, {180, 16.2}, {0, 0}},
    {{0, 0}, {40, 1.4}, {80, 2.8}, {120, 4.2}, {160, 5.6}, {200, 7}, {240, 8.4}, {280, 9.8}},
    {{0, 0}, {40, 1.4}, {80, 2.8}, {120, 4.2}, {160, 5.6}, {200, 7}, {240, 8.4}, {280, 9.8}}
};

//~{:O3IPG<63oBk~}
const int iCompoundEquiptSilver = 13500;

//~{G?;/5H<6J}A?6TS&5D8EBJ~}
const double iUpgradeEquiptPer[10][2] =
{
    {8,9},
    {1,100},
    {2,45},
    {3,31},
    {4,22},
    {5,17},
    {6,14},
    {7,12},
    {8,11},
    {8,10}
};

//~{G?;/5H<6C?<6Tv<S3oBk~}
const int iUpgradeEquiptSilver = 360;
//~{G?;/7@=5<6O{:D~}
const int iUpgradeProtectGold = 50;

/******************** ~{811>~}********************/
//~{C?HU?I9%;w4NJ}~}
const int iCopyTotal = 20;
const int iCopyEvery = 2;
//vip~{6TS&9:Br4NJ}~}
const int iCopyCanBuy[iMaxVIP+1] = {2,3,3,4,4,4,4,4,4,4,4};
//~{9:Br4NJ}O{:D~}
const int iCopyBuyCost = 20;
//vip~{6TS&VXVC4NJ}~}
const int iCopyCanReset[iMaxVIP+1] = {0,1,1,2,2,2,2,2,2,2,2};
//~{VXVC~}
const int iCopyResetCost = 50;

/******************** ~{R0Mb3G3X~}********************/
const int iWildCityTotal = 100;
const int iWildCityCnt = 15;
const int iWildCityAttackCost = 3000;
const int iWildCityFastLevyVIP = 3;
const int iWildCityNotifyLevy = 5000;
//~{?IU<Al3G3XIOO^~}
const int iWildOwnMax[iMaxVIP+1] = {5,6,7,8,9,10,11,12,13,14,15};


/******************** ~{>:<<3!~}********************/
const int iDefaultArenaFreeTimes = 15;
//~{>:<<9:Br=p1RIOO^~}
const int iArenaBuyGoldMax = 20;
const int iArenaCD = 600;    //~{LtU=@dH4J1<d~} 600~{Ck~}
const int iArenaRankShowSize = 8;

/******************** ~{9R;z~}********************/
//~{9R;zWn4sJ1<d~}
const int iMaxExploreTime = iONE_DAY_SECS / 2;

/******************** ~{IqAiK~~}********************/
const int iShenlingFreeTime = 1;
const int iShenlingRefreshSkillGold = 50;
const int iShenlingResetGold = 50;

/******************** ~{I(54~}********************/
const int iSweepTime = 600;//~{I(545%4NO{:DJ1<d~}(second)
const int iSweepOpenLevel = 20;
const int iSweepOpenVip = 2;
const int iSweepFinishVip = 3;

/******************** ~{C?HU1XWv~}********************/
const int iDailyScoreTaskCnt = 6;//~{C?HU1XWvHNNqR;4NK"PB8vJ}~}
const int iDailyScoreTaskMax = 10;//~{C?HU1XWvHNNqIOO^~}
const int iDailyScoreTaskRefreshFree = 1;//~{C?HU1XWvHNNqCb7QK"PB4NJ}~}
const int iDailyScoreTaskRefreshCost = 10;//~{C?HU1XWvHNNqK"PBO{:D~}
const int iDailySpecilCnt = 3;//~{C?HU1XWvD?1jHNNqWn4s8vJ}~}
const int iDailySpecilAdd[3] = {20,30,45};//~{C?HU1XWvD?1jHNNq8vJ}6TS&=1@x~}

/******************** BOSS********************/
const int iBossMinLevel = 30;//BOSS~{3uJ<5H<6~}
const int iBossCombatDelay = 120;//BOSS~{5H:rJ1<d~}
const int iBossEndCDCost = 5;//BOSS~{Ck@dH4O{:D~}
const int iBossInspireGold = 20;//~{9DNhO{:D~}
const int iBossInspirePer = 10;//~{9DNh<S3I~}
const int iBossCombatCD = 50;

/******************** ~{9DNh~}********************/
const int iInspireGoldMaxLevel = 10;

//~{W*ELO{:D~}
const int iLotteryCost[3] = {50,200,500};
const int iLotteryGet = 50;
const int iLotteryRechargeGet = 1000;

