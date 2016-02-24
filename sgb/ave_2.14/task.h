#pragma once

#include "base_item.h"
#include <list>
#include <vector>

#include <string>
#include <map>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "json_spirit.h"
#include "spls_const.h"

using namespace json_spirit;

struct StrongholdData;

class CharData;

//任务类型,1、关卡 2、主将等级 3、获得武将 4、武将等级 5、进入场景 6、进入地图 7、技能等级 8、阵型等级
//9、购买兵器 10、刷新兵器 11、布阵操作 12、屯田操作 13、收藏游戏 
//14、探索操作 15、装备等级 16、技能研究 17、加入阵营 18、竞技 19、状态刷新 20、银币数量 21、武器等级
enum task_type
{
    task_empty = 0,        //空白任务
    task_attack_stronghold = 1,//击败指定关卡
    task_char_level,        //2主将达到指定等级
    task_get_general,        //3获得指定武将
    task_general_level,    //4武将升级到一定等级
    task_enter_stage,        //5进入指定场景
    task_enter_map,        //6进入指定地图
    task_skill_level,        //7技能升级到一定等级
    task_zhen_level,        //8阵型升级到指定等级
    task_buy_weapon,        //9 购买指定兵器
    task_refresh_weapon,    //10刷新铁匠铺
    task_send_general,    //11排兵布阵，让指定武将上阵
    task_first_farm,        //12首次进行屯田操作
    task_add_to_favorites,//13收藏游戏
    task_do_explore,        //14探索两次
    task_equipment_level,//15装备等级
    task_do_research,        //16研究技能
    task_choose_camp,        //17加入阵营
    task_do_race,            //18竞技战斗
    task_refresh_state,    //19刷新状态
    task_silver_nums,        //20银币数量
    task_attack_equipment_level,    //21武器等级
    task_get_treasure,    //22获得道具
    task_group_general_level,    //23多个武将达到指定等级
    task_first_smelt,        //24首次冶炼操作
    task_weapon_level,    //25兵器等级
    
/****************** 支线任务 ***************************/
    
    task_rob_stronghold,//26掠夺关卡
    task_get_gem,        //27获得道具
    task_elite_combat,  //28精英战役通关
    task_equipment_make,//29制造装备
    task_buy_bag,        //30购买一个背包位置
    task_gather_gem,    //31急满材料
    task_join_corps,    //32加入军团
    task_corps_jisi,    //33军团祭天
    task_corps_ymsj,    //34军团辕门射戟
    task_corps_explore, //35军团探索
    task_daily_score,   //36每日活跃度

    task_open_libao,    //37打开礼包

    task_levy,          //征收 38 次数
    task_add_friends,   //盟友 39 个数
    task_train,         //训练 40 次数
    task_normal_wash,   //普通洗髓 41 次数
    task_2gold_wash,    //青铜洗髓 42 次数
    task_wash_star,     //洗髓星级 43 
    task_horse_train,   //战马培养 44 次数
    task_arena_win,     //竞技场胜利 次数 45
    task_arena_liansheng,//竞技场连胜 46
    task_farm_harvest,  //屯田收获 47 次数
    task_farm_water,    //屯田浇灌 次数 48
    task_farm_yechan,   //屯田野产 次数 49
    task_baoshi_exchange,   //宝石兑换 50
    task_baoshi_combine,    //宝石合成 51
    task_baoshi_convert,    //宝石转换 52
    task_baoshi_combine_level,//宝石合成 53

    task_arrest_servant,   //抓捕壮丁  次数 54
    task_rescue_servant,   //解救壮丁 次数 55
    task_reborn,            //重生 次数 56
    task_reborn_star,      //重生星级 57

    task_trade_star,    //贸易星级 58
    task_trade_wjbs,    //贸易使用无商不奸 59
    task_upgrade_soul,  //升级演兵 60
    task_center_soul_level, //演兵魂眼等级 61

    task_maze_score,    //八卦阵积分 62

    task_general_inherit,   //武将传承 次数 63
    task_shop_buy_mat,      //商店购买材料 64
    task_shop_buy_baoshi,   //商店购买宝石 65
};

enum guide_type_enum
{
    guide_type_login,
    guide_type_gettask,
    guide_type_taskdone,
    guide_type_char_level,
    guide_type_stronghold,
    guide_type_frist_farm,
    guide_type_enter_map,
    guide_type_view_map,
    guide_type_no_ling,
    guide_type_choose_camp,
    guide_type_get_stage_reward,
    guide_type_no_supply
};

enum guide_id_enum
{
    guide_id_enhance = 101,    //强化引导 攻击第一个区域、第一个场景第七个关卡
    guide_id_upgrade_weapon = 102,    //升级兵器引导 第一个区域、第一个场景领取通关奖励后
    guide_id_recruit = 103,    //招募引导 攻击第一个区域、第二个场景第二个关卡
    guide_id_equipment = 104,    //装备引导 攻击第一个区域、第二个场景第四个关卡
    guide_id_levy = 105,        //征收引导 攻击第一个区域、第二个场景第六个关卡
    guide_id_next_stage = 106,//第一个区域、第二个场景领取通关奖励后
    guide_id_newbie_present = 107,//引导领取新手礼包 攻击第一个区域、第三个场景第二个关卡
    guide_id_train = 108,    //引导训练 攻击第一个区域、第三个场景第四个关卡
    guide_id_wash = 109,    //洗髓引导 攻击第一个区域、第三个场景第六个关卡
    guide_id_next_map = 110,//引导进入下一区域  第一个区域、第三个场景领取通关奖励后
    guide_id_race = 111,    //引导竞技场 攻击第二个区域、第一个场景第五个怪后，竞技场中不止玩家一个人
    guide_id_up_offical = 112,//引导升官 请求主将信息时，第一次可以升官
    guide_id_get_salary = 113,//引导领俸禄 请求主将信息时，第一次可以领俸禄
    guide_id_horse = 114,    //引导战马培养 攻击第二个区域、第一个场景第六个怪后
    guide_id_get_daily = 115,//引导领取日常活动奖励 请求游戏助手信息，第一次可领取日常活动奖励
    guide_id_shop = 116,    //引导购买物品 攻击第二个区域、第二个场景第六个怪后
    guide_id_117 = 117,    //剧情提示 攻击第二个区域、第三个场景第六个怪后
    guide_id_118 = 118,    //剧情提示 攻击第二个区域、第三个场景第七个怪后
    guide_id_sweep = 119,    //扫荡引导 第一次出现军粮不足时
    guide_id_120 = 120,    //剧情提示 攻击第三个区域、第二个场景第二个怪后
    guide_id_121 = 121,    //剧情提示 攻击第三个区域、第二个场景第六个怪后

    guide_id_trade = 122,    //贸易开启
    guide_id_reborn = 123,//重生开启
    guide_id_xiangqian = 124,    //镶嵌开启
    guide_id_servant = 125,//壮丁开启
    guide_id_guard = 126,    //护送开启
};

enum task_id_enum
{
    task_id_dongcao = 3,    //董超任务
    task_id_xueba = 5,        //薛霸任务
    task_id_farm = 11        //屯田任务
};

struct baseTask
{
    baseTask();
    ~baseTask();
    int task_type;        //0主线任务  1支线任务
    int need_task;        //支线任务有效 支线任务前置的主线任务

    int id;        //任务id
    int type;            //任务列别
    int need[4];
    int done_level;        //主将达到该等级，任务自动完成

    boost::shared_ptr<StrongholdData> target_stronghold;    //目标关卡
    int mapid;
    int stageid;
    int sweep;

    std::string title;    //任务标题
    std::string memo;    //任务描述

    std::list<Item> reward;        //奖励

    json_spirit::Object detail_obj;    //任务整个obj
    json_spirit::Object simple_obj;    //简单信息的obj

    std::list<boost::shared_ptr<const struct baseTask> > m_trunk_tasks;
    void loadRewards();
};

struct charTask
{
    charTask(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    boost::shared_ptr<const baseTask> _task;
    int tid;
    int cur;
    int need;
    bool done;
    void Save();
};

struct charTrunkTasks
{
    charTrunkTasks(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    std::map<int, boost::shared_ptr<charTask> > m_trunk_tasks;

    //接受任务
    void acceptTask(boost::shared_ptr<const baseTask> t);
    //完成任务
    int taskDone(int id, json_spirit::Object& robj);
    //任务列表
    int getList(json_spirit::Array& rlist);
    //更新任务
    int updateTask(int type, int n1 = 0, int n2 = 0);
    bool getFinishState();
};

class taskMgr
{
public:
    taskMgr();
    boost::shared_ptr<const baseTask> getTask(int tid);    //根据任务id获得任务
    boost::shared_ptr<const baseTask> getNext(boost::shared_ptr<const baseTask> task);    //下一个任务

    boost::shared_ptr<const baseTask> getTrunkTask(int tid); //根据任务id获得任务

    int queryCurTask(boost::shared_ptr<CharData> cdata);    //获得角色当前任务
    int newChar(boost::shared_ptr<CharData> cdata);
    int taskDone(boost::shared_ptr<CharData> cdata, int id, json_spirit::Object& robj);        //角色完成任务
    //任务详情
    int getTaskInfo(CharData& cData, int tid, json_spirit::Object& robj);
    //任务列表
    int getTaskList(CharData& cData, int page, int pageNums, json_spirit::Object& robj);
    //删除角色
    int deleteChar(int cid);
    //角色通关尝试领取相关支线
    int acceptTrunkTask(CharData& cData, int strongholdid);
    //角色完成支线任务是领取相关任务
    int acceptTrunkTask2(CharData& cData, int tid);
    
    static taskMgr* getInstance();
    
private:
    static taskMgr* m_handle;
    std::vector<boost::shared_ptr<const baseTask> > m_total_tasks;        //所有任务
    std::vector<boost::shared_ptr<const baseTask> > m_trunk_tasks;        //所有支线任务
    std::map<int, std::vector<boost::shared_ptr<const baseTask> > > m_get_trunk_tasks;        //支线任务对应的接取关卡
    std::map<int, std::vector<boost::shared_ptr<const baseTask> > > m_get_trunk_tasks2;       //支线任务完成开放的支线任务
    std::map<int, boost::shared_ptr<const baseTask> > m_char_tasks;    //角色任务记录
};

