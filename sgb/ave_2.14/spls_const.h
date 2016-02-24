#pragma once

#define STRONGHOLD_ID(m,s,i) (24*(m-1)+(s-1)*8+i)

const int iOpenNearMap = 3;

//最大地图数量
const int max_map_id = 8;

//目前开放地图
const int iMaxOpenMap = 5;

//创建角色时的图片数量
const int maxCharactorSpic = 16;
//每日免费可攻击精英次数
const int FreeTimes = 5;

//初始化角色自带阵型
const int iDefaultZhenType = 1;

//系统消息的格式
const std::string strSystemMsgFormat = "{\"cmd\":\"chat\",\"ctype\":5,\"s\":200,\"m\":\"$M\",\"type\":$T}";
//玩家普通消息的格式
const std::string strCharMsgFormat = "{\"cmd\":\"chat\",\"ctype\":8,\"s\":200,\"m\":\"$M\"}";

//创建角色时可以选择的武将
const int create_choose_generals_id[6] = {3,1,5,4,2,6};

//兵器数量上限
const int iMaxWeaponNums = 200;

//士气修改 wlj 20120815
const int iAttackSuccessShiqi = 50;    //攻击成功增加自己的士气值
const int iBeAttackedShiqi = 0;        //被攻击增加士气值

const int iStrongholdHistoryNums = 5;        //攻略数量5条历史战报

const int iArchiveFirstStrongholdSuccess = 1;    //保存第一个攻打成功的记录

const int iSplsFirstYear = 200;        //开始年份 200年

const int iNewbieGoGoGoSecs = 259200;        //新手冲锋号倒计时72小时

const int iRechargeFirst = 604800;        //第一期充值奖励有效期
const int iRechargeSecond = 604800;        //第二期充值奖励有效期

const int iNewbieGoGoGoCheaper = 20;        //新手冲锋号期间内，金币休息优惠20%

/*********************** 兵器相关 ***************************/

//各种品质武器的价格系数
//const int baseWeaponPriceFactor[5] = {1,2,3,5,10};

//各地图场景的武器铺初始刷新银币价格
//const int baseWeaponRefreshSilver[max_map_id] = {50,400,1200,2800,5000};

//根据mapid，武器类型，品质算出spic
//#define WEAPON_SPIC(mapid,type,quality) (25*(mapid-1)+5*(type-1)+quality)

//使用金币刷新兵器铺的数量
//const int iRefreshWeaponGoldCost = 2;

/*********************** 休息相关 ***************************/

//活动休息冷却是时间1小时  3600秒
const int iEventRestCooltime = 3600;

//每次休息增加的军令
const int iRestLing = 10;

//活动休息恢复军令随次数变化,总共可以休息5次
const int iFreeRestLing[5] = {4,4,6,6,10};
//活动休息CD随次数变化,总共可以休息5次
const int iFreeRestCD[5] = {10,15,20,20,20};

//VIP休息次数                0  1  2  3  4  5  6  7  8  9  10 11 12
const int iVIPRestTimes[13] = {5, 10, 15, 20, 25, 30, 35, 40, 50, 60, 70, 80, 100};

//系统恢复军令上限
const int iSystemRecoverMaxLing = 45*4;

//世界聊天的金币消费
const int iWorldChatGoldCost = 0;

//刷新商店金币消费
const int iRefreshShopGoldCost = 30;
#ifdef JP_SERVER
const int iRefreshShopGoldVIP[] = {0, 1, 2, 3, 5, 8, 10, 15, 20, 25, 30, 35, 50};
#else
const int iRefreshShopGoldVIP[] = {0, 0, 0, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3};
#endif

/*********************** 竞技场相关 ***************************/

//银币宝箱=地图银币系数*主将等级
//地图编号    地图银币系数
//第1图    220
//第2图    340
//第3图    420
//第4图    500
//第5图    550
//第6图    600
const int iSilverBoxCounts[max_map_id] = {220,340,420,500,550,600,600,600};
//金币宝箱概率
//70% +5
//20% +10
//10% +20
const int iGoldBoxChances[3] = {70, 90, 100};
//矿石宝箱概率
//50% +1
//35% +2
//15% +4
const int iOreBoxChances[3] = {50, 85, 100};

/* 2012-1-4 qj调整竞技场年度奖励
1、每年冬天晚上23:00，每个地图的竞技场根据即时排名结算年度奖励。
2、结算后系统自动将军令奖励发放给玩家
名次    年度奖励
第1名    20个军令、1000声望
第2名    15个军令、800声望
第3名    12个军令、700声望
4-6名    10个军令、600声望
5-10名    9个军令、500声望
11-20名    8个军令、400声望
21-30名    7个军令、300声望
31-50名    6个军令、250声望
51-100名5个军令、200声望
101-200名3个军令、150声望
201-500名2个军令、100声望

*/
const int iRaceRankReward[][3] =
{
    {1, 20, 1000},    //排名1
    {2, 15, 800},    //排名2
    {3, 12, 700},    //排名3
    {6, 10, 600},    //排名4-6
    {10, 9, 500},    //排名7-10
    {20, 8, 400},    //排名11-20
    {30, 7, 300},    //排名21-30
    {50, 6, 250},    //排名31-50
    {100, 5, 200},    //排名50-100
    {200, 3, 150},    //排名101-200
    {500, 2, 100}    //排名201-500    
};

const int iRaceWinPrestige = 10;
const int iRaceLosePrestige = 5;

#ifdef TEST_SERVER
const int iRaceCD = 10;    //挑战冷却时间 30秒
#else
const int iRaceCD = 600;    //挑战冷却时间 600秒
#endif

//const int iRaceFreeTimes = 15;    //免费15次挑战
const int iRaceScoreTimes = 20;    //前20次挑战有积分

//每场关卡战斗的最大秒数
const int TIME_MAX_A_COMBAT = 30;

//强化真冷却时间29分
const int TIME_MAX_ENHANCE = 29 * 60;
//强化无冷却VIP等级
const int iEnhanceNoCDVip = 1;


/*********************** 技能相关 ***************************/

const int max_skill_level = 60;        //技能最高等级
const int total_skill_type = 21;        //技能总数

const int skill_research_mins = 1;    //技能研究加点时间
const int skill_teacher_nums = 3;        //技能研究者数量
const int skill_queue_max = 5;            //技能研究队列最大数量
const int skill_queue_data[skill_queue_max][2] =
{
    {0,0},        //第一个队列免费给
    {0, 50},    //第二个队列50金币
    {1, 100},    //第3个队列vip1 100金币
    {2, 300},    //第4个队列vip2 300金币
    {6, 500}    //第5个队列vip6 500金币
};

//金钻队列速度加成百分比
const int skill_vip_queue_more_speed = 30;

const int skill_queue_levelup_gold = 400;//400金币可以升级队列


//银币刷新技能训练
const int skill_teacher_update_silver[max_map_id] =
{
    1000, 4000, 12000,28000,50000,75000,75000,75000
};

//金币刷新技能训练
const int iSkill_teacher_update_gold = 2;


/*********************** 武将训练相关 ***************************/

const int general_book_nums = 1;        //武将兵书数量
const int general_queue_max = 5;            //武将研究队列最大数量
const int general_vip_queue_more_speed = 3600;//金钻队列减少冷却时间
const int general_vip_queue_more_level = 5;//金钻队列增加训练效果
const int general_queue_levelup_gold = 300;//400金币可以升级队列
#ifdef JP_SERVER
const int general_queue_data[general_queue_max][2] =
{
    {0, 0},        //第一个队列免费给
    {0, 50},    //第2个队列vip1 50金币
    {0, 100},    //第3个队列vip3 100金币
    {0, 200},    //第4个队列vip4 200金币
    {0, 400}    //第5个队列vip5 400金币
};
#else
const int general_queue_data[general_queue_max][2] =
{
    {0, 0},        //第一个队列免费给
    {0, 10},    //第2个队列vip1 10金币
    {0, 100},    //第3个队列vip3 100金币
    {0, 200},    //第4个队列vip4 200金币
    {0, 400}    //第5个队列vip5 400金币
};
#endif

//不同地图刷新训练书费用
const int general_book_update_silver[max_map_id] =
{
    3000, 6000, 18000, 42000, 75000, 120000, 120000, 120000
};

const int iBookRefreshVIP = 0;
const int iBookBestRefresh = 6;
#ifdef JP_SERVER
const int iBookBestRefreshGold = 60;
#else
const int iBookBestRefreshGold = 100;
#endif

/*********************** 冶炼相关 ***************************/

const int iSmeletSpeedVip = 8;//金币加速冶炼vip
const double smelt_ore_fac[6] = {0.0,0.0,1.0,1.5,2.0,2.5};        //冶炼消耗矿石系数(品质决定)
const int smelt_queue_max = 5;            //冶炼队列最大数量
const int smelt_task_max = 3;            //冶炼任务最大数量

const int smelt_update_silver[max_map_id] =
{
    0, 0, 20000, 50000, 80000, 100000, 100000, 100000
};

/*********************** 状态相关 ***************************/

//最大状态数量
const int max_state_nums = 3;

const int max_refresh_state_types = 24;    //前24种状态开放给用户刷新

const int iRefreshStateSilver[max_map_id] = {-400,-400,-1200,-2800,-5000, -9000, -9000, -9000};    //每个图刷状态银币消耗

const int iRefreshStateGold = -1;        //刷状态金币消耗
const int iRefreshStateWithGold_VIP_level = 2;    //使用金币刷状态需要的vip等级


/*********************** 掉落相关 ***************************/

const int map_normal_silver_field[max_map_id] = {3,6,8,12,18,24,24,24};//24
const int map_elite_silver_field[max_map_id] = {4,8,10,18,24,30,30,30};//30

/* vip 特权 */

//使用金币刷新兵器铺的vip要求
const int iRefreshWeaponWithGold_VIP_level = 2;

//金币刷新技能训练,需要的vip等级
const int iSkill_teacher_update_gold_vip_level = 3;

const int skill_queue_levelup_vip = 5;    //vip5可以升级技能训练队列

const int skill_queue_speedup_vip = 0;    //加速研究需要的vip等级

const int general_queue_levelup_vip = 4;//vip4可以升级武将训练队列
const int iTrainSpeedVip = 0;    //加速英雄训练开放vip等级

//精英关卡数量加1( vip 7 )
const int iMoreStrongholdAttackVIP_level = 7;

//世界聊天的vip要求
const int iWorldChatVipLevel = 0;

//世界聊天要求主将等级
const int iWorldChatLevel = 10;

const int iTEST_recharge[] =
{
    300,
    300,
    300,
    1500,
    2500,
    5000,
    10000,
    25000,
    50000,
    90000,
    120000,
    180000
};


//金币消费提示最大值
const int iMaxGoldCostConfirm = 100;


//银币鼓舞成功率
const int iInspireSilver[] = {    30,20,20,10,10};

//银币鼓舞消耗 600*等级
const int iInspireSilverCost = 600;

//金币鼓舞消耗
const int iInspireGoldCost[] = {10,15,20,25,30};

//金币鼓舞vip等级
const int iInspireGoldVIPLevel = 3;

//boss战的CD
const int iBossCombatCD = 30;
const int iBossMinCombatCD = 20;
//boss战可涅槃次数
const int iBossEndCDTimes = 5;

//boss战单场获得声望上限
const int iBossCombatExpLimit = 1000;

//boss战开启后3分钟准备时间
const int iBossCombatDelay = 180;

#ifdef CAMP_RACE_TWO_QUEUE    //是否划分为两个队列
//阵营战等级划分
const int iCampRaceLevel_low = 50;
#else
//阵营战等级划分
const int iCampRaceLevel_low = 9999;
#endif

//阵营战单场最高声望
const int iCampRaceMaxPrestige = 2000;

//阵营战轮空冷却时间(秒)
const int iCampRaceNomatchCD = 20;
//阵营战战斗最短冷却时间(秒)
const int iCampRaceFightCD = 30;
//阵营战匹配战斗周期(秒)
const int iCampRaceMatchPeriod = 20;

//军团祭祀活动(金币，功勋，声望，VIP等级)
const int iCorpsJisi[][4] =
{
    {0, 200, 10, 0},
    {30, 0, 100, 0},
    {90, 0, 1000, 1}
};

//每日军团探索次数
const int iCorpsExploreTimesOneday[] = {0,10,15,20,25,30,30,30,30,30,30};

//军团捐献
const int iDonateGoldVIP = 2;

//军团宴会军团等级要求
const int iCorpsLevelForParty = 2;

//军团宴会人数上限
const int iCorpsYanhuiMax = 7;

//宴会获得军令
const int iCorpsYanhuiLing = 7*4;

//创建军团所需vip
const int iCorpsCreateVip = 1;

//屯田任务数量
const int iFarmTaskNum = 4;
const int iFarmSpeedVip = 4;
//屯田延迟次数VIP限制
const int iDelayFarmVIP[] = {0,0,0,1,2,3,3,6,12,12,12,12,12};
//屯田浇水次数
const int iFarmWater = 3;
const int iFarmFriendWater = 30;

//精英扫荡单个精英消耗时间(second)
const int iSweepEliteTime = 120;
const int iSweepEliteFinishVip = 2;


/*********************** 洗髓相关 ***************************/
//const int iWashBuyVip = 7;
//const int iWashSilverFac = 80000;
const int iWashSilver = 50000;
const int iWashGold = 10;
const int iWashUltra = 100;
const int iWashFullValVIP = 6;//洗髓上限翻倍VIP限定
//洗髓类型对应vip要求，消耗，加成下限(低于此属性必加)，洗髓保底
const int iWashConfig[][4] =
{
    {0, 2000, 0, 1},//普通
    {0, 2, 40, 20},//青铜
    {3, 10, 80, 40},//白金
    {5, 20, 160, 80},//钻石
#ifdef JP_SERVER
    {6, 50, 400, 200}//至尊
#else
    {6, 100, 400, 200}//至尊
#endif
};

/*const int iWashTypeMax = 11;//洗髓概率档次总共12档
const int iWashGroup[iWashTypeMax] = {0,2,3,5,7,9,11,13,16,21,31};//武将洗髓加点所属的类别
const int iWashPer[3][iWashTypeMax] =
{
    {100, 100, 100, 100, 100, 100, 100, 100, 80, 60, 50},//treasure
    {80, 50, 25, 15, 5, 2, 0, 0, 0, 0, 0},//silver
    {100, 80, 60, 50, 40, 20, 15, 5, 2, 1, 0}//gold
};*/

/*********************** 重生相关 ***************************/
const int iRebornVip = 4;
const int iRebornOpenLevel = 50;
const int iRebornGold = 30;
const int iRebornFreeVip = 6;
const int iRebornFreeTimes = 1;
const int iOnekeyRebronVip = 8;    // V8一键重生
const int iRebornPointMin = 30;//重生最低需要30点
const int iRebornPoint[] = {15, 25, 35, 40, 50, 65, 75, 80, 90, 100, 100};//重生消耗点数
const double iChengZhangMax[] = {8, 12, 18, 26, 36, 45};//成长率由品质决定上限

/*********************** 生辰纲相关 ***************************/
//刷新进阶概率
const int iGuardRefreshPer[] = {70, 35, 15, 8};

#ifdef JP_SERVER
//刷新护送纲
const int iGuardRefreshGold = 5;
//召唤生辰纲
const int iGuardCallGold = 100;
//秒护送CD
const int iGuardFinishGold = 20;//每10分钟20金
//秒劫取CD
const int iGuardSpeedGold = 20;
#else
//刷新护送纲
const int iGuardRefreshGold = 10;
//召唤生辰纲
const int iGuardCallGold = 200;
//秒护送CD
const int iGuardFinishGold = 30;//每10分钟30金
//秒劫取CD
const int iGuardSpeedGold = 10;
#endif

const int iGuardFinishVip = 0;
const int iGuardCallVip = 5;
//劫取CD
const int iGuardCD = 600;
//鼓舞
const int iGuardInspireGold = 20;
const int iGuardInspireVIP = 4;

/*********************** 通商相关 ***************************/
const int iTradeTypeNum = 3;//通商类型数量
const int iTradeTypeLevel[iTradeTypeNum][2] =//通商各区间等级范围
{
    {16, 49},
    {50, 64},
    {65, 1000}
};
//通商奖励银币系数
const int iTradeSilver[]={12000,30000,50000};
const int iTradeEveryday = 4;
const int iTradeTakeSeatLing = 0;//通商入席所需军令
const int iTradeRobSeatLing = 0;//通商夺席所需军令
const int iTradeProtectGold = 10;//通商保护10金币
const int iTradeTime = 600;//通商个人奖励时间周期(second)
const int iTradeSpeedTime = 3600;//通商加速时间(second)
const int iTradeTotalTime[] = {1800,1800,1800};//通商时间

/***********************征收相关 ***************************/
#ifdef JP_SERVER
const int iLevyFreeTime = 1;//每天免费征收次数
#else
const int iLevyFreeTime = 0;//每天免费征收次数
#endif
const int iLevyMoreTimeVIP = 6;//批量征收10次需要VIP
const int iLevyMaxTimeVIP = 10;//批量征收100次需要VIP
const int iLevyTimes[]={10,20,30,40,50,70,85,100,150,200,250,300,500};//各vip征收次数

/***********************战马相关 ***************************/
const int iHorseTurnsMax = 5;//开放最大转身次数
const int iHorseStarsMax = 10;//开放最大星级
const int iHorseTrainSilver = 15000;//银币升星消费
const int iHorseTrainGold = 100;//金币升星消费
const int iHorseTrainSilverAdd = 40;//银币升星经验
const int iHorseTrainGoldAdd = 48;//银币升星经验
//const int iHorseTrainTime = 20;//每天培养战马次数
//const int iHorseGoldTrainTime = 3;//每天金币培养战马次数

const int iHorseMoreTrainVip = 6;//高级培养所需VIP
const int iHorseQuality[] = {0,1,2,3,4,5};//各转身战马颜色
const int iHorseActionTime = 2592000;//战马活动持续时间(30days)
const int iHorseFruitTime = 259200;//果子持续时间(3days)

/***********************家丁相关 ***************************/
const int iServantRandomCatchTime_Gold_First = 2;//第一次购买随机
const int iServantRandomCatchTime_Gold_Add = 2;//购买随机递增
const int iServantRandomCatchTime = 1;//每天免费随机抓捕次数
const int iServantCatchTime = 10;//每天免费抓捕次数
const int iServantCatchTime_First = 5;//第一次购买抓捕消耗
const int iServantCatchTime_Add = 5;//购买抓捕次数递增消耗
const int iServantRescueTime = 3;//每天免费解救次数
const int iServantInteractTime = 6;//每天免费互动次数
const int iServantCD = 1200;//互动冷却时间
const int iServantResistTime = 5;//每天免费反抗次数
const int iServantResistTime_Gold = 10;//购买反抗消耗
const int iServantSOSTime = 2;//每天免费求救次数
const int iServantBeSOSTime_F = 1;//每天被好友求救次数
const int iServantBeSOSTime_C = 1;//每天被军团求救次数
const int iServantExploitGold = 2;//剥削所需金币
const int iServantEscapeVIP = 3;//赎身所需VIP
const int iServantEscapeGold = 50;//赎身所需金币
const int iServantTime = 600;//家丁加产量间隔时间
const int iServantMaxFac = 220;//家丁产量上限系数

/***********************多人副本相关 ***************************/

//副本npc部队最多6组
const int iMaxGroupCopyArmys = 6;
//副本玩家最多3人
const int iMaxGroupCopyMembers = 3;
//副本玩家最少2人开战
const int iMinGroupCopyMembers = 2;

//多人副本开放等级
const int iGroupCopyOpenLevel = 20;

/*************场景精英关卡功勋奖励 *****************/
const int iStageGongxun[]={200,500,800,1100,1450,1800,2150,2500,2900,3350,3800,4300,4850,5450,6050,6550,7050,7550};

/******************** 各个功能的开放等级 ********************/

/**********************内政 ********************************/
//探索开放 击败第5个关卡第6个怪
const int iExploreOpenStronghold = (5-1)*8 + 6;
//商店开放
const int iShopOpenStronghold = 38;
//屯田开放 击败第8个关卡第6个怪
const int iFarmOpenStronghold[] = {50, 92, 118, 138, 158, 178};//(8-1)*8 + 6;
//通商开放
const int iTradeOpenStronghold = 62;

/**********************军事 *******************************/
//技能 未知开放??????????????
const int iSkillOpenStronghold = 9999;
//布阵开放
const int iZhenOpenStronghold = 10;
//战马开放 击败第4个关卡第5个怪
const int iHorseOpenStronghold = (4-1)*8+5;
//科技开放 秘法开放 击败第1个关卡第8个怪
const int iWeaponOpenStronghold = 8;

/**********************武将 *******************************/
//训练开放 击败第3个关卡第4个怪
const int iTrainOpenStronghold = (3-1)*8+4;
//装备开放 击败第1个关卡第4个怪
const int iEquipOpenStronghold = 3;
//强化开放 击败第1个关卡第7个怪
const int iEquipEnhanceOpenStronghold = 7;
//洗髓开放 击败第3个关卡第6个怪
const int iWashOpenStronghold = (3-1)*8 + 6;
//重生开放关卡
const int iRebornOpenStronghold = STRONGHOLD_ID(3,2,4);
//镶嵌开放
const int iXiangqianOpenStronghold = 54;

//军团开放 击败第5个关卡第4个怪
const int iCorpsOpenStronghold = 36;

/******************上方按钮开放 **********************/
//游戏助手开放 击败第5个关卡第2个怪
const int iHelperOpenStronghold = (5-1)*8+2;
//竞技场开放 击败第4个关卡第6个怪
const int iRaceOpenStronghold = (4-1)*8+6;
//神兽开放 击败第6个关卡第6个怪
const int iBossOpenStronghold = (6-1)*8 + 6;
//阵营战开放 击败第6个关卡第4个怪
const int iCampraceOpenStronghold = (6-1)*8 + 4;
//精英战役开放
const int iEliteOpenStronghold = (8-1)*8 + 2;

//其他开放
//家丁后台实际开放2图3关卡8个怪
const int iServantRealOpenStronghold = 48;
//家丁可以被玩家看到3图1关卡8个怪
const int iServantOpenStronghold = 56;
//征收开放 击败第2个关卡第6个怪
const int iLevyOpenStronghold = (2-1)*8 + 6;
//护送开放 4图1关卡6个怪
const int iGuardOpenStronghold = 78;
//官职开放 ?????????
const int iOfficalOpenStronghold = 0;
//购买军令开放
const int iBuyLingOpenStronghold = 42;
//扫荡开放
const int iSweepOpenStronghold = (2-1)*3*8 + (3-1)*8 + 8;

//索引+1=开放的武将上限，值=需要的关卡
const int iGeneralLimitStronghold[] = {0,0,0,0,28,70,80,90,100,9999};

//默认上阵人数
const int iDefaultUpGenerals = 3;

//第一个增加上阵人数的关卡
const int iFirstUpGeneralStronghold = 78;
//第二个增加上阵人数的关卡
const int iSecondUpGeneralStronghold = 122;
//周排行开启1-3-5
const int iRankEventOpenStronghold = 0;
//钱庄开启2-1-3
const int iBankOpenStronghold = 0;

//八卦阵开放关卡334
const int iMazeOpenStronghold = (3-1)*3*8 + (3-1)*8 + 4;

//七日目标关卡314
const int iSevenOpenStronghold = (3-1)*3*8 + (1-1)*8 + 4;

//将星录开放关卡
const int iJxlOpenStronghold = 10;

//将魂开启关卡
const int iGeneralSoulOpenStronghold = 10;

/**************************************************************/

//屯田队列开放等级
const int iFarmOpenLevel[] = {26,47,60,70,80,90};

//探索刷新VIP
const int iExploreRefreshVIP = 1;
const int iExploreBaseOpenLevel = 41;
const int iExploreTianOpenLevel = 50;
const int iExploreRefreshTimes[] = {0,1,3,8,8,8};
const int iExploreRefreshUnlimitVIP = 6;

//开放最高角色等级
const int iMaxCharLevel = 120;

//连续登录奖励
const int iContinueLoginStronghold = 38;
//在线奖励
const int iOnlineGiftStronghold = 4;

//首充礼包 2-1-2
const int iFirstRechargeStronghold = 0;

//收藏礼包出现关卡2-1-3
const int iCollectLibaoStronghold = STRONGHOLD_ID(2,1,3);

//刷新时间
const int iRefreshHour = 5;
const int iRefreshMin = 0;

//GM福利
const int iGM_Level_Max = 2;
const int iGM_Cancel = 7;
const int iGM_Level_Gold[] = {1000,2000};

//VIP试用
const int iTmpVip_level = 4;
#ifdef TEST_SERVER
const int iTmpVip_time = 60;
#else
const int iTmpVip_time = 7200;
#endif
const int iTmpVip_OpenStronghold = (6-1)*8+6;

/******************** 差异化常量配置 ********************/
#ifdef TW_SERVER
const int iTradeSpeedGold = 10;//通商加速所需金币每小时10金币
const int iRestGold_First = 15;//金币休息第一次消费
const int iRestGold_Add = 5;//金币休息消费递增
const int stronghold_times_cost = 2;//购买精英攻击次数消费

const int iExploreRefreshGold_First = 5;//第一次刷新
const int iExploreRefreshGold_Add = 5;//之后刷新递增
const int iExploreRefreshGold_Max = 30;//刷新消费上限

const int smelt_queue_data[smelt_queue_max][2] =
{
    {0,0},        //第一个队列免费给
    {0, 250},    //第2个队列 250金币
    {4, 500},    //第3个队列vip4 500金币
    {5, 750},    //第4个队列vip5 750金币
    {7, 1000}    //第5个队列vip7 1000金币
};
const int iSmeletSpeedGold = 2;//冶炼加速每10分钟的消费
const int iBookRefreshGold = 2;//刷新训练兵书消费
const int iTrainSpeedGold = 1;//加速英雄训练每10分钟消费
const int iWashBuyFac = 20;//金币购买洗髓的公式系数
const int iRegetGold_First = 5;//第一次再次获得消费
const int iRegetGold_Add = 5;//再次获得消费递增
const int skill_queue_speedup_mins = 5;    //每金币能加速的分钟
//VIP需要的充值额
const int iVIP_recharge[] = 
{
    100,
    250,
    500,
    1500,
    2500,
    5000,
    10000,
    25000,
    100000,
    500000
};
#else
const int iTradeSpeedGold = 30;//通商加速所需金币每小时30金币
const int iRestGold_First = 30;//金币休息第一次消费
const int iRestGold_Add = 10;//金币休息消费递增
const int stronghold_times_cost = 5;//购买精英攻击次数消费

const int iExploreRefreshGold_First = 2;//第一次刷新
const int iExploreRefreshGold_Add = 2;//之后刷新递增
const int iExploreRefreshGold_Max = 20;//刷新消费上限

const int smelt_queue_data[smelt_queue_max][2] =
{
    {0,0},        //第一个队列免费给
    {0, 100},    //第2个队列 100金币
    {4, 200},    //第3个队列vip4 200金币
    {5, 500},    //第4个队列vip5 500金币
    {7, 1000}    //第5个队列vip7 1000金币
};
const int iSmeletSpeedGold = 5;//冶炼加速每10分钟的消费
const int iBookRefreshGold = 2;//刷新训练兵书每次递增金币
const int iTrainSpeedGold = 2;//加速英雄训练每10分钟消费
const int iWashBuyFac = 50;//金币购买洗髓的公式系数
const int iRegetGold_First = 10;//第一次再次获得消费
const int iRegetGold_Add = 5;//再次获得消费递增
const int skill_queue_speedup_mins = 5;    //每金币能加速的分钟
#ifdef JP_SERVER
//VIP需要的充值额
const int iVIP_recharge[] = 
{
	100,
	500,
	1000,
	2000,
	5000,
	10000,
	30000,
	50000,
	100000,
	200000,
	350000,
	600000
};
#else
//VIP需要的充值额
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
    200000,
    500000,
    1000000
};
#endif
#endif


