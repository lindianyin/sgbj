#pragma once

#define STRONGHOLD_ID(m,s,i) (24*(m-1)+(s-1)*8+i)

const int iOpenNearMap = 3;

//����ͼ����
const int max_map_id = 8;

//Ŀǰ���ŵ�ͼ
const int iMaxOpenMap = 5;

//������ɫʱ��ͼƬ����
const int maxCharactorSpic = 16;
//ÿ����ѿɹ�����Ӣ����
const int FreeTimes = 5;

//��ʼ����ɫ�Դ�����
const int iDefaultZhenType = 1;

//ϵͳ��Ϣ�ĸ�ʽ
const std::string strSystemMsgFormat = "{\"cmd\":\"chat\",\"ctype\":5,\"s\":200,\"m\":\"$M\",\"type\":$T}";
//�����ͨ��Ϣ�ĸ�ʽ
const std::string strCharMsgFormat = "{\"cmd\":\"chat\",\"ctype\":8,\"s\":200,\"m\":\"$M\"}";

//������ɫʱ����ѡ����佫
const int create_choose_generals_id[6] = {3,1,5,4,2,6};

//������������
const int iMaxWeaponNums = 200;

//ʿ���޸� wlj 20120815
const int iAttackSuccessShiqi = 50;    //�����ɹ������Լ���ʿ��ֵ
const int iBeAttackedShiqi = 0;        //����������ʿ��ֵ

const int iStrongholdHistoryNums = 5;        //��������5����ʷս��

const int iArchiveFirstStrongholdSuccess = 1;    //�����һ������ɹ��ļ�¼

const int iSplsFirstYear = 200;        //��ʼ��� 200��

const int iNewbieGoGoGoSecs = 259200;        //���ֳ��ŵ���ʱ72Сʱ

const int iRechargeFirst = 604800;        //��һ�ڳ�ֵ������Ч��
const int iRechargeSecond = 604800;        //�ڶ��ڳ�ֵ������Ч��

const int iNewbieGoGoGoCheaper = 20;        //���ֳ����ڼ��ڣ������Ϣ�Ż�20%

/*********************** ������� ***************************/

//����Ʒ�������ļ۸�ϵ��
//const int baseWeaponPriceFactor[5] = {1,2,3,5,10};

//����ͼ�����������̳�ʼˢ�����Ҽ۸�
//const int baseWeaponRefreshSilver[max_map_id] = {50,400,1200,2800,5000};

//����mapid���������ͣ�Ʒ�����spic
//#define WEAPON_SPIC(mapid,type,quality) (25*(mapid-1)+5*(type-1)+quality)

//ʹ�ý��ˢ�±����̵�����
//const int iRefreshWeaponGoldCost = 2;

/*********************** ��Ϣ��� ***************************/

//���Ϣ��ȴ��ʱ��1Сʱ  3600��
const int iEventRestCooltime = 3600;

//ÿ����Ϣ���ӵľ���
const int iRestLing = 10;

//���Ϣ�ָ�����������仯,�ܹ�������Ϣ5��
const int iFreeRestLing[5] = {4,4,6,6,10};
//���ϢCD������仯,�ܹ�������Ϣ5��
const int iFreeRestCD[5] = {10,15,20,20,20};

//VIP��Ϣ����                0  1  2  3  4  5  6  7  8  9  10 11 12
const int iVIPRestTimes[13] = {5, 10, 15, 20, 25, 30, 35, 40, 50, 60, 70, 80, 100};

//ϵͳ�ָ���������
const int iSystemRecoverMaxLing = 45*4;

//��������Ľ������
const int iWorldChatGoldCost = 0;

//ˢ���̵�������
const int iRefreshShopGoldCost = 30;
#ifdef JP_SERVER
const int iRefreshShopGoldVIP[] = {0, 1, 2, 3, 5, 8, 10, 15, 20, 25, 30, 35, 50};
#else
const int iRefreshShopGoldVIP[] = {0, 0, 0, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3};
#endif

/*********************** ��������� ***************************/

//���ұ���=��ͼ����ϵ��*�����ȼ�
//��ͼ���    ��ͼ����ϵ��
//��1ͼ    220
//��2ͼ    340
//��3ͼ    420
//��4ͼ    500
//��5ͼ    550
//��6ͼ    600
const int iSilverBoxCounts[max_map_id] = {220,340,420,500,550,600,600,600};
//��ұ������
//70% +5
//20% +10
//10% +20
const int iGoldBoxChances[3] = {70, 90, 100};
//��ʯ�������
//50% +1
//35% +2
//15% +4
const int iOreBoxChances[3] = {50, 85, 100};

/* 2012-1-4 qj������������Ƚ���
1��ÿ�궬������23:00��ÿ����ͼ�ľ��������ݼ�ʱ����������Ƚ�����
2�������ϵͳ�Զ�����������Ÿ����
����    ��Ƚ���
��1��    20�����1000����
��2��    15�����800����
��3��    12�����700����
4-6��    10�����600����
5-10��    9�����500����
11-20��    8�����400����
21-30��    7�����300����
31-50��    6�����250����
51-100��5�����200����
101-200��3�����150����
201-500��2�����100����

*/
const int iRaceRankReward[][3] =
{
    {1, 20, 1000},    //����1
    {2, 15, 800},    //����2
    {3, 12, 700},    //����3
    {6, 10, 600},    //����4-6
    {10, 9, 500},    //����7-10
    {20, 8, 400},    //����11-20
    {30, 7, 300},    //����21-30
    {50, 6, 250},    //����31-50
    {100, 5, 200},    //����50-100
    {200, 3, 150},    //����101-200
    {500, 2, 100}    //����201-500    
};

const int iRaceWinPrestige = 10;
const int iRaceLosePrestige = 5;

#ifdef TEST_SERVER
const int iRaceCD = 10;    //��ս��ȴʱ�� 30��
#else
const int iRaceCD = 600;    //��ս��ȴʱ�� 600��
#endif

//const int iRaceFreeTimes = 15;    //���15����ս
const int iRaceScoreTimes = 20;    //ǰ20����ս�л���

//ÿ���ؿ�ս�����������
const int TIME_MAX_A_COMBAT = 30;

//ǿ������ȴʱ��29��
const int TIME_MAX_ENHANCE = 29 * 60;
//ǿ������ȴVIP�ȼ�
const int iEnhanceNoCDVip = 1;


/*********************** ������� ***************************/

const int max_skill_level = 60;        //������ߵȼ�
const int total_skill_type = 21;        //��������

const int skill_research_mins = 1;    //�����о��ӵ�ʱ��
const int skill_teacher_nums = 3;        //�����о�������
const int skill_queue_max = 5;            //�����о������������
const int skill_queue_data[skill_queue_max][2] =
{
    {0,0},        //��һ��������Ѹ�
    {0, 50},    //�ڶ�������50���
    {1, 100},    //��3������vip1 100���
    {2, 300},    //��4������vip2 300���
    {6, 500}    //��5������vip6 500���
};

//��������ٶȼӳɰٷֱ�
const int skill_vip_queue_more_speed = 30;

const int skill_queue_levelup_gold = 400;//400��ҿ�����������


//����ˢ�¼���ѵ��
const int skill_teacher_update_silver[max_map_id] =
{
    1000, 4000, 12000,28000,50000,75000,75000,75000
};

//���ˢ�¼���ѵ��
const int iSkill_teacher_update_gold = 2;


/*********************** �佫ѵ����� ***************************/

const int general_book_nums = 1;        //�佫��������
const int general_queue_max = 5;            //�佫�о������������
const int general_vip_queue_more_speed = 3600;//������м�����ȴʱ��
const int general_vip_queue_more_level = 5;//�����������ѵ��Ч��
const int general_queue_levelup_gold = 300;//400��ҿ�����������
#ifdef JP_SERVER
const int general_queue_data[general_queue_max][2] =
{
    {0, 0},        //��һ��������Ѹ�
    {0, 50},    //��2������vip1 50���
    {0, 100},    //��3������vip3 100���
    {0, 200},    //��4������vip4 200���
    {0, 400}    //��5������vip5 400���
};
#else
const int general_queue_data[general_queue_max][2] =
{
    {0, 0},        //��һ��������Ѹ�
    {0, 10},    //��2������vip1 10���
    {0, 100},    //��3������vip3 100���
    {0, 200},    //��4������vip4 200���
    {0, 400}    //��5������vip5 400���
};
#endif

//��ͬ��ͼˢ��ѵ�������
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

/*********************** ұ����� ***************************/

const int iSmeletSpeedVip = 8;//��Ҽ���ұ��vip
const double smelt_ore_fac[6] = {0.0,0.0,1.0,1.5,2.0,2.5};        //ұ�����Ŀ�ʯϵ��(Ʒ�ʾ���)
const int smelt_queue_max = 5;            //ұ�������������
const int smelt_task_max = 3;            //ұ�������������

const int smelt_update_silver[max_map_id] =
{
    0, 0, 20000, 50000, 80000, 100000, 100000, 100000
};

/*********************** ״̬��� ***************************/

//���״̬����
const int max_state_nums = 3;

const int max_refresh_state_types = 24;    //ǰ24��״̬���Ÿ��û�ˢ��

const int iRefreshStateSilver[max_map_id] = {-400,-400,-1200,-2800,-5000, -9000, -9000, -9000};    //ÿ��ͼˢ״̬��������

const int iRefreshStateGold = -1;        //ˢ״̬�������
const int iRefreshStateWithGold_VIP_level = 2;    //ʹ�ý��ˢ״̬��Ҫ��vip�ȼ�


/*********************** ������� ***************************/

const int map_normal_silver_field[max_map_id] = {3,6,8,12,18,24,24,24};//24
const int map_elite_silver_field[max_map_id] = {4,8,10,18,24,30,30,30};//30

/* vip ��Ȩ */

//ʹ�ý��ˢ�±����̵�vipҪ��
const int iRefreshWeaponWithGold_VIP_level = 2;

//���ˢ�¼���ѵ��,��Ҫ��vip�ȼ�
const int iSkill_teacher_update_gold_vip_level = 3;

const int skill_queue_levelup_vip = 5;    //vip5������������ѵ������

const int skill_queue_speedup_vip = 0;    //�����о���Ҫ��vip�ȼ�

const int general_queue_levelup_vip = 4;//vip4���������佫ѵ������
const int iTrainSpeedVip = 0;    //����Ӣ��ѵ������vip�ȼ�

//��Ӣ�ؿ�������1( vip 7 )
const int iMoreStrongholdAttackVIP_level = 7;

//���������vipҪ��
const int iWorldChatVipLevel = 0;

//��������Ҫ�������ȼ�
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


//���������ʾ���ֵ
const int iMaxGoldCostConfirm = 100;


//���ҹ���ɹ���
const int iInspireSilver[] = {    30,20,20,10,10};

//���ҹ������� 600*�ȼ�
const int iInspireSilverCost = 600;

//��ҹ�������
const int iInspireGoldCost[] = {10,15,20,25,30};

//��ҹ���vip�ȼ�
const int iInspireGoldVIPLevel = 3;

//bossս��CD
const int iBossCombatCD = 30;
const int iBossMinCombatCD = 20;
//bossս����������
const int iBossEndCDTimes = 5;

//bossս���������������
const int iBossCombatExpLimit = 1000;

//bossս������3����׼��ʱ��
const int iBossCombatDelay = 180;

#ifdef CAMP_RACE_TWO_QUEUE    //�Ƿ񻮷�Ϊ��������
//��Ӫս�ȼ�����
const int iCampRaceLevel_low = 50;
#else
//��Ӫս�ȼ�����
const int iCampRaceLevel_low = 9999;
#endif

//��Ӫս�����������
const int iCampRaceMaxPrestige = 2000;

//��Ӫս�ֿ���ȴʱ��(��)
const int iCampRaceNomatchCD = 20;
//��Ӫսս�������ȴʱ��(��)
const int iCampRaceFightCD = 30;
//��Ӫսƥ��ս������(��)
const int iCampRaceMatchPeriod = 20;

//���ż���(��ң���ѫ��������VIP�ȼ�)
const int iCorpsJisi[][4] =
{
    {0, 200, 10, 0},
    {30, 0, 100, 0},
    {90, 0, 1000, 1}
};

//ÿ�վ���̽������
const int iCorpsExploreTimesOneday[] = {0,10,15,20,25,30,30,30,30,30,30};

//���ž���
const int iDonateGoldVIP = 2;

//���������ŵȼ�Ҫ��
const int iCorpsLevelForParty = 2;

//���������������
const int iCorpsYanhuiMax = 7;

//����þ���
const int iCorpsYanhuiLing = 7*4;

//������������vip
const int iCorpsCreateVip = 1;

//������������
const int iFarmTaskNum = 4;
const int iFarmSpeedVip = 4;
//�����ӳٴ���VIP����
const int iDelayFarmVIP[] = {0,0,0,1,2,3,3,6,12,12,12,12,12};
//���ｽˮ����
const int iFarmWater = 3;
const int iFarmFriendWater = 30;

//��Ӣɨ��������Ӣ����ʱ��(second)
const int iSweepEliteTime = 120;
const int iSweepEliteFinishVip = 2;


/*********************** ϴ����� ***************************/
//const int iWashBuyVip = 7;
//const int iWashSilverFac = 80000;
const int iWashSilver = 50000;
const int iWashGold = 10;
const int iWashUltra = 100;
const int iWashFullValVIP = 6;//ϴ�����޷���VIP�޶�
//ϴ�����Ͷ�ӦvipҪ�����ģ��ӳ�����(���ڴ����Աؼ�)��ϴ�豣��
const int iWashConfig[][4] =
{
    {0, 2000, 0, 1},//��ͨ
    {0, 2, 40, 20},//��ͭ
    {3, 10, 80, 40},//�׽�
    {5, 20, 160, 80},//��ʯ
#ifdef JP_SERVER
    {6, 50, 400, 200}//����
#else
    {6, 100, 400, 200}//����
#endif
};

/*const int iWashTypeMax = 11;//ϴ����ʵ����ܹ�12��
const int iWashGroup[iWashTypeMax] = {0,2,3,5,7,9,11,13,16,21,31};//�佫ϴ��ӵ����������
const int iWashPer[3][iWashTypeMax] =
{
    {100, 100, 100, 100, 100, 100, 100, 100, 80, 60, 50},//treasure
    {80, 50, 25, 15, 5, 2, 0, 0, 0, 0, 0},//silver
    {100, 80, 60, 50, 40, 20, 15, 5, 2, 1, 0}//gold
};*/

/*********************** ������� ***************************/
const int iRebornVip = 4;
const int iRebornOpenLevel = 50;
const int iRebornGold = 30;
const int iRebornFreeVip = 6;
const int iRebornFreeTimes = 1;
const int iOnekeyRebronVip = 8;    // V8һ������
const int iRebornPointMin = 30;//���������Ҫ30��
const int iRebornPoint[] = {15, 25, 35, 40, 50, 65, 75, 80, 90, 100, 100};//�������ĵ���
const double iChengZhangMax[] = {8, 12, 18, 26, 36, 45};//�ɳ�����Ʒ�ʾ�������

/*********************** ��������� ***************************/
//ˢ�½��׸���
const int iGuardRefreshPer[] = {70, 35, 15, 8};

#ifdef JP_SERVER
//ˢ�»��͸�
const int iGuardRefreshGold = 5;
//�ٻ�������
const int iGuardCallGold = 100;
//�뻤��CD
const int iGuardFinishGold = 20;//ÿ10����20��
//���ȡCD
const int iGuardSpeedGold = 20;
#else
//ˢ�»��͸�
const int iGuardRefreshGold = 10;
//�ٻ�������
const int iGuardCallGold = 200;
//�뻤��CD
const int iGuardFinishGold = 30;//ÿ10����30��
//���ȡCD
const int iGuardSpeedGold = 10;
#endif

const int iGuardFinishVip = 0;
const int iGuardCallVip = 5;
//��ȡCD
const int iGuardCD = 600;
//����
const int iGuardInspireGold = 20;
const int iGuardInspireVIP = 4;

/*********************** ͨ����� ***************************/
const int iTradeTypeNum = 3;//ͨ����������
const int iTradeTypeLevel[iTradeTypeNum][2] =//ͨ�̸�����ȼ���Χ
{
    {16, 49},
    {50, 64},
    {65, 1000}
};
//ͨ�̽�������ϵ��
const int iTradeSilver[]={12000,30000,50000};
const int iTradeEveryday = 4;
const int iTradeTakeSeatLing = 0;//ͨ����ϯ�������
const int iTradeRobSeatLing = 0;//ͨ�̶�ϯ�������
const int iTradeProtectGold = 10;//ͨ�̱���10���
const int iTradeTime = 600;//ͨ�̸��˽���ʱ������(second)
const int iTradeSpeedTime = 3600;//ͨ�̼���ʱ��(second)
const int iTradeTotalTime[] = {1800,1800,1800};//ͨ��ʱ��

/***********************������� ***************************/
#ifdef JP_SERVER
const int iLevyFreeTime = 1;//ÿ��������մ���
#else
const int iLevyFreeTime = 0;//ÿ��������մ���
#endif
const int iLevyMoreTimeVIP = 6;//��������10����ҪVIP
const int iLevyMaxTimeVIP = 10;//��������100����ҪVIP
const int iLevyTimes[]={10,20,30,40,50,70,85,100,150,200,250,300,500};//��vip���մ���

/***********************ս����� ***************************/
const int iHorseTurnsMax = 5;//�������ת�����
const int iHorseStarsMax = 10;//��������Ǽ�
const int iHorseTrainSilver = 15000;//������������
const int iHorseTrainGold = 100;//�����������
const int iHorseTrainSilverAdd = 40;//�������Ǿ���
const int iHorseTrainGoldAdd = 48;//�������Ǿ���
//const int iHorseTrainTime = 20;//ÿ������ս�����
//const int iHorseGoldTrainTime = 3;//ÿ��������ս�����

const int iHorseMoreTrainVip = 6;//�߼���������VIP
const int iHorseQuality[] = {0,1,2,3,4,5};//��ת��ս����ɫ
const int iHorseActionTime = 2592000;//ս������ʱ��(30days)
const int iHorseFruitTime = 259200;//���ӳ���ʱ��(3days)

/***********************�Ҷ���� ***************************/
const int iServantRandomCatchTime_Gold_First = 2;//��һ�ι������
const int iServantRandomCatchTime_Gold_Add = 2;//�����������
const int iServantRandomCatchTime = 1;//ÿ��������ץ������
const int iServantCatchTime = 10;//ÿ�����ץ������
const int iServantCatchTime_First = 5;//��һ�ι���ץ������
const int iServantCatchTime_Add = 5;//����ץ��������������
const int iServantRescueTime = 3;//ÿ����ѽ�ȴ���
const int iServantInteractTime = 6;//ÿ����ѻ�������
const int iServantCD = 1200;//������ȴʱ��
const int iServantResistTime = 5;//ÿ����ѷ�������
const int iServantResistTime_Gold = 10;//���򷴿�����
const int iServantSOSTime = 2;//ÿ�������ȴ���
const int iServantBeSOSTime_F = 1;//ÿ�챻������ȴ���
const int iServantBeSOSTime_C = 1;//ÿ�챻������ȴ���
const int iServantExploitGold = 2;//����������
const int iServantEscapeVIP = 3;//��������VIP
const int iServantEscapeGold = 50;//����������
const int iServantTime = 600;//�Ҷ��Ӳ������ʱ��
const int iServantMaxFac = 220;//�Ҷ���������ϵ��

/***********************���˸������ ***************************/

//����npc�������6��
const int iMaxGroupCopyArmys = 6;
//����������3��
const int iMaxGroupCopyMembers = 3;
//�����������2�˿�ս
const int iMinGroupCopyMembers = 2;

//���˸������ŵȼ�
const int iGroupCopyOpenLevel = 20;

/*************������Ӣ�ؿ���ѫ���� *****************/
const int iStageGongxun[]={200,500,800,1100,1450,1800,2150,2500,2900,3350,3800,4300,4850,5450,6050,6550,7050,7550};

/******************** �������ܵĿ��ŵȼ� ********************/

/**********************���� ********************************/
//̽������ ���ܵ�5���ؿ���6����
const int iExploreOpenStronghold = (5-1)*8 + 6;
//�̵꿪��
const int iShopOpenStronghold = 38;
//���￪�� ���ܵ�8���ؿ���6����
const int iFarmOpenStronghold[] = {50, 92, 118, 138, 158, 178};//(8-1)*8 + 6;
//ͨ�̿���
const int iTradeOpenStronghold = 62;

/**********************���� *******************************/
//���� δ֪����??????????????
const int iSkillOpenStronghold = 9999;
//���󿪷�
const int iZhenOpenStronghold = 10;
//ս���� ���ܵ�4���ؿ���5����
const int iHorseOpenStronghold = (4-1)*8+5;
//�Ƽ����� �ط����� ���ܵ�1���ؿ���8����
const int iWeaponOpenStronghold = 8;

/**********************�佫 *******************************/
//ѵ������ ���ܵ�3���ؿ���4����
const int iTrainOpenStronghold = (3-1)*8+4;
//װ������ ���ܵ�1���ؿ���4����
const int iEquipOpenStronghold = 3;
//ǿ������ ���ܵ�1���ؿ���7����
const int iEquipEnhanceOpenStronghold = 7;
//ϴ�迪�� ���ܵ�3���ؿ���6����
const int iWashOpenStronghold = (3-1)*8 + 6;
//�������Źؿ�
const int iRebornOpenStronghold = STRONGHOLD_ID(3,2,4);
//��Ƕ����
const int iXiangqianOpenStronghold = 54;

//���ſ��� ���ܵ�5���ؿ���4����
const int iCorpsOpenStronghold = 36;

/******************�Ϸ���ť���� **********************/
//��Ϸ���ֿ��� ���ܵ�5���ؿ���2����
const int iHelperOpenStronghold = (5-1)*8+2;
//���������� ���ܵ�4���ؿ���6����
const int iRaceOpenStronghold = (4-1)*8+6;
//���޿��� ���ܵ�6���ؿ���6����
const int iBossOpenStronghold = (6-1)*8 + 6;
//��Ӫս���� ���ܵ�6���ؿ���4����
const int iCampraceOpenStronghold = (6-1)*8 + 4;
//��Ӣս�ۿ���
const int iEliteOpenStronghold = (8-1)*8 + 2;

//��������
//�Ҷ���̨ʵ�ʿ���2ͼ3�ؿ�8����
const int iServantRealOpenStronghold = 48;
//�Ҷ����Ա���ҿ���3ͼ1�ؿ�8����
const int iServantOpenStronghold = 56;
//���տ��� ���ܵ�2���ؿ���6����
const int iLevyOpenStronghold = (2-1)*8 + 6;
//���Ϳ��� 4ͼ1�ؿ�6����
const int iGuardOpenStronghold = 78;
//��ְ���� ?????????
const int iOfficalOpenStronghold = 0;
//��������
const int iBuyLingOpenStronghold = 42;
//ɨ������
const int iSweepOpenStronghold = (2-1)*3*8 + (3-1)*8 + 8;

//����+1=���ŵ��佫���ޣ�ֵ=��Ҫ�Ĺؿ�
const int iGeneralLimitStronghold[] = {0,0,0,0,28,70,80,90,100,9999};

//Ĭ����������
const int iDefaultUpGenerals = 3;

//��һ���������������Ĺؿ�
const int iFirstUpGeneralStronghold = 78;
//�ڶ����������������Ĺؿ�
const int iSecondUpGeneralStronghold = 122;
//�����п���1-3-5
const int iRankEventOpenStronghold = 0;
//Ǯׯ����2-1-3
const int iBankOpenStronghold = 0;

//�����󿪷Źؿ�334
const int iMazeOpenStronghold = (3-1)*3*8 + (3-1)*8 + 4;

//����Ŀ��ؿ�314
const int iSevenOpenStronghold = (3-1)*3*8 + (1-1)*8 + 4;

//����¼���Źؿ�
const int iJxlOpenStronghold = 10;

//���꿪���ؿ�
const int iGeneralSoulOpenStronghold = 10;

/**************************************************************/

//������п��ŵȼ�
const int iFarmOpenLevel[] = {26,47,60,70,80,90};

//̽��ˢ��VIP
const int iExploreRefreshVIP = 1;
const int iExploreBaseOpenLevel = 41;
const int iExploreTianOpenLevel = 50;
const int iExploreRefreshTimes[] = {0,1,3,8,8,8};
const int iExploreRefreshUnlimitVIP = 6;

//������߽�ɫ�ȼ�
const int iMaxCharLevel = 120;

//������¼����
const int iContinueLoginStronghold = 38;
//���߽���
const int iOnlineGiftStronghold = 4;

//�׳���� 2-1-2
const int iFirstRechargeStronghold = 0;

//�ղ�������ֹؿ�2-1-3
const int iCollectLibaoStronghold = STRONGHOLD_ID(2,1,3);

//ˢ��ʱ��
const int iRefreshHour = 5;
const int iRefreshMin = 0;

//GM����
const int iGM_Level_Max = 2;
const int iGM_Cancel = 7;
const int iGM_Level_Gold[] = {1000,2000};

//VIP����
const int iTmpVip_level = 4;
#ifdef TEST_SERVER
const int iTmpVip_time = 60;
#else
const int iTmpVip_time = 7200;
#endif
const int iTmpVip_OpenStronghold = (6-1)*8+6;

/******************** ���컯�������� ********************/
#ifdef TW_SERVER
const int iTradeSpeedGold = 10;//ͨ�̼���������ÿСʱ10���
const int iRestGold_First = 15;//�����Ϣ��һ������
const int iRestGold_Add = 5;//�����Ϣ���ѵ���
const int stronghold_times_cost = 2;//����Ӣ������������

const int iExploreRefreshGold_First = 5;//��һ��ˢ��
const int iExploreRefreshGold_Add = 5;//֮��ˢ�µ���
const int iExploreRefreshGold_Max = 30;//ˢ����������

const int smelt_queue_data[smelt_queue_max][2] =
{
    {0,0},        //��һ��������Ѹ�
    {0, 250},    //��2������ 250���
    {4, 500},    //��3������vip4 500���
    {5, 750},    //��4������vip5 750���
    {7, 1000}    //��5������vip7 1000���
};
const int iSmeletSpeedGold = 2;//ұ������ÿ10���ӵ�����
const int iBookRefreshGold = 2;//ˢ��ѵ����������
const int iTrainSpeedGold = 1;//����Ӣ��ѵ��ÿ10��������
const int iWashBuyFac = 20;//��ҹ���ϴ��Ĺ�ʽϵ��
const int iRegetGold_First = 5;//��һ���ٴλ������
const int iRegetGold_Add = 5;//�ٴλ�����ѵ���
const int skill_queue_speedup_mins = 5;    //ÿ����ܼ��ٵķ���
//VIP��Ҫ�ĳ�ֵ��
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
const int iTradeSpeedGold = 30;//ͨ�̼���������ÿСʱ30���
const int iRestGold_First = 30;//�����Ϣ��һ������
const int iRestGold_Add = 10;//�����Ϣ���ѵ���
const int stronghold_times_cost = 5;//����Ӣ������������

const int iExploreRefreshGold_First = 2;//��һ��ˢ��
const int iExploreRefreshGold_Add = 2;//֮��ˢ�µ���
const int iExploreRefreshGold_Max = 20;//ˢ����������

const int smelt_queue_data[smelt_queue_max][2] =
{
    {0,0},        //��һ��������Ѹ�
    {0, 100},    //��2������ 100���
    {4, 200},    //��3������vip4 200���
    {5, 500},    //��4������vip5 500���
    {7, 1000}    //��5������vip7 1000���
};
const int iSmeletSpeedGold = 5;//ұ������ÿ10���ӵ�����
const int iBookRefreshGold = 2;//ˢ��ѵ������ÿ�ε������
const int iTrainSpeedGold = 2;//����Ӣ��ѵ��ÿ10��������
const int iWashBuyFac = 50;//��ҹ���ϴ��Ĺ�ʽϵ��
const int iRegetGold_First = 10;//��һ���ٴλ������
const int iRegetGold_Add = 5;//�ٴλ�����ѵ���
const int skill_queue_speedup_mins = 5;    //ÿ����ܼ��ٵķ���
#ifdef JP_SERVER
//VIP��Ҫ�ĳ�ֵ��
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
//VIP��Ҫ�ĳ�ֵ��
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


