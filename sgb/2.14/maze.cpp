
#include "maze.h"
#include <algorithm>    // std::random_shuffle
#include "singleton.h"
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include "spls_errcode.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "statistics.h"

#include "combat.h"
#include "igeneral.h"
#include "loot.h"
#include "SaveDb.h"
#include "daily_task.h"
#include "first_seven_goals.h"
#include "char_general_soul.hpp"

Database& GetDb();
int giveLoots(CharData* cdata, Item& getItem, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);
int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

void ItemToObj(Item* sitem, boost::shared_ptr<json_spirit::Object>& sgetobj);

extern Combat* createMazeCombat(int cid, int maze_army_id, int extra_data, int& ret, bool);
extern void InsertCombat(Combat* pcombat);
extern void InsertSaveCombat(Combat* pcombat);
void InsertSaveDb(const std::string& sql);
void InsertSaveDb(const saveDbJob& job);

extern std::string strMazeNotify1;
extern std::string strMazeNotify2;

//八卦阵实际收益
void mazeRealReward(int& get);

//迷宫商品折扣 - 7折
//const int iMazeShopDiscount = 70;

//猜数字最多次数
const int iMazeGuessNumberTimes = 15;

//初始移动次数
const int iMazeOriginalMoveSteps = 45;

//迷宫超时 2小时 (秒数)
const int iMazeTimeout = 2 * 3600;

static const int map_x_y[iMazeSize][2] =
{
    {1,5},
    {2,4},{2,5},{2,6},
    {3,1},{3,2},{3,3},{3,4},{3,5},{3,6},{3,7},{3,8},{3,9},
    {4,1},{4,2},{4,3},{4,4},{4,5},{4,6},{4,7},{4,8},{4,9},
    {5,1},{5,2},{5,3},{5,4},{5,5},{5,6},{5,7},{5,8},{5,9},
    {6,1},{6,2},{6,3},{6,4},{6,5},{6,6},{6,7},{6,8},{6,9},
    {7,1},{7,2},{7,3},{7,4},{7,5},{7,6},{7,7},{7,8},{7,9},
    {8,1},{8,2},{8,3},{8,4},{8,5},{8,6},{8,7},{8,8},{8,9},
    {9,1},{9,2},{9,3},{9,4},{9,5},{9,6},{9,7},{9,8},{9,9},
    {10,1},{10,2},{10,3},{10,4},{10,5},{10,6},{10,7},{10,8},{10,9},
    {11,1},{11,2},{11,3},{11,4},{11,5},{11,6},{11,7},{11,8},{11,9},
    {12,4},{12,5},{12,6},
    {13,5}
};

int maze_distance(int id1, int id2)
{
    if (id1 >= 1 && id1 <= iMazeSize
        && id2 >= 1 && id2 <= iMazeSize)
    {
        return abs(map_x_y[id1-1][0] - map_x_y[id2-1][0]) + abs(map_x_y[id1-1][1] - map_x_y[id2-1][1]);
    }
    else
    {
        return 9999;
    }
}

enum maze_gold_cost_enum
{
    MAZE_GOLD_COST_FULL = 1,
    MAZE_GOLD_COST_SKIP = 2,
    MAZE_GOLD_COST_CHANGE = 3,
    MAZE_GOLD_COST_RESET = 4
};

const int iMazeGoldCostFull[4] = {1,30,50,100};

inline int mazeGoldCost(int type, int times)
{
    switch (type)
    {
        case MAZE_GOLD_COST_FULL:
            if (times >= 1 && times <= 4)
            {
                return iMazeGoldCostFull[times-1];
            }
            else
            {
                return 100;
            }

        case MAZE_GOLD_COST_SKIP:
            if (times <= 1)
            {
                return 10;
            }
            else
            {
                return 20;
            }
            
        case MAZE_GOLD_COST_CHANGE:
            if (times <= 1)
            {
                return 20;
            }
            else if (times <= 2)
            {
                return 30;
            }
            else
            {
                return 50;
            }
            
        case MAZE_GOLD_COST_RESET:
        default:
            if (times <= 1)
            {
                return 20;
            }
            else if (times <= 2)
            {
                return 30;
            }
            else
            {
                return 50;
            }
    }
}

inline int getPingjia(int score)
{
    if (score < 500)
    {
        return 1;
    }
    else if (score < 700)
    {
        return 2;
    }
    else if (score < 850)
    {
        return 3;
    }
    else
    {
        return 4;
    }
}

inline bool maze_inview(int id1, int id2, int r)
{
    if (id1 >= 1 && id1 <= iMazeSize
        && id2 >= 1 && id2 <= iMazeSize)
    {
        return abs(map_x_y[id1-1][0] - map_x_y[id2-1][0]) <= r && abs(map_x_y[id1-1][1] - map_x_y[id2-1][1]) <= r;
    }
    return false;
}

int iMazeScore[MAZE_EVENT_OUT] = {5,30,10,5,10,5,20,10,5,10,10,30,10,10,100};

static int getMazeScore(int type, int extra)
{
    switch (type)
    {
        //固定积分
        case MAZE_EVENT_CHANGE_POSTION:
        case MAZE_EVENT_REPEAT:            //3重复
        case MAZE_EVENT_WILD:            //4狂暴
        case MAZE_EVENT_WEAK:            //5虚弱
        case MAGE_EVENT_FULL:            //6恢复
        case MAZE_EVENT_BOMB:            //8炸弹
        case MAZE_EVENT_LUCKY:            //9遁甲
        case MAZE_EVENT_MISCHANCE:        //10横祸
        case MAZE_EVENT_SCORE:            //11积分
        case MAZE_EVENT_GEM:            //13宝物
        case MAZE_EVENT_OUT:
            return iMazeScore[type-1];

        case MAZE_EVENT_GUESS_NUMBER:
            switch (extra)
            {
                case 1:
                    return 30;
                case 2:
                    return 25;
                case 3:
                    return 20;
                case 4:
                    return 10;
                default:
                    return 0;
            }
            break;            
        case MAZE_EVENT_DISCOUNT:        //7甩卖
            if (extra)
            {
                return 20;
            }
            else
            {
                return 5;
            }
            break;
        case MAZE_EVENT_MORA:            //12猜拳
            switch (extra)
            {
                case 1:
                    return 30;
                case 2:
                    return 15;
                default:
                    return 5;
            }
            break;
        case MAZE_EVENT_MONSTER:         //14怪物
            if (extra)
            {
                return 20;
            }
            else
            {
                return 10;
            }
            break;
        default:
            return 0;
    }
}

inline int get_supply(int score, int fac, bool pass)
{
    int supply = 0;
    switch (fac)
    {
        case 1:
            supply = 50 + score/3;
            break;
        case 2:
            supply = 100 + score/2;
            break;
        default:
            supply = 200 + score;
            break;
    }
    if (!pass)
    {
        supply /= 2;
    }
    //军粮数值放大10倍
    if (supply > 0)
        supply *= 10;

    //八卦阵实际收益
    mazeRealReward(supply);
    
    return supply;
}


inline int get_silver(int score, int fac, bool pass)
{
    int silver = fac * 100 * score;
    if (!pass)
    {
        silver /= 2;
    }
    //八卦阵实际收益
    mazeRealReward(silver);
    return silver;
}

boost::shared_ptr<base_maze_event_result> base_maze_event::random_result()
{
    if (m_results.size() == 1)
    {
        return m_results[0];
    }
    else
    {
        boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
        boost::random::discrete_distribution<> dist(m_gailvs);
        int idx = dist(gen);
        assert(idx >= 0 && idx < m_results.size());
        return m_results[idx];
    }
}

char_maze_point::char_maze_point()
{
    id = 0;
    type = 0;
    result = 0;
}

void char_maze_general::Save()
{
    InsertSaveDb("update char_maze_generals set hurt_hp=" + LEX_CAST_STR(m_hp_hurt)
            + ",max_hp=" + LEX_CAST_STR(m_hp_org)
            + ",org_hurt_hp=" + LEX_CAST_STR(m_org_hp_hurt)
            + ",inspired=" + LEX_CAST_STR(m_inspired)
            + " where cid=" + LEX_CAST_STR(cid) + " and guid=" + LEX_CAST_STR(id)
        );
}

char_maze::char_maze(int id,mazeMgr & m)
:m_cid(id)
,m_handle(m)
{
    m_timer = boost::uuids::nil_uuid();
    m_boss_combat_id = 0;
    m_move_range = 1;
    m_view_range = 1;
    m_double_score = 0;
    m_timeout = 0;
    m_cur_pos = 1;
    m_last_pos = 1;
    m_last_event = 0;
    m_left_move_count = 0;
    m_score = 0;
    m_used_times[0] = 0;
    m_used_times[1] = 0;
    m_used_times[2] = 0;
    m_used_times[3] = 0;
    m_star = 0;
    m_curMaze.reset();
    m_state = 0;
    m_bossList.clear();

    m_buffs.clear();
    m_generals.clear();
    m_cur_pos_pending = 0;
    m_cur_pos_score = 0;
}

void char_maze::load()
{
    Query q(GetDb());
    q.get_result("select mid,star,state,score,move,view,curPos,curPosScore,curPosPending,prePos,preEvent,leftTimes,closeTime,fullTimes,skipTimes,changeTimes,resetTimes,extra1,extra2,maze_map,map_state from char_mazes where cid=" + LEX_CAST_STR(m_cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        int mid = q.getval();
        if (mid > 0)
        {
            m_curMaze = m_handle.getBaseMaze(mid);
            if (m_curMaze.get())
            {
                m_star = q.getval();
                m_state = q.getval();
                m_score = q.getval();
                m_move_range = q.getval();
                m_view_range = q.getval();
                m_cur_pos = q.getval();
                m_cur_pos_score = q.getval();
                m_cur_pos_pending = q.getval();
                m_last_pos = q.getval();
                m_last_event = q.getval();
                m_left_move_count = q.getval();
                m_timeout = q.getval();
                m_used_times[0] = q.getval();
                m_used_times[1] = q.getval();
                m_used_times[2] = q.getval();
                m_used_times[3] = q.getval();
                extra[0] = q.getval();
                extra[1] = q.getval();

                //迷宫地图数据
                std::string maze_types = q.getstr();
                std::string maze_results = q.getstr();
                q.free_result();

                json_spirit::Value types;
                json_spirit::read(maze_types, types);
                if (types.type() == json_spirit::array_type)
                {
                    json_spirit::Array& types_array = types.get_array();
                    for (json_spirit::Array::iterator it = types_array.begin(); it != types_array.end(); ++it)
                    {
                        if ((*it).type() != json_spirit::int_type)
                        {
                            break;
                        }
                        m_map_types.push_back((*it).get_int());
                    }
                    if (m_map_types.size() != iMazeSize)
                    {
                        m_curMaze.reset();
                        Clear();
                        return;
                    }
                }
                else
                {
                    m_curMaze.reset();
                    Clear();
                    return;
                }

                if (maze_results == "")
                {
                    m_map_results.clear();
                    m_map_results.insert(m_map_results.begin(), iMazeSize, 0);
                }
                else
                {
                    json_spirit::Value results;
                    json_spirit::read(maze_results, results);
                    if (results.type() == json_spirit::array_type)
                    {
                        json_spirit::Array& results_array = results.get_array();
                        for (json_spirit::Array::iterator it = results_array.begin(); it != results_array.end(); ++it)
                        {
                            if ((*it).type() != json_spirit::int_type)
                            {
                                break;
                            }
                            m_map_results.push_back((*it).get_int());
                        }
                        if (m_map_results.size() != iMazeSize)
                        {
                            m_curMaze.reset();
                            Clear();
                            return;
                        }
                    }
                    else
                    {
                        m_curMaze.reset();
                        Clear();
                        return;
                    }
                }

                //boss数据
                if (m_state == 1)
                {
                    q.get_result("select seq,bossId,state from char_maze_boss where cid=" + LEX_CAST_STR(m_cid) + " order by seq");
                    CHECK_DB_ERR(q);
                    while (q.fetch_row())
                    {
                        int seq = q.getval();
                        if (seq != (m_bossList.size() + 1))
                        {
                            continue;
                        }
                        char_maze_boss boss_data;
                        boss_data.id = q.getval();
                        boss_data.state = q.getval();
                        m_bossList.push_back(boss_data);
                    }
                    q.free_result();
                }
                //buff数据 
                q.get_result("select type,disableTime from char_maze_buffs where cid=" + LEX_CAST_STR(m_cid) + " and disableTime>unix_timestamp()");
                CHECK_DB_ERR(q);
                while (q.fetch_row())
                {
                    char_maze_buff b;
                    b.type = q.getval();
                    b.remove_time = q.getval();
                    m_buffs.push_back(b);
                }
                q.free_result();

                //武将数据
                q.get_result("select pos,guid,gid,level,color,nickname,str,wisdom,tong,org_attack,org_wufang,org_cefang,org_max_hp,org_hurt_hp,combatAttr,max_hp,hurt_hp,inspired from char_maze_generals where cid=" + LEX_CAST_STR(m_cid) + " order by pos");
                CHECK_DB_ERR(q);
                while (q.fetch_row())
                {
                    char_maze_general cg;
                    cg.pos = q.getval();
                    cg.id = q.getval();
                    cg.gid = q.getval();
                    cg.level = q.getval();

                    boost::shared_ptr<GeneralTypeData> bg = GeneralDataMgr::getInstance()->GetBaseGeneral(cg.gid);
                    if (!bg.get())
                    {
                        continue;
                    }
                    cg.cid = m_cid;
                    cg.spic = bg->m_spic;
                    cg.name = bg->m_name;
                    cg.color = q.getval();
                    cg.b_nickname = q.getval();
                    cg.m_str = q.getval();
                    cg.m_int = q.getval();
                    cg.m_tongyu = q.getval();
                    cg.m_org_attack = q.getval();
                    cg.m_org_wu_fang = q.getval();
                    cg.m_org_ce_fang = q.getval();

                    cg.m_org_hp_max = q.getval();
                    cg.m_org_hp_hurt = q.getval();
                    std::string combatAttr = q.getstr();
                    //解析combatAttr
                    cg.m_combat_attribute.load(combatAttr);

                    cg.m_hp_org = q.getval();
                    cg.m_hp_hurt = q.getval();
                    cg.m_inspired = q.getval();

                    if (cg.m_inspired != 0)
                    {
                        cg.m_wu_fang = cg.m_org_wu_fang * (100 + cg.m_inspired) / 100;
                        cg.m_ce_fang = cg.m_org_ce_fang * (100 + cg.m_inspired) / 100;
                        cg.m_attack = cg.m_org_attack * (100 + cg.m_inspired) / 100;
                    }
                    else
                    {
                        cg.m_wu_fang = cg.m_org_wu_fang;
                        cg.m_ce_fang = cg.m_org_ce_fang;
                        cg.m_attack = cg.m_org_attack;
                    }
                    cg.m_combat_attribute.maze_inspired(cg.m_inspired);

                    m_generals.push_back(cg);
                }
                q.free_result();

                updateBuffEffect();

                if (m_map_types[m_cur_pos-1] == MAZE_EVENT_MONSTER)
                {
                    if (extra[0] != 0)
                    {
                        m_cur_pos_pending = 0;
                    }
                }
            }
        }
        q.free_result();
    }
    else
    {
        q.free_result();

        q.execute("insert into char_mazes (cid,mid) values (" + LEX_CAST_STR(m_cid) + ",0)");
        CHECK_DB_ERR(q);
    }
}

void char_maze::init()
{
    m_last_pos = iMazeStartPoint;
    m_cur_pos_pending = 0;
    m_move_range = 1;    //移动范围
    m_view_range = 1;    //视野范围
    m_double_score = 0; //下次是否双倍积分
    m_timeout = time(NULL) + iMazeTimeout;        //超时时间
    m_cur_pos = iMazeStartPoint;            //当前位置
    m_last_event = MAZE_EVENT_START;        //上次触发事件类型
    m_left_move_count = iMazeOriginalMoveSteps;    //剩余行动次数
    m_score = 0;                //当前积分
    m_used_times[0] = 0;        //4类操作使用次数，1恢复，2跳过，3变更 4重置
    m_used_times[1] = 0;
    m_used_times[2] = 0;
    m_used_times[3] = 0;
    //int m_star;                 //迷宫难度
    //boost::shared_ptr<base_maze_map> m_curMaze; //当前迷宫
    
    m_state = 0;        //状态，0闯关阶段  1打boss阶段
    m_bossList.clear();    //可以击杀的boss列表
    
    //char_maze_point m_points[iMazeSize];//迷宫地图数据
    m_handle.random_maze(m_map_types);

    m_map_results.clear();
    m_map_results.insert(m_map_results.begin(), iMazeSize, 0);

    m_buffs.clear();    //buff列表

    //std::list<char_maze_general> m_generals;    //队伍里面的武将
    reset_generals(m_cid);
}

void char_maze::reset_generals(int cid)
{
    //获得角色信息
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
    {
        ERR();
        return;
    }
    cdata->updateAttackDefense();
    //获得角色武将数据
    CharTotalGenerals& char_generals = cdata->GetGenerals();
    //角色阵型信息
    CharZhens& zhens = cdata->GetZhens();
    boost::shared_ptr<ZhenData> zdata = zhens.GetZhen(zhens.GetDefault());
    if (!zdata.get())
    {
        ERR();
        cout<<"default zhen "<<zhens.GetDefault()<<endl;
        return;
    }
    m_generals.clear();

    saveDbJob job;
    job.sqls.push_back("delete from char_maze_generals where cid=" + LEX_CAST_STR(m_cid));
    for (size_t i = 0; i < 9; ++i)
    {
        if (zdata->m_generals[i] > 0)
        {
            boost::shared_ptr<CharGeneralData> sp = char_generals.GetGenral(zdata->m_generals[i]);
            if (sp.get())
            {
                const CharGeneralData& gdata = *sp.get();
                if (gdata.m_baseGeneral.get() && gdata.m_baseSoldier.get())
                {
                    char_maze_general cmg;
                    cmg.pos = i + 1;
                    cmg.id = gdata.m_id;
                    cmg.gid = gdata.m_gid;
                    cmg.cid = gdata.m_cid;
                    cmg.spic = gdata.m_spic;
                    cmg.level = gdata.m_level;
                    cmg.color = gdata.m_color;
                    cmg.b_nickname = gdata.b_nickname;
                    cmg.name = gdata.m_baseGeneral->m_name;
                    //三围
                    cmg.m_str = gdata.m_str;
                    cmg.m_int = gdata.m_int;
                    cmg.m_tongyu = gdata.m_tongyu;
                    if (gdata.m_baowu_type != 0)
                    {
                        switch(gdata.m_baowu_type)
                        {
                            case 1:
                                cmg.m_str += gdata.m_baowu_add;
                                break;
                            case 2:
                                cmg.m_int += gdata.m_baowu_add;
                                break;
                            case 3:
                                cmg.m_tongyu += gdata.m_baowu_add;
                                break;
                            default:
                                break;
                        }
                    }
                    //战斗属性继承自主将
                    cmg.m_combat_attribute = cdata->m_combat_attribute;
                    //战斗属性加上兵种自带的属性
                    cmg.m_combat_attribute += gdata.m_baseSoldier->m_combat_attribute;
                    //战斗属性
                     cmg.m_combat_attribute += gdata.m_combat_attr;
                    //获得练兵数据
                    boost::shared_ptr<CharTrainings> ct = Singleton<trainingMgr>::Instance().getChar(*(cdata.get()));
                    if (ct.get())
                    {
                        //战斗属性加上兵魂属性
                        cmg.m_combat_attribute += ct->_combatAttr;
                    }
                    cmg.m_combat_attribute.enable();
                    
                    if (act_wuli_attack == gdata.m_baseSoldier->m_damage_type)
                    {
                        cmg.m_attack = 2 * cmg.m_str + cdata->getPugong(true) + cmg.m_combat_attribute.skill_add(1) + gdata.m_attack + cmg.m_combat_attribute.soul_add_attack(gdata.m_baseSoldier->m_base_type);
                    }
                    else
                    {
                        cmg.m_attack = 2 * cmg.m_int + cdata->getCegong(true) + cmg.m_combat_attribute.skill_add(3) + gdata.m_attack + cmg.m_combat_attribute.soul_add_attack(gdata.m_baseSoldier->m_base_type);
                    }
                    cmg.m_wu_fang = 7 * cmg.m_str / 5 + cdata->getPufang(true) + cmg.m_combat_attribute.skill_add(2) + gdata.m_pufang + cmg.m_combat_attribute.soul_add_wufang(gdata.m_baseSoldier->m_base_type);
                    cmg.m_ce_fang = 7 * cmg.m_int / 5 + cdata->getCefang(true) + cmg.m_combat_attribute.skill_add(4) + gdata.m_cefang + cmg.m_combat_attribute.soul_add_cefang(gdata.m_baseSoldier->m_base_type);
    
                    /****血量 *******************************************/
                    cmg.m_hp_org = 3*cmg.m_tongyu;   //原始血量
                    if (cmg.m_hp_org <= 0)
                    {
                        cmg.m_hp_org = 1;
                    }
                    //兵器装备加成血量
                    cmg.m_hp_org += (cdata->getBingli(true));
                    //技能加血
                    cmg.m_hp_org += cmg.m_combat_attribute.skill_add(0);
                    cmg.m_hp_org += gdata.m_hp;
                    cmg.m_hp_org += cmg.m_combat_attribute.soul_add_hp(gdata.m_baseSoldier->m_base_type);
                    //成长星级加成
                    if (gdata.m_chengzhang_star.get())
                    {
                        cmg.m_attack += gdata.m_chengzhang_star->gongji;
                        cmg.m_wu_fang += gdata.m_chengzhang_star->fangyu;
                        cmg.m_ce_fang += gdata.m_chengzhang_star->fangyu;
                        cmg.m_hp_org += gdata.m_chengzhang_star->bingli;
                    }
                    //将魂加成
                    if (gdata.m_general_soul)
                    {
                        cmg.m_attack += gdata.m_general_soul->getAttack(gdata.m_color);
                        cmg.m_wu_fang += gdata.m_general_soul->getWufang(gdata.m_color);
                        cmg.m_ce_fang += gdata.m_general_soul->getCefang(gdata.m_color);
                        cmg.m_hp_org += gdata.m_general_soul->getBingli(gdata.m_color);
                    }

                    //武将天赋
                    if (gdata.m_baseGeneral.get())
                    {
                        cmg.m_combat_attribute += gdata.m_baseGeneral->m_new_tianfu.m_combatAttr;
                        if (gdata.m_baseGeneral->m_new_tianfu.m_more_hp)
                        {
                            cmg.m_hp_org = (100 + gdata.m_baseGeneral->m_new_tianfu.m_more_hp) * cmg.m_hp_org / 100;
                        }
                    }
                    //限时增益效果加成
                    int hp_buff = 0, attack_buff = 0, wu_fang_buff = 0, ce_fang_buff = 0;
                    //兵力物攻物防策攻策防
                    for (int i = 0; i < 5; ++i)
                    {
                        switch(i+1)
                        {
                            case 1:
                                hp_buff = cmg.m_hp_org * (cdata->m_Buffs.buffs[i].m_value) / 100;
                                break;
                            case 2:
                                if (act_wuli_attack == gdata.m_baseSoldier->m_damage_type)
                                {
                                    attack_buff = cmg.m_attack * (cdata->m_Buffs.buffs[i].m_value) / 100;
                                }
                                break;
                            case 3:
                                wu_fang_buff = cmg.m_wu_fang * (cdata->m_Buffs.buffs[i].m_value) / 100;
                                break;
                            case 4:
                                if (act_wuli_attack != gdata.m_baseSoldier->m_damage_type)
                                {
                                    attack_buff = cmg.m_attack * (cdata->m_Buffs.buffs[i].m_value) / 100;
                                }
                                break;
                            case 5:
                                ce_fang_buff = cmg.m_ce_fang * (cdata->m_Buffs.buffs[i].m_value) / 100;
                                break;
                            default:
                                break;
                        }
                    }

                    //将星录加成
                    int hp_jxl = 0, cefang_jxl = 0, wufang_jxl = 0, attack_jxl = 0;
                    cdata->m_jxl_buff.total_buff_attr.get_add(cmg.m_attack, cmg.m_hp_org, cmg.m_wu_fang, cmg.m_ce_fang, attack_jxl, hp_jxl, wufang_jxl, cefang_jxl);

                    //皇座称号加成
                    int hp_throne = 0, attack_throne = 0, wu_fang_throne = 0, ce_fang_throne = 0;
                    int throne_per = 0;
                    if (cdata->m_nick.check_nick(nick_throne_start))
                    {
                        throne_per = 8;
                    }
                    else if(cdata->m_nick.check_nick(nick_throne_start + 1))
                    {
                        throne_per = 5;
                    }
                    else if(cdata->m_nick.check_nick(nick_throne_start + 2))
                    {
                        throne_per = 3;
                    }
                    hp_throne = cmg.m_hp_org * throne_per / 100;
                    attack_throne = cmg.m_attack * throne_per / 100;
                    wu_fang_throne = cmg.m_wu_fang * throne_per / 100;
                    ce_fang_throne = cmg.m_ce_fang * throne_per / 100;

                    cmg.m_hp_org += (hp_buff + hp_jxl + hp_throne);
                    cmg.m_attack += (attack_buff + attack_jxl + attack_throne);
                    cmg.m_wu_fang += (wu_fang_buff + wufang_jxl + wu_fang_throne);
                    cmg.m_ce_fang += (ce_fang_buff + cefang_jxl + ce_fang_throne);

                    cdata->m_jxl_buff.total_buff_attr.add_special(cmg.m_combat_attribute);
                    
                    cmg.m_hp_hurt = 0;
                    cmg.m_org_hp_hurt = 0;
                    cmg.m_org_hp_max = cmg.m_hp_org;
                    cmg.m_org_attack = cmg.m_attack;
                    cmg.m_org_wu_fang = cmg.m_wu_fang;
                    cmg.m_org_ce_fang = cmg.m_ce_fang;
                    cmg.m_inspired = 0;
                    m_generals.push_back(cmg);

                    std::string combat_string = "";
                    cmg.m_combat_attribute.save(combat_string);
                    job.sqls.push_back("replace into char_maze_generals (cid,pos,guid,gid,level,color,nickname,str,wisdom,tong,org_attack,org_wufang,org_cefang,org_max_hp,org_hurt_hp,combatAttr,max_hp,hurt_hp,inspired) values ("
                        + LEX_CAST_STR(m_cid) + ","
                        + LEX_CAST_STR(cmg.pos) + ","
                        + LEX_CAST_STR(cmg.id) + ","
                        + LEX_CAST_STR(cmg.gid) + ","
                        + LEX_CAST_STR(cmg.level) + ","
                        //+ LEX_CAST_STR(cmg.spic) + ","
                        + LEX_CAST_STR(cmg.color) + ","
                        + LEX_CAST_STR(cmg.b_nickname) + ","
                        + LEX_CAST_STR(cmg.m_str) + ","
                        + LEX_CAST_STR(cmg.m_int) + ","
                        + LEX_CAST_STR(cmg.m_tongyu) + ","
                        + LEX_CAST_STR(cmg.m_org_attack) + ","
                        + LEX_CAST_STR(cmg.m_org_wu_fang) + ","
                        + LEX_CAST_STR(cmg.m_org_ce_fang) + ","
                        + LEX_CAST_STR(cmg.m_org_hp_max) + ","
                        + LEX_CAST_STR(cmg.m_org_hp_hurt) + ",'"
                        + LEX_CAST_STR(combat_string) + "',"
                        + LEX_CAST_STR(cmg.m_hp_org) + ","
                        + LEX_CAST_STR(cmg.m_hp_hurt) + ","
                        + LEX_CAST_STR(cmg.m_inspired) + ")");
                }
            }
        }
    }
    InsertSaveDb(job);
}

//重置剩余
int char_maze::reset_left(CharData& cdata, json_spirit::Object& robj)
{
    if (m_state == 1 || m_cur_pos_pending)
    {
        return HC_ERROR;
    }
    //扣除金币
    if (cdata.addGold(-mazeGoldCost(MAZE_GOLD_COST_RESET, m_used_times[MAZE_GOLD_COST_RESET-1]+1)) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, mazeGoldCost(MAZE_GOLD_COST_RESET, m_used_times[MAZE_GOLD_COST_RESET-1]+1), gold_cost_for_maze_reset, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(&cdata,mazeGoldCost(MAZE_GOLD_COST_RESET, m_used_times[MAZE_GOLD_COST_RESET-1]+1),gold_cost_for_maze_reset);
#endif
    cdata.NotifyCharData();
    ++m_used_times[MAZE_GOLD_COST_RESET-1];

    //已经触发的不刷新，未触发的事件类型不变，只是位置再随机下
    std::vector<int> types;
    for (int i = 1; i < (iMazeSize-1); ++i)
    {
        if (m_map_results[i] == 0)
        {
            types.push_back(m_map_types[i]);
        }
    }
    std::random_shuffle ( types.begin(), types.end() );

    std::vector<int>::iterator it = types.begin();
    for (int i = 1; i < (iMazeSize-1); ++i)
    {
        if (m_map_results[i] == 0)
        {
            m_map_types[i] = *it;
            ++it;
        }
    }

    SaveMap();
    return HC_SUCCESS;
}

int char_maze::_mazeFull()
{
    for (std::list<char_maze_general>::iterator it = m_generals.begin(); it != m_generals.end(); ++it)    //队伍里面的武将
    {
        char_maze_general& g = *it;
        g.m_hp_hurt = 0;
        g.m_org_hp_hurt = 0;
        g.Save();
    }
    return HC_SUCCESS;
}

//恢复队伍血量
int char_maze::mazeFull(CharData& cdata, json_spirit::Object& robj)
{
    if (m_state == 1)
    {
        return HC_ERROR;
    }
    //扣除金币
    if (cdata.addGold(-mazeGoldCost(MAZE_GOLD_COST_FULL, m_used_times[MAZE_GOLD_COST_FULL-1]+1)) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, mazeGoldCost(MAZE_GOLD_COST_FULL, m_used_times[MAZE_GOLD_COST_FULL-1]+1), gold_cost_for_maze_full, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(&cdata,mazeGoldCost(MAZE_GOLD_COST_FULL, m_used_times[MAZE_GOLD_COST_FULL-1]+1),gold_cost_for_maze_full);
#endif
    cdata.NotifyCharData();
    ++m_used_times[MAZE_GOLD_COST_FULL-1];

    //补血
    _mazeFull();

    //SaveGenerals();
    return HC_SUCCESS;
}

//迷宫跳过
int char_maze::mazeSkip(CharData& cdata, int id, json_spirit::Object& robj)
{
    if (m_state == 1 || id <= 1 || id >= iMazeSize)
    {
        return HC_ERROR;
    }
    if (m_cur_pos_pending)
    {
        ERR();
        return HC_ERROR;
    }
    if (m_map_results[id-1] != 0)
    {
        ERR();
        return HC_ERROR;
    }
    int distance = maze_distance(id, m_cur_pos);
    if (distance > m_move_range || m_map_types[m_cur_pos-1] == MAZE_EVENT_LONGSTOP)
    {
        ERR();
        //太远了
        return HC_ERROR;
    }
    if (m_left_move_count <= 0)
    {
        ERR();
        return HC_ERROR;
    }
    //扣除金币
    if (cdata.addGold(-mazeGoldCost(MAZE_GOLD_COST_SKIP, m_used_times[MAZE_GOLD_COST_SKIP-1]+1)) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, mazeGoldCost(MAZE_GOLD_COST_SKIP, m_used_times[MAZE_GOLD_COST_SKIP-1]+1), gold_cost_for_maze_skip, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(&cdata,mazeGoldCost(MAZE_GOLD_COST_SKIP, m_used_times[MAZE_GOLD_COST_SKIP-1]+1),gold_cost_for_maze_skip);
#endif
    cdata.NotifyCharData();
    ++m_used_times[MAZE_GOLD_COST_SKIP-1];
    
    --m_left_move_count;

    //移动
    m_last_pos = m_cur_pos;
    m_cur_pos = id;
    m_map_results[id-1] = 999;

    m_move_range = 1;
    m_view_range = 1;

    SaveMapState();
    Save();

    return HC_SUCCESS;
}

//迷宫变更
int char_maze::mazeChange(CharData& cdata, int id, json_spirit::Object& robj)
{
    if (m_state == 1 || id <= 1 || id >= iMazeSize)
    {
        return HC_ERROR;
    }
    if (m_cur_pos_pending)
    {
        ERR();
        return HC_ERROR;
    }
    if (m_map_results[id-1] != 0)
    {
        ERR();
        return HC_ERROR;
    }
    int distance = maze_distance(id, m_cur_pos);
    if (distance > m_move_range)
    {
        ERR();
        //太远了
        return HC_ERROR;
    }
    //扣除金币
    if (cdata.addGold(-mazeGoldCost(MAZE_GOLD_COST_CHANGE, m_used_times[MAZE_GOLD_COST_CHANGE-1]+1)) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, mazeGoldCost(MAZE_GOLD_COST_CHANGE, m_used_times[MAZE_GOLD_COST_CHANGE-1]+1), gold_cost_for_maze_change, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(&cdata,mazeGoldCost(MAZE_GOLD_COST_CHANGE, m_used_times[MAZE_GOLD_COST_CHANGE-1]+1),gold_cost_for_maze_change);
#endif
    cdata.NotifyCharData();
    ++m_used_times[MAZE_GOLD_COST_CHANGE-1];

    //随机事件
    int old_type = m_map_types[id-1];
    for (;;)
    {
        m_map_types[id-1] = my_random(MAZE_EVENT_START+1, MAZE_EVENT_OUT-1);
        if (old_type != m_map_types[id-1])
        {
            break;
        }
    }

    robj.push_back( Pair("type", m_map_types[id-1]) );
    SaveMap();
    Save();
    return HC_SUCCESS;
}

//移动
int char_maze::mazeMove(CharData& cdata, int id, json_spirit::Object& robj)
{
    if (m_state == 1 || id <= 1 || id > iMazeSize || id == m_cur_pos)
    {
        return HC_ERROR;
    }
    assert(m_cur_pos >= 1 && m_cur_pos <= iMazeSize);
    if (m_cur_pos_pending)
    {
        return HC_ERROR;
    }
    int distance = maze_distance(id, m_cur_pos);
    if (distance > m_move_range)
    {
        //太远了
        return HC_ERROR;
    }
    //障碍物不能去
    if (m_map_results[id-1] > 0 && m_map_types[id-1] == MAZE_EVENT_LONGSTOP)
    {
        return HC_ERROR;
    }
    if (m_left_move_count <= 0)
    {
        return HC_ERROR;
    }
    --m_left_move_count;

    //移动
    m_last_pos = m_cur_pos;
    m_cur_pos = id;

    m_move_range = 1;
    m_view_range = 1;

    json_spirit::Array list;
    int next = id;
    while (m_map_results[next-1] == 0
        && m_cur_pos_pending != 1)
    {
        int next_temp = next;
        next = mazeTrigger(cdata, m_map_types[next-1], list);
        if (next > iMazeSize || next < 1)
        {
            //有问题???
            m_left_move_count = 0;
            //调试信息
            const json_spirit::Value val(m_map_types.begin(), m_map_types.end());
            std::string _report = "maze move loop, next = " + LEX_CAST_STR(next) + ",cid:" + LEX_CAST_STR(m_cid) + ",id:" + LEX_CAST_STR(id) + "," + LEX_CAST_STR(next_temp) + "," + json_spirit::write(val);
            InsertSaveDb("insert into char_mails (input,unread,archive,type,`from`,title,content,cid) values (now(),'1','0','1',0,'debug info','"
                + GetDb().safestr(_report)    + "','0')");
            break;
        }
    }

    if (m_state == 0 && m_left_move_count <= 0)
    {
        //通知超时失败了
        notifyFail();

        m_curMaze.reset();    
        Clear();
    }
    else
    {
        robj.push_back( Pair("list", list) );

        SaveMapState();
        Save();
    }
    return HC_SUCCESS;
}

void char_maze::add_event_score(boost::shared_ptr<base_maze_event_result> result, json_spirit::Object& obj)
{
    obj.push_back( Pair("result", result->result));
    int score = result->score;
    obj.push_back( Pair("double", m_double_score));
    if (m_double_score == MAZE_EVENT_RESULT_DOUBLE_SCORE)
    {
        score *= 2;
        m_double_score = 0;
    }
    else if (m_double_score == MAZE_EVENT_RESULT_HALF_SCORE)
    {
        score /= 2;
        m_double_score = 0;
    }
    obj.push_back( Pair("score", score));
    if (score != result->score)
    {
        std::string msg = result->org_msg;
        str_replace(msg, "$S", LEX_CAST_STR(score));
        obj.push_back( Pair("emsg", msg));
    }
    else
    {
        obj.push_back( Pair("emsg", result->msg));
    }
    m_cur_pos_score = score;
    m_map_results[m_cur_pos-1] = result->result;
    m_score += score;
    m_cur_pos_pending = 0;
    return;
}

void char_maze::add_buff(int type, int last_secs)
{
    if (type > 0)
    {
        char_maze_buff b;
        b.type = type;
        b.remove_time = time(NULL) + last_secs;
        m_buffs.push_back(b);

        InsertSaveDb("insert into char_maze_buffs (cid,type,disableTime) values ("
            + LEX_CAST_STR(m_cid) + ","
            + LEX_CAST_STR(type) + ","
            + LEX_CAST_STR(b.remove_time) + ")");
    }

    if (m_buffs.size() > 0)
    {
        std::vector<char_maze_buff>::iterator it = m_buffs.begin();
        while (it != m_buffs.end())
        {
            if (it->remove_time <= time(NULL))
            {
                it = m_buffs.erase(it);
            }
            else
            {
                ++it;
            }
        }

        while (m_buffs.size() > 4)
        {
            m_buffs.erase(m_buffs.begin());
        }
    }
    //buff效果
    updateBuffEffect();
}

void char_maze::updateBuffEffect()
{
    int inspired = getBuffeffect();
    //cout<<"maze set inspired "<<inspired<<endl;
    for (std::list<char_maze_general>::iterator it = m_generals.begin(); it != m_generals.end(); ++it)
    {
        char_maze_general& cg = *it;
        int old = cg.m_combat_attribute.maze_inspired();
        if (old != inspired)
        {
            cg.m_inspired = inspired;
            cg.m_combat_attribute.maze_inspired(inspired);
            int old_hp_org = cg.m_hp_org;

            if (inspired != 0)
            {
                cg.m_hp_org = cg.m_org_hp_max * (100 + inspired) / 100;
                cg.m_wu_fang = cg.m_org_wu_fang * (100 + inspired) / 100;
                cg.m_ce_fang = cg.m_org_ce_fang * (100 + inspired) / 100;
                cg.m_attack = cg.m_org_attack * (100 + inspired) / 100;
            }
            else
            {
                cg.m_hp_org = cg.m_org_hp_max;
                cg.m_wu_fang = cg.m_org_wu_fang;
                cg.m_ce_fang = cg.m_org_ce_fang;
                cg.m_attack = cg.m_org_attack;
            }
            if (old_hp_org != cg.m_hp_org && cg.m_hp_hurt > 0)
            {
                cg.m_hp_hurt = cg.m_org_hp_hurt * cg.m_hp_org / cg.m_org_hp_max;
            }
        }
    }
}

int char_maze::getBuffeffect()
{
    int inspired = 0;
    std::vector<char_maze_buff>::iterator it = m_buffs.begin();
    while (it != m_buffs.end())
    {
        if (it->remove_time > time(NULL))
        {
            if (it->type == 1)
            {
                inspired += 5;
            }
            else
            {
                inspired -= 5;
            }
        }
        ++it;
    }
    return inspired;
}

//触发 - 返回新的落点
int char_maze::mazeTrigger(CharData& cdata, int type, json_spirit::Array& list)
{
    assert(0 == m_map_results[m_cur_pos-1]);
    //触发事件
    switch (type)
    {
        case MAZE_EVENT_CHANGE_POSTION:
            {
                boost::shared_ptr<base_maze_event_result> r = m_handle.randomEventResult(type);
                json_spirit::Object obj;
                add_event_score(r, obj);
                list.push_back(obj);
                int old_pos = m_cur_pos;
                m_last_pos = m_cur_pos;
                for (;;)
                {
                    m_cur_pos = my_random(iMazeStartPoint+1, iMazeOutPoint-1);
                    if (old_pos != m_cur_pos
                        && m_cur_pos > iMazeStartPoint
                        && m_cur_pos < iMazeOutPoint
                        && m_map_types[m_cur_pos-1] != MAZE_EVENT_LONGSTOP)
                    {
                        break;
                    }
                }
                return m_cur_pos;
            }
        case MAZE_EVENT_GUESS_NUMBER:
            {
                //初始化要猜的数字
                extra[0] = my_random(1, 500);
                extra[1] = 0;
                json_spirit::Object obj;
                obj.push_back( Pair("type", type));
                list.push_back(obj);
                m_cur_pos_pending = 1;
                m_last_event = type;
                return m_cur_pos;
            }
        case MAZE_EVENT_REPEAT:            //3重复
            {
                boost::shared_ptr<base_maze_event_result> r = m_handle.randomEventResult(type);
                json_spirit::Object obj;
                add_event_score(r, obj);
                list.push_back(obj);
                if (m_last_event > 0 && m_last_event != MAZE_EVENT_REPEAT)
                {
                    m_map_types[m_cur_pos-1] = m_last_event;
                    m_map_results[m_cur_pos-1] = 0;
                }
                return m_cur_pos;
            }
        case MAZE_EVENT_WILD:            //4狂暴
            {                
                boost::shared_ptr<base_maze_event_result> r = m_handle.randomEventResult(type);
                json_spirit::Object obj;
                add_event_score(r, obj);
                list.push_back(obj);
                add_buff(1, 300);
                m_last_event = type;
                return m_cur_pos;
            }
        case MAZE_EVENT_WEAK:            //5虚弱
            {
                boost::shared_ptr<base_maze_event_result> r = m_handle.randomEventResult(type);
                json_spirit::Object obj;
                add_event_score(r, obj);
                list.push_back(obj);
                add_buff(2, 300);
                m_last_event = type;
                return m_cur_pos;
            }
        case MAGE_EVENT_FULL:            //6恢复
            {
                boost::shared_ptr<base_maze_event_result> r = m_handle.randomEventResult(type);
                json_spirit::Object obj;
                add_event_score(r, obj);
                list.push_back(obj);
                _mazeFull();
                m_last_event = type;
                return m_cur_pos;
            }
        case MAZE_EVENT_DISCOUNT:        //7甩卖
            {
                std::map<int, int> d_list =  Singleton<mazeMgr>::Instance().get_discount_items();
                int tmp1 = 0, tmp2 = 1;
                if (d_list.size() >= 2)
                {
                    tmp1 = my_random(0,d_list.size()-1);
                    do
                    {
                        tmp2 = my_random(0,d_list.size()-1);
                    }while(tmp2 == tmp1);
                }
                extra[0] = tmp1;
                extra[1] = tmp2;
                json_spirit::Object obj;
                obj.push_back( Pair("type", type));
                list.push_back(obj);
                m_cur_pos_pending = 1;
                m_last_event = type;
                return m_cur_pos;
            }
        case MAZE_EVENT_BOMB:            //8炸弹
            {
                boost::shared_ptr<base_maze_event_result> r = m_handle.randomEventResult(type);
                json_spirit::Object obj;
                add_event_score(r, obj);
                list.push_back(obj);
                m_last_event = type;

                //上下左右直接触发掉
                for (int i = 2; i <= (iMazeSize-1); ++i)
                {
                    if (m_map_results[i-1] == 0 && maze_distance(i, m_cur_pos) == 1)
                    {
                        m_map_results[i-1] = 999;
                    }
                }
                return m_cur_pos;
            }
        case MAZE_EVENT_LUCKY:            //9遁甲
            {
                boost::shared_ptr<base_maze_event_result> r = m_handle.randomEventResult(type);
                json_spirit::Object obj;
                add_event_score(r, obj);
                list.push_back(obj);
                m_last_event = type;
                switch (r->result)
                {
                    case MAZE_EVENT_RESULT_LUCKY_MOVE_MORE:
                        m_move_range = 2;
                        break;
                    case MAZE_EVENT_RESULT_LUCKY_VIEW_MORE:
                        m_view_range = 2;
                        break;
                    default:
                        m_left_move_count += 2;
                        break;
                }
                return m_cur_pos;
            }
        case MAZE_EVENT_MISCHANCE:        //10横祸
            {
                boost::shared_ptr<base_maze_event_result> r;
                //关键位置不能是障碍物
                if (m_cur_pos == 3 || m_cur_pos == 87)
                {
                    r = m_handle.getEventResult(MAZE_EVENT_RESULT_LOSE_MOVE_TIMES);
                }
                else
                {
                    r = m_handle.randomEventResult(type);
                }
                json_spirit::Object obj;
                add_event_score(r, obj);
                list.push_back(obj);
                m_last_event = type;
                switch (r->result)
                {
                    case MAZE_EVENT_RESULT_LOSE_MOVE_TIMES:
                        m_left_move_count -= 2;
                        break;
                    default:
                        //遇到障碍物，退回去
                        m_map_types[m_cur_pos-1] = MAZE_EVENT_LONGSTOP;
                        m_cur_pos = m_last_pos;
                        SaveMap();
                        break;
                }
                return m_cur_pos;
            }
        case MAZE_EVENT_SCORE:            //11积分
            {
                boost::shared_ptr<base_maze_event_result> r = m_handle.randomEventResult(type);
                json_spirit::Object obj;
                add_event_score(r, obj);
                list.push_back(obj);

                m_double_score = r->result;
                m_last_event = type;
                return m_cur_pos;
            }
        case MAZE_EVENT_MORA:            //12猜拳
            {
                extra[0] = my_random(1, 3);
                json_spirit::Object obj;
                obj.push_back( Pair("type", type));
                list.push_back(obj);
                m_cur_pos_pending = 1;
                m_last_event = type;
                return m_cur_pos;
            }
        case MAZE_EVENT_GEM:             //13宝物
            {
                extra[0] = my_random(1, 3);
                json_spirit::Object obj;
                obj.push_back( Pair("type", type));
                list.push_back(obj);
                m_cur_pos_pending = 1;
                m_last_event = type;
                m_handle.random_lottery_items(m_lottery_list, 3);
                return m_cur_pos;
            }
        case MAZE_EVENT_OUT:
            {
                if (m_double_score == MAZE_EVENT_RESULT_DOUBLE_SCORE)
                {
                    m_score += 200;
                }
                else if (m_double_score == MAZE_EVENT_RESULT_HALF_SCORE)
                {
                    m_score += 50;
                }
                else
                {
                    m_score += 100;
                }
                m_double_score = 0;
                m_state = 1;    //进入打BOSS阶段
                m_map_results[m_cur_pos-1] = MAZE_EVENT_RESULT_OUT;

                //补血
                _mazeFull();

                SaveMapState();
                //SaveGenerals();
                Save();
                
                json_spirit::Object obj;
                obj.push_back( Pair("cmd", "mazeResult") );
                obj.push_back( Pair("s", 200) );
                obj.push_back( Pair("star", m_star) );
                obj.push_back( Pair("success", 1) );
                obj.push_back( Pair("score", m_score) );

                int pj = getPingjia(m_score);
                obj.push_back( Pair("pj", pj) );

                int fac = 1;
                if (m_curMaze.get())
                {
                    fac = m_curMaze->id;
                    if (fac < 1 || fac > 3)
                    {
                        fac = 1;
                    }
                }
                int supply = get_supply(m_score, fac, true);
                int silver = get_silver(m_score, fac, true);

                cdata.addSilver(silver);
                cdata.addTreasure(treasure_type_supply, supply);
                cdata.NotifyCharData();

                json_spirit::Array alist;

                json_spirit::Object supplyO;
                supplyO.push_back( Pair("type", item_type_treasure) );
                supplyO.push_back( Pair("id", treasure_type_supply) );
                supplyO.push_back( Pair("nums", supply) );
                alist.push_back(supplyO);

                json_spirit::Object silverO;
                silverO.push_back( Pair("type", item_type_silver) );
                //silverO.push_back( Pair("id", treasure_type_supply) );
                silverO.push_back( Pair("nums", silver) );
                alist.push_back(silverO);
                obj.push_back( Pair("list", alist) );

                //根据积分/评价给出boss列表
                for (int i = 0; i < pj; ++i)
                {
                    char_maze_boss boss_data;
                    boss_data.id = m_curMaze->stars[m_star-1].boss_id[i];
                    boss_data.state = 0;
                    m_bossList.push_back(boss_data);
                    InsertSaveDb("replace into char_maze_boss (cid,seq,bossId,state) values (" + LEX_CAST_STR(m_cid)
                        + "," + LEX_CAST_STR(i+1)
                        + "," + LEX_CAST_STR(boss_data.id)
                        + ",0)");
                }
                json_spirit::Array blist;
                for (std::list<char_maze_boss>::iterator it = m_bossList.begin(); it != m_bossList.end(); ++it)
                {
                    boost::shared_ptr<mazeMonster> pm = m_handle.getMonsterById(it->id);
                    if (pm.get())
                    {
                        json_spirit::Object boss;
                        boss.push_back( Pair("id", pm->_id) );
                        boss.push_back( Pair("name", pm->_name) );
                        boss.push_back( Pair("spic", pm->_spic) );
                        boss.push_back( Pair("level", pm->_level) );
                        blist.push_back(boss);
                    }
                }
                obj.push_back( Pair("blist", blist) );
                //返回通关结果
                cdata.sendObj(obj);
                return m_cur_pos;
            }
        case MAZE_EVENT_MONSTER:         //14怪物
        default:
            {
                boost::shared_ptr<base_maze_event_result> r = m_handle.randomEventResult(type);
                m_map_results[m_cur_pos-1] = r->result;
                json_spirit::Object obj;
                obj.push_back( Pair("type", type));
                obj.push_back( Pair("result", r->result));
                list.push_back(obj);
                m_cur_pos_pending = 1;
                m_cur_pos_score = r->score;
                if (m_double_score == MAZE_EVENT_RESULT_DOUBLE_SCORE)
                {
                    m_cur_pos_score *= 2;
                    m_double_score = 0;
                }
                else if (m_double_score == MAZE_EVENT_RESULT_HALF_SCORE)
                {
                    m_cur_pos_score /= 2;
                    m_double_score = 0;
                }
                m_last_event = type;
                extra[0] = 0;
                extra[1] = r->result;
                return m_cur_pos;
            }
    }
}

void char_maze::combatEnd(Combat* pCombat)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        return;
    }
    //击杀boss阶段
    if (m_state == 1)
    {
        m_boss_combat_id = 0;
        if (attacker_win == pCombat->m_state)
        {
            std::list<char_maze_boss>::iterator it;
            int id = 1;
            for (it = m_bossList.begin(); it != m_bossList.end(); ++it)
            {
                if (it->state == 0)
                {
                    //击杀成功
                    it->state = 1;

                    {
                        //给奖励!
                        std::list<Item> getItems;
                        lootMgr::getInstance()->getMazeBossLoots(it->id, getItems, 0);
                        int tmp = 1000;
                        mazeRealReward(tmp);
                        if (tmp != 1000)
                        {
                            for (std::list<Item>::iterator it = getItems.begin(); it != getItems.end(); ++it)
                            {
                                Item& itm = *it;
                                mazeRealReward(itm.nums);
                            }
                        }
                        giveLoots(cdata.get(), getItems, 0, cdata->m_level, 0, pCombat, NULL, true, give_maze);
                    }
                    SaveBoss(id);
                    ++id;
                    ++it;
                    break;
                }
            }
            int left = 0;
            while (it != m_bossList.end())
            {
                ++left;
                ++it;
            }
            if (left == 0)
            {
                //公告
                int pj = getPingjia(m_score);
                if (pj > 2)
                {
                    std::string msg = "";
                    if (pj == 3)
                    {
                        msg = strMazeNotify1;
                    }
                    else if (pj == 4)
                    {
                        msg = strMazeNotify2;
                    }
                    str_replace(msg, "$N", MakeCharNameLink(cdata->m_name));
                    if (m_curMaze.get())
                    {
                        str_replace(msg, "$G", m_curMaze->name);
                        str_replace(msg, "$H", m_curMaze->stars[m_star-1].memo);
                    }
                    if (msg != "")
                    {
                        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
                    }
                }
                //支线任务
                cdata->m_trunk_tasks.updateTask(task_maze_score, pj);
                
                //七日目标
                Singleton<seven_Goals_mgr>::Instance().updateGoals(*(cdata.get()),cdata->queryCreateDays(),goals_type_maze,m_curMaze->id);
                //act统计
                act_to_tencent(cdata.get(),act_new_maze_finish,m_curMaze->id,m_star);
                m_curMaze.reset();
                Clear();

                if (m_timer.is_nil() == false)
                {
                    splsTimerMgr::getInstance()->delTimer(m_timer);
                    m_timer = boost::uuids::nil_uuid();
                }

                //通知数字变化
                int enterTimes = cdata->queryExtraData(char_data_type_daily, char_data_daily_maze_times);
                int left = iMazeTimesEveryday - enterTimes;
                cdata->notifyEventState(top_level_event_maze, 0, left);
            }
        }
    }
    //遇到怪物战斗未完成
    else if (m_cur_pos >= 1 && m_cur_pos <= iMazeSize
        && m_cur_pos_pending
        && m_map_types[m_cur_pos-1] == MAZE_EVENT_MONSTER)
    {
        if (attacker_win == pCombat->m_state)
        {
            Army* winner_army = pCombat->m_attacker;
            //更新剩余血量
            for (std::list<char_maze_general>::iterator it = m_generals.begin(); it != m_generals.end(); ++it)    //队伍里面的武将
            {
                char_maze_general& g = *it;
                //g.m_hp_now = g.m_hp_org;
                iGeneral* combat_g = winner_army->GetGeneral(g.pos);
                if (combat_g)
                {
                    int cur_hp = combat_g->Hp();
                    if (cur_hp > 0)
                    {
                        int inspired = combat_g->total_inspired();
                        if (inspired != 0 && inspired != -100)
                        {
                            cur_hp = cur_hp * 100 / (100 + inspired);
                            if (cur_hp <= 0)
                            {
                                cur_hp = 1;
                            }
                        }
                    }
                    if (cur_hp > 0)
                    {
                        int hurt = combat_g->MaxHp() - cur_hp;
                        if (combat_g->MaxHp() == g.m_hp_org)
                        {
                            g.m_hp_hurt = hurt;
                        }
                        else
                        {
                            g.m_hp_hurt = hurt * g.m_hp_org / combat_g->MaxHp();                            
                        }
                        
                        if (combat_g->MaxHp() == g.m_org_hp_max)
                        {
                            g.m_org_hp_hurt = hurt;
                        }
                        else
                        {
                            g.m_org_hp_hurt = hurt * g.m_org_hp_max / combat_g->MaxHp();                            
                        }
                    }
                    else
                    {
                        g.m_hp_hurt = g.m_hp_org;
                        g.m_org_hp_hurt = g.m_org_hp_max;
                    }
                }
                else
                {
                    g.m_hp_hurt = g.m_hp_org;
                    g.m_org_hp_hurt = g.m_org_hp_max;
                }
            }
            //事件结算
            m_score += m_cur_pos_score;
            m_cur_pos_pending = 0;

            SaveGenerals();
            SaveMapState();
            Save();
        }
        else
        {
            //战斗失败迷阵结束
            //通知超时失败了
            notifyFail();

            m_curMaze.reset();
            Clear();
        }
    }
}

void char_maze::notifyFail()
{
    //通知超时失败
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "mazeResult") );
    obj.push_back( Pair("s", 200) );
    obj.push_back( Pair("success", 0) );
    obj.push_back( Pair("score", m_score) );
    obj.push_back( Pair("star", m_star) );
    obj.push_back( Pair("pj", getPingjia(m_score)) );
    json_spirit::Array alist;
    int fac = 1;
    if (m_curMaze.get())
    {
        fac = m_curMaze->id;
        if (fac < 1 || fac > 3)
        {
            fac = 1;
        }
    }
    int supply = get_supply(m_score, fac, false);
    int silver = get_silver(m_score, fac, false);

    json_spirit::Object supplyO;
    supplyO.push_back( Pair("type", item_type_treasure) );
    supplyO.push_back( Pair("id", treasure_type_supply) );
    supplyO.push_back( Pair("nums", supply) );
    alist.push_back(supplyO);

    json_spirit::Object silverO;
    silverO.push_back( Pair("type", item_type_silver) );
    //silverO.push_back( Pair("id", treasure_type_supply) );
    silverO.push_back( Pair("nums", silver) );
    alist.push_back(silverO);

    obj.push_back( Pair("list", alist) );
    //返回通关结果
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (cdata.get())
    {
        cdata->addSilver(silver);
        cdata->addTreasure(treasure_type_supply, supply);
        cdata->NotifyCharData();
        cdata->sendObj(obj);

        //通知数字变化
        int enterTimes = cdata->queryExtraData(char_data_type_daily, char_data_daily_maze_times);
        int left = iMazeTimesEveryday - enterTimes;
        cdata->notifyEventState(top_level_event_maze, 0, left);
    }

    if (m_timer.is_nil() == false)
    {
        splsTimerMgr::getInstance()->delTimer(m_timer);
        m_timer = boost::uuids::nil_uuid();
    }    
}

void char_maze::Save()
{
    if (m_curMaze.get())
    {
        InsertSaveDb("update char_mazes set mid=" + LEX_CAST_STR(m_curMaze->id)
            + ",star=" + LEX_CAST_STR(m_star)
            + ",state=" + LEX_CAST_STR(m_state)
            + ",score=" + LEX_CAST_STR(m_score)
            + ",move=" + LEX_CAST_STR(m_move_range)
            + ",view=" + LEX_CAST_STR(m_view_range)
            + ",curPos=" + LEX_CAST_STR(m_cur_pos)
            + ",curPosPending=" + LEX_CAST_STR(m_cur_pos_pending)
            + ",curPosScore=" + LEX_CAST_STR(m_cur_pos_score)
            + ",prePos=" + LEX_CAST_STR(m_last_pos)
            + ",preEvent=" + LEX_CAST_STR(m_last_event)
            + ",leftTimes=" + LEX_CAST_STR(m_left_move_count)
            + ",closeTime=" + LEX_CAST_STR(m_timeout)
            + ",fullTimes=" + LEX_CAST_STR(m_used_times[0])
            + ",skipTimes=" + LEX_CAST_STR(m_used_times[1])
            + ",changeTimes=" + LEX_CAST_STR(m_used_times[2])
            + ",resetTimes=" + LEX_CAST_STR(m_used_times[3])
            + ",extra1=" + LEX_CAST_STR(extra[0])
            + ",extra2=" + LEX_CAST_STR(extra[1])
            + " where cid=" + LEX_CAST_STR(m_cid));
    }
    else
    {
        InsertSaveDb("update char_mazes set mid=0 where cid=" + LEX_CAST_STR(m_cid));
    }
}

void char_maze::Clear()
{
    saveDbJob job;
    job.sqls.push_back("update char_mazes set mid=0 where cid=" + LEX_CAST_STR(m_cid));
    job.sqls.push_back("delete from char_maze_boss where cid=" + LEX_CAST_STR(m_cid));
    job.sqls.push_back("delete from char_maze_buffs where cid=" + LEX_CAST_STR(m_cid));
    job.sqls.push_back("delete from char_maze_item_list where cid=" + LEX_CAST_STR(m_cid));
    job.sqls.push_back("delete from char_maze_generals where cid=" + LEX_CAST_STR(m_cid));
    InsertSaveDb(job);
}

void char_maze::SaveGenerals()
{
    for (std::list<char_maze_general>::iterator it = m_generals.begin(); it != m_generals.end(); ++it)    //队伍里面的武将
    {
        char_maze_general& g = *it;
        g.Save();
    }
}

void char_maze::SaveBoss(int id)
{
    InsertSaveDb("update char_maze_boss set state=1 where cid=" + LEX_CAST_STR(m_cid) + " and seq=" + LEX_CAST_STR(id));
}

void char_maze::SaveMap()
{
    const json_spirit::Value val(m_map_types.begin(), m_map_types.end());
    InsertSaveDb("update char_mazes set maze_map='" + json_spirit::write(val) + "' where cid=" + LEX_CAST_STR(m_cid));
}

void char_maze::SaveMapState()
{
    const json_spirit::Value val(m_map_results.begin(), m_map_results.end());
    InsertSaveDb("update char_mazes set map_state='" + json_spirit::write(val) + "' where cid=" + LEX_CAST_STR(m_cid));
}

int mazeMonster::load()
{
    Query q(GetDb());
    q.get_result("select pos,name,spic,stype,hp,attack,pufang,cefang,str,wisdom,special,skill from base_maze_boss_generals where bid=" + LEX_CAST_STR(_id)+ " order by pos");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int pos = q.getval();
        if (pos >= 9)
        {
            pos = 9;
        }
        else if (pos < 1)
        {
            pos = 1;
        }
        boost::shared_ptr<StrongholdGeneralData> sg;
        if (!(m_generals[pos-1].get()))
        {
            sg.reset(new (StrongholdGeneralData));
            m_generals[pos-1] = sg;
        }
        else
        {
            sg = m_generals[pos-1];
        }
        sg->m_pos = pos;
        sg->m_name = q.getstr();
        sg->m_spic = q.getval();
        sg->m_stype = q.getval();
        sg->m_hp = q.getval();
        sg->m_attack = q.getval();
        sg->m_pufang = q.getval();
        sg->m_cefang = q.getval();
        sg->m_str = q.getval();
        sg->m_int = q.getval();
        sg->m_special = q.getval();
        sg->m_speSkill = GeneralDataMgr::getInstance()->getSpeSkill(q.getval());
        sg->m_level = _level;

        sg->m_baseSoldier = GeneralDataMgr::getInstance()->GetBaseSoldier(sg->m_stype);
    }
    q.free_result();
    return 0;
}

mazeMgr::mazeMgr()
{
    load();
}

void mazeMgr::load()
{
    lootMgr::getInstance();
    
    Query q(GetDb());

    //迷宫列表
    q.get_result("select m.id,m.name,md.level from base_mazes as m left join base_maze_data as md on m.id=md.id where 1 order by m.id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<base_maze> bm(new base_maze);
        bm->id = q.getval();
        bm->name = q.getstr();
        bm->openLevel = q.getval();

        m_mazes.push_back(bm);
        assert(bm->id == m_mazes.size());
    }
    q.free_result();

    q.get_result("select mid,star,tjLevel,tjAttack,diaoluo,memo from base_maze_stars where 1 order by mid,star");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mid = q.getval();
        int star = q.getval();
        if (mid >= 1 && mid <= m_mazes.size() && star >= 1 && star <= 3)
        {
            base_maze_map_data& md = m_mazes[mid-1]->stars[star-1];
            md.star = star;
            md.tjLevel = q.getval();
            md.tjAttack = q.getval();
            md.gailv = q.getstr();
            md.memo = q.getstr();
        }
        else
        {
            ERR();
        }
    }
    q.free_result();

    //抽奖物品
    q.get_result("select type,id,num from base_maze_random_gems where 1");
    while (q.fetch_row())
    {
        Item itm;
        itm.type = q.getval();
        itm.id = q.getval();
        itm.spic = itm.id;
        itm.nums = q.getval();

        m_lottery_items.push_back(itm);
    }
    q.free_result();
    //折扣物品
    q.get_result("select id,discount from base_maze_disount_gems where 1");
    while (q.fetch_row())
    {
        int item_id = q.getval();
        int discount = q.getval();
        m_discount_items[item_id] = discount;
    }
    q.free_result();
    //各种随机事件出现的总次数
    const int aMazeEventTimes[MAZE_EVENT_OUT-1] = {2,3,2,2,2,4,3,3,7,7,4,4,4,40};
    memcpy(m_maze_event_times, aMazeEventTimes, sizeof(int)*(MAZE_EVENT_OUT-1));

    m_maze_template.clear();
    for (int i = 0; i < (MAZE_EVENT_OUT-1); ++i)
    {
        for (int j = 0; j < m_maze_event_times[i]; ++j)
        {
            m_maze_template.push_back(i+1);
        }
    }
    assert(m_maze_template.size() == (iMazeSize-2));

    //各种事件类型
    q.get_result("select type,name,memo,score from base_maze_events where 1 order by type");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<base_maze_event> e(new base_maze_event);
        e->type = q.getval();
        e->name = q.getstr();
        e->memo = q.getstr();
        e->score = q.getstr();
        m_events.push_back(e);
        assert(e->type == m_events.size());
    }
    q.free_result();

    //事件的结果
    q.get_result("select d.result,d.type,d.score,d.gailv,d.param,r.msg from base_maze_event_results_data as d left join base_maze_event_results as r on d.result=r.result where 1 order by r.result");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<base_maze_event_result> r(new base_maze_event_result);
        r->result = q.getval();
        r->event_type = q.getval();
        r->score = q.getval();
        r->gailv = q.getval();
        r->param_count = q.getval();

        r->msg = q.getstr();
        r->org_msg = r->msg;
        if (r->param_count > 0)
        {
            str_replace(r->msg, "$S", LEX_CAST_STR(r->score));
        }

        assert(r->event_type >= 1 && r->event_type <= m_events.size());

        m_events[r->event_type-1]->m_results.push_back(r);
        m_events[r->event_type-1]->m_gailvs.push_back(r->gailv);

        m_results.push_back(r);
        assert(m_results.size() == r->result);
    }
    q.free_result();

    std::map<int,int> mod_map;
    mod_map[0] = 1;
    q.get_result("select seq,`mod` from base_maze_boss where 1");
    while (q.fetch_row())
    {
        int seq = q.getval();
        int mod = q.getval();
        mod_map[seq] = mod;
    }
    q.free_result();

    //各种怪
    q.get_result("select id,mid,star,type,spic,level,seq,name,hit,crit,shipo,parry,resist_hit,resist_crit,resist_shipo,resist_parry from base_maze_boss_army where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int mid = q.getval();
        int star = q.getval();
        int type = q.getval();
        boost::shared_ptr<mazeMonster> m_data;
        m_data.reset(new mazeMonster);
        m_data->_id = id;
        m_data->_spic = q.getval();
        m_data->_level = q.getval();
        int seq = q.getval();
        m_data->_mod = mod_map[seq];
        m_data->_name = q.getstr();
        //特性hit,crit,shipo,parry,resist_hit,resist_crit,resist_shipo,resist_parry
        m_data->m_combat_attribute.special_resist(special_attack_dodge, 10 * q.getval());
        m_data->m_combat_attribute.special_attack(special_attack_baoji, 10 * q.getval());
        m_data->m_combat_attribute.special_attack(special_attack_shipo, 10 * q.getval());
        m_data->m_combat_attribute.special_attack(special_attack_parry, 10 * q.getval());
        m_data->m_combat_attribute.special_attack(special_attack_dodge, 10 * q.getval());
        m_data->m_combat_attribute.special_resist(special_attack_baoji, 10 * q.getval());
        m_data->m_combat_attribute.special_resist(special_attack_shipo, 10 * q.getval());
        m_data->m_combat_attribute.special_resist(special_attack_parry, 10 * q.getval());
        m_data->m_combat_attribute.enable();
        //加载武将
        m_data->load();
        //掉落加载
        //lootMgr::getInstance()->getEliteCombatsLootInfo(m_data->_id, m_data->m_Item_list);
        //怪物id关联到基础迷阵
        if (mid >= 1 && mid <= m_mazes.size() && star >= 1 && star <= 3)
        {
            base_maze_map_data& md = m_mazes[mid-1]->stars[star-1];
            if (type == 0)
                md.normal_mid = id;
            else if(type == 1)
                md.elite_mid = id;
            else
            {
                md.boss_id.push_back(id);
                //cout<<"mid "<<mid<<",star "<<star<<",boss "<<seq<<"->"<<id<<endl;
            }
            //md.loots载入
        }

        //boss掉落
        lootMgr::getInstance()->getMazeBossLootsInfo(id, m_data->m_Item_list);
        //用1star第一个boss作为副本的掉落
        if (star == 1 && seq == 1 && type == 2)
        {
            base_maze& bm = *(m_mazes[mid-1].get());
            bm.loots = m_data->m_Item_list;
        }
        m_mazeMonsters[id] = m_data;
    }
    q.free_result();

    /*
    cout<<"results-->"<<endl;
    for (std::vector<boost::shared_ptr<base_maze_event_result> >::iterator it = m_results.begin();
        it != m_results.end();
        ++it)
    {
        cout<<"result:"<<(*it)->result<<",type:"<<(*it)->event_type<<endl;
    }

    cout<<"types-->"<<endl;
    //各种事件类型
    for (std::vector<boost::shared_ptr<base_maze_event> >::iterator it = m_events.begin();
        it != m_events.end(); ++it)
    {
        base_maze_event& e = *((*it).get());
        cout<<"type "<<e.type<<endl;
        for (std::vector<boost::shared_ptr<base_maze_event_result> >::iterator itr = e.m_results.begin();
            itr != e.m_results.end(); ++itr)
        {
            cout<<"\t"<<(*itr)->result<<","<<(*itr)->event_type<<endl;
        }
    }*/
    
    //加载玩家数据
    q.execute("delete from char_maze_buffs where disableTime<unix_timestamp()");
    q.execute("delete from char_mazes where closeTime<unix_timestamp()");
    
    
}

boost::shared_ptr<char_maze> mazeMgr::getChar(int cid)
{
    std::map<int, boost::shared_ptr<char_maze> >::iterator it = m_char_datas.find(cid);
    if (it != m_char_datas.end())
    {
        return it->second;
    }
    boost::shared_ptr<char_maze> r(new char_maze(cid, *this));
    m_char_datas[cid] = r;
    r->load();
    if (r->m_curMaze.get()
        && (r->m_timeout <= time(NULL)
            || r->m_map_results.size() != iMazeSize
            || r->m_map_types.size() != iMazeSize))
    {
        if (r->m_state == 0)
        {
            //通知超时失败了
            r->notifyFail();
        }
        r->m_curMaze.reset();
        r->Clear();
    }
    else if (r->m_curMaze.get())
    {
        int t = r->m_timeout - time(NULL);
        //定时器通知，刷新
        json_spirit::mObject mobj;
        mobj["cmd"] = "mazeTimeout";
        mobj["cid"] = cid;
        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(t, 1, mobj,1));
        r->m_timer = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return r;
}

int mazeMgr::getMazeList(CharData& cdata, json_spirit::Object& robj)
{
    (void)cdata;
    //是否在迷宫中
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);

    json_spirit::Array list;
    for (std::vector<boost::shared_ptr<base_maze> >::iterator it = m_mazes.begin(); it != m_mazes.end(); ++it)
    {
        if ((*it).get())
        {
            base_maze& bm = *((*it).get());
            json_spirit::Object obj;
            obj.push_back( Pair("id", bm.id) );
            obj.push_back( Pair("name", bm.name) );
            obj.push_back( Pair("level", bm.openLevel) );

            //是否在迷宫中
            if (cm.get() && cm->m_curMaze.get()
                && cm->m_curMaze->id == bm.id)
            {
                int left = cm->m_timeout - time(NULL);
                if (left > 0)
                {
                    obj.push_back( Pair("countDown", left) );
                    obj.push_back( Pair("star", cm->m_star) );
                }                
            }
            //掉落列表
            if (bm.loots.size() > 0)
            {
                json_spirit::Array alist;
                std::list<Item>::iterator it_l = bm.loots.begin();
                while (it_l != bm.loots.end())
                {
                    boost::shared_ptr<json_spirit::Object> p_obj;
                    ItemToObj(&(*it_l), p_obj);
                    if (p_obj.get())
                    {
                        alist.push_back(*(p_obj.get()));
                    }
                    ++it_l;
                }
                obj.push_back( Pair("alist", alist) );
            }

            list.push_back(obj);
        }
    }
    robj.push_back( Pair("mlist", list) );

    int left = 0;
    int enterTimes = cdata.queryExtraData(char_data_type_daily, char_data_daily_maze_times);
    left = iMazeTimesEveryday - enterTimes;
    if (left < 0)
    {
        left = 0;
    }
    robj.push_back( Pair("left", left) );
    return HC_SUCCESS;
}

int mazeMgr::getMazeDetail(int id, json_spirit::Object& robj)
{
    if (id < 1 || id > m_mazes.size()
        || m_mazes[id-1].get() == NULL)
    {
        return HC_ERROR;
    }
    json_spirit::Array list;
    base_maze& bm = *(m_mazes[id-1].get());
    for (int i = 0; i < 3; ++i)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("star", i+1) );
        obj.push_back( Pair("tjLevel", bm.stars[i].tjLevel) );
        obj.push_back( Pair("tjAttack", bm.stars[i].tjAttack) );
        obj.push_back( Pair("gailv", bm.stars[i].gailv) );    
        list.push_back(obj);
    }
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

boost::shared_ptr<mazeMonster> mazeMgr::getMonsterById(int mid)
{
    std::map<int, boost::shared_ptr<mazeMonster> >::iterator it = m_mazeMonsters.find(mid);
    if (it != m_mazeMonsters.end())
    {
        return it->second;
    }
    boost::shared_ptr<mazeMonster> p;
    p.reset();
    return p;
}

int mazeMgr::enterMaze(CharData& cdata, int id, int star, json_spirit::Object& robj)
{
    if (cdata.m_currentStronghold < iMazeOpenStronghold)
    {
        return HC_ERROR;
    }
    //是否在迷宫中
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (cm.get() && cm->m_curMaze.get())
    {
        if (cm->m_curMaze->id == id)
        {
        }
        robj.push_back( Pair("id", cm->m_curMaze->id) );
    }
    else
    {
        int left = 0;
        int enterTimes = cdata.queryExtraData(char_data_type_daily, char_data_daily_maze_times);
        left = iMazeTimesEveryday - enterTimes;
        if (left < 0)
        {
            //次数不够
            return HC_ERROR;
        }
        if (id >= 1 && id <= m_mazes.size())
        {
            robj.push_back( Pair("id", id) );
        }
        else
        {
            return HC_ERROR;
        }
        char_maze* pcm = cm.get();
        if (cdata.m_level < m_mazes[id-1]->openLevel)
        {
            robj.push_back( Pair("needLevel", m_mazes[id-1]->openLevel) );
            return HC_ERROR_NEED_MORE_LEVEL;
        }
        
        pcm->m_curMaze = m_mazes[id-1];
        pcm->m_star = star;
        pcm->init();

        ++enterTimes;
        cdata.setExtraData(char_data_type_daily, char_data_daily_maze_times, enterTimes);

        pcm->Save();
        pcm->SaveMapState();
        pcm->SaveMap();
        
        //日常任务
        dailyTaskMgr::getInstance()->updateDailyTask(cdata,daily_task_maze);
        //act统计
        act_to_tencent(&cdata,act_new_maze,id,star);

        int t = pcm->m_timeout - time(NULL);
        //定时器通知，刷新
        json_spirit::mObject mobj;
        mobj["cmd"] = "mazeTimeout";
        mobj["cid"] = cdata.m_id;
        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(t, 1, mobj,1));
        pcm->m_timer = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return HC_SUCCESS;
}

//查询迷宫地图信息
int mazeMgr::queryMazeMap(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    char_maze* pcm = cm.get();

    //cout<<"queryMazeMap()."<<pcm->m_map_types.size()<<","<<pcm->m_map_results.size()<<endl;
    robj.push_back( Pair("state", pcm->m_state) );
    robj.push_back( Pair("curId", pcm->m_cur_pos) );
    robj.push_back( Pair("range1", pcm->m_move_range) );
    robj.push_back( Pair("range2", pcm->m_view_range) );

    //是否有未完成的
    if (pcm->m_cur_pos > 1 && pcm->m_cur_pos < iMazeSize
        && pcm->m_cur_pos_pending)
    {
        json_spirit::Object pending;
        pending.push_back( Pair("id", pcm->m_cur_pos) );
        pending.push_back( Pair("type", pcm->m_map_types[pcm->m_cur_pos-1]) );
        if (pcm->m_map_types[pcm->m_cur_pos-1] == MAZE_EVENT_GUESS_NUMBER)
        {
            pending.push_back( Pair("times", pcm->extra[1]) );
        }
        else if (MAZE_EVENT_MONSTER == pcm->m_map_types[pcm->m_cur_pos-1])
        {
            pending.push_back( Pair("result", pcm->extra[1]) );
            pending.push_back( Pair("kill", pcm->extra[0]) );
        }
        robj.push_back( Pair("pending", pending) );
    }
    json_spirit::Array list;
    for (int i = 0; i < iMazeSize; ++i)
    {
        int dist = maze_distance(pcm->m_cur_pos, i+1);
        if (pcm->m_map_types[i] == MAZE_EVENT_OUT
            || pcm->m_map_results[i] > 0
            || dist <= pcm->m_move_range
            || maze_inview(pcm->m_cur_pos, i+1, pcm->m_view_range))
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", i+1) );
            obj.push_back( Pair("type", pcm->m_map_types[i]) );
            if ((i+1) == pcm->m_cur_pos && pcm->m_cur_pos_pending)
            {
                obj.push_back( Pair("state", 0) );
            }
            else
            {
                obj.push_back( Pair("state", pcm->m_map_results[i] > 0 ? 1 : 0) );
            }
            obj.push_back( Pair("result", pcm->m_map_results[i]) );
            obj.push_back( Pair("dist", dist) );

            if ((i+1) != pcm->m_cur_pos
                && dist <= pcm->m_move_range
                && (pcm->m_map_results[i] == 0 || pcm->m_map_types[i] != MAZE_EVENT_LONGSTOP))
            {
                obj.push_back( Pair("canM", 1) );
            }
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//查询迷宫事件tips
int mazeMgr::queryMazeEventTips(int type, json_spirit::Object& robj)
{
    if (type == MAZE_EVENT_LONGSTOP)
    {
        type = MAZE_EVENT_MISCHANCE;
    }
    if (type >= 1 && type <= m_events.size())
    {
        base_maze_event* b = m_events[type-1].get();
        robj.push_back( Pair("type", b->type) );
        robj.push_back( Pair("name", b->name) );
        robj.push_back( Pair("effect", b->memo) );
        robj.push_back( Pair("score", b->score) );
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

//查询当前迷宫
int mazeMgr::queryCurMaze(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (cm.get() && cm->m_curMaze.get())
    {
        robj.push_back( Pair("id", cm->m_curMaze->id) );
        robj.push_back( Pair("name", cm->m_curMaze->name) );
        robj.push_back( Pair("star", cm->m_star) );
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

//查询当前迷宫信息
int mazeMgr::queryCurMazeInfo(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (cm.get() && cm->m_curMaze.get())
    {
        char_maze* pcm = cm.get();
        int left_secs = pcm->m_timeout - time(NULL);
        if (left_secs > 0)
        {
            robj.push_back( Pair("countDown", left_secs) );

            //移除超时的buff
            pcm->add_buff(0, 0);
            //buff
            json_spirit::Array buffs;
            std::vector<char_maze_buff>::iterator it = pcm->m_buffs.begin();
            while (it != pcm->m_buffs.end())
            {
                int left = it->remove_time - time(NULL);
                if (left > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("type", it->type) );
                    obj.push_back( Pair("countDown", left) );
                    buffs.push_back(obj);
                    ++it;
                }
                else
                {
                    it = pcm->m_buffs.erase(it);
                }
            }
            robj.push_back( Pair("buffs", buffs) );

            robj.push_back( Pair("left", pcm->m_left_move_count) );
            robj.push_back( Pair("score", pcm->m_score) );
            robj.push_back( Pair("pj", getPingjia(pcm->m_score)) );
            json_spirit::Array chances;
            for (int i = 0; i < 4; ++i)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("id", i+1) );
                obj.push_back( Pair("used", pcm->m_used_times[i]) );
                obj.push_back( Pair("gold", mazeGoldCost(i+1, pcm->m_used_times[i]+1)) );
                chances.push_back(obj);
            }

            robj.push_back( Pair("chances", chances) );
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//查询迷宫队伍信息
int mazeMgr::queryMazeTeam(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    else
    {
        json_spirit::Array list;
        char_maze* pcm = cm.get();
        for (std::list<char_maze_general>::iterator it = pcm->m_generals.begin(); it != pcm->m_generals.end(); ++it)    //队伍里面的武将
        {
            char_maze_general& g = *it;
            json_spirit::Object g_obj;
            g_obj.push_back( Pair("name", g.name) );
            g_obj.push_back( Pair("level", g.level) );
            g_obj.push_back( Pair("quality", g.color) );
            g_obj.push_back( Pair("spic", g.spic) );
            g_obj.push_back( Pair("org_hp", g.m_hp_org) );
            g_obj.push_back( Pair("now_hp", g.m_hp_org-g.m_hp_hurt) );
            list.push_back(g_obj);
        }
        robj.push_back( Pair("general_list", list) );
    }
    return HC_SUCCESS;
}

//迷宫中移动
int mazeMgr::mazeMove(CharData& cdata, int toid, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    return cm->mazeMove(cdata, toid, robj);
}

//迷宫跳过
int mazeMgr::mazeSkip(CharData& cdata, int id, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    return cm->mazeSkip(cdata, id, robj);
}

//迷宫改变事件类型
int mazeMgr::mazeChange(CharData& cdata, int id, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    return cm->mazeChange(cdata, id, robj);
}

int mazeMgr::mazeFull(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    return cm->mazeFull(cdata, robj);
}

//迷宫重置(剩余的)
int mazeMgr::mazeReset(CharData& cdata, json_spirit::Object& robj)
{
    //是否在迷宫中
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (cm.get() && cm->m_curMaze.get())
    {
        //cm->init();
        //重置
        //random_maze(cm->m_points);
        return cm->reset_left(cdata, robj);
    }
    return HC_SUCCESS;
}

//查询迷宫中的boss列表
int mazeMgr::getMazeBossList(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    json_spirit::Array list;
    for (std::list<char_maze_boss>::iterator it = cm->m_bossList.begin(); it != cm->m_bossList.end(); ++it)
    {
        json_spirit::Object b_obj;
        b_obj.push_back( Pair("id", it->id) );
        b_obj.push_back( Pair("killed", it->state != 0 ? 1 : 0) );
        boost::shared_ptr<mazeMonster> pm = getMonsterById(it->id);
        if (pm.get())
        {
            b_obj.push_back( Pair("spic", pm->_spic) );
            b_obj.push_back( Pair("name", pm->_name) );
            b_obj.push_back( Pair("level", pm->_level) );
            b_obj.push_back( Pair("mod", pm->_mod) );
        }
        list.push_back(b_obj);
    }
    robj.push_back( Pair("boss_list", list) );
    return HC_SUCCESS;
}

//击杀boss
int mazeMgr::mazeKillBoss(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get()
        || cm->m_boss_combat_id > 0)
    {
        return HC_ERROR;
    }
    for (std::list<char_maze_boss>::iterator it = cm->m_bossList.begin(); it != cm->m_bossList.end(); ++it)
    {
        if (it->state == 0)
        {
            cdata.m_combat_attribute.maze_inspired(cm->getBuffeffect());
            //攻击
            int ret = HC_SUCCESS;
            Combat* pCombat = createMazeCombat(cdata.m_id, it->id, 0, ret, true);
            if (HC_SUCCESS == ret && pCombat)
            {
                cm->m_boss_combat_id = pCombat->m_combat_id;
                //立即返回战斗双方的信息
                pCombat->setCombatInfo();
                InsertCombat(pCombat);
                std::string sendMsg = "{\"cmd\":\"attack\",\"s\":200,\"getBattleList\":" + pCombat->getCombatInfoText() + "}";
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata.m_name);
                if (account.get())
                {
                    account->Send(sendMsg);
                }
                return HC_SUCCESS_NO_RET;
            }
            return ret;
        }
    }
    return HC_ERROR;
}

//迷宫杀小怪
int mazeMgr::mazeKill(CharData& cdata, int useGold, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    char_maze* pcm = cm.get();
    if (pcm->m_state == 1 || pcm->m_cur_pos < 1 || pcm->m_cur_pos > iMazeSize)
    {
        return HC_ERROR;
    }
    if (pcm->m_cur_pos_pending != 1
        || pcm->m_map_types[pcm->m_cur_pos-1] != MAZE_EVENT_MONSTER
        || pcm->extra[0] != 0)
    {
        return HC_ERROR;
    }

    if (useGold > 0)
    {
        if (cdata.addGold(-20) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, 20, gold_cost_for_maze_kill, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(&cdata,20,gold_cost_for_maze_kill);
#endif
        cdata.NotifyCharData();
        //事件结算
        pcm->m_score += pcm->m_cur_pos_score;
        pcm->m_cur_pos_pending = 0;

        pcm->SaveGenerals();
        pcm->SaveMapState();
        pcm->Save();
        robj.push_back( Pair("useGold", 1) );
        return HC_SUCCESS;
    }
    else
    {
        pcm->extra[0] = 1;
        //进入战斗
        int maze_army_id = pcm->m_curMaze->stars[pcm->m_star-1].normal_mid;
        switch (pcm->extra[1])
        {
            case MAZE_EVENT_RESULT_ELITE_MONSTER:
                maze_army_id = pcm->m_curMaze->stars[pcm->m_star-1].elite_mid;
                break;
            case MAZE_EVENT_RESULT_NORMAL_MONSTER:
                maze_army_id = pcm->m_curMaze->stars[pcm->m_star-1].normal_mid;
                break;
            default:
                break;
        }
        //buff效果
        pcm->updateBuffEffect();
        int ret = HC_SUCCESS;
        Combat* pCombat = createMazeCombat(pcm->m_cid, maze_army_id, 0, ret, false);
        if (HC_SUCCESS == ret && pCombat)
        {
            //立即返回战斗双方的信息
            pCombat->setCombatInfo();
            InsertCombat(pCombat);
            std::string sendMsg = "{\"cmd\":\"attack\",\"s\":200,\"getBattleList\":" + pCombat->getCombatInfoText() + "}";
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata.m_name);
            if (account.get())
            {
                account->Send(sendMsg);
            }
        }
        return HC_SUCCESS;
    }
}

//迷宫小游戏猜拳
int mazeMgr::mazeMora(CharData& cdata, int type, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    char_maze* pcm = cm.get();
    if (pcm->m_state == 1 || pcm->m_cur_pos < 1 || pcm->m_cur_pos > iMazeSize)
    {
        return HC_ERROR;
    }
    if (pcm->m_cur_pos_pending != 1 || pcm->m_map_types[pcm->m_cur_pos-1] != MAZE_EVENT_MORA)
    {
        return HC_ERROR;
    }

    int result = MAZE_EVENT_RESULT_MORA_WIN;

    robj.push_back( Pair("type", type) );

    //0表示用金币必胜
    if (type == 0)
    {
        if (cdata.addGold(-30) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, 30, gold_cost_for_maze_winmora, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(&cdata,30,gold_cost_for_maze_winmora);
#endif
        cdata.NotifyCharData();
        result = MAZE_EVENT_RESULT_MORA_WIN;
        switch (pcm->extra[0])
        {
            //石头
            case 1:
                type = 3;
                break;
            //剪刀
            case 2:
                type = 1;
                break;
            default:
                type = 2;
                break;
        }
    }
    else
    {
        switch (type)
        {
            //石头
            case 1:
                switch (pcm->extra[0])
                {
                    //石头
                    case 1:
                        result = MAZE_EVENT_RESULT_MORA_DRAW;
                        break;
                    //剪刀
                    case 2:
                        result = MAZE_EVENT_RESULT_MORA_WIN;
                        break;
                    default:
                        result = MAZE_EVENT_RESULT_MORA_LOSE;
                        break;
                }
                break;
            //剪刀
            case 2:
                switch (pcm->extra[0])
                {
                    //石头
                    case 1:
                        result = MAZE_EVENT_RESULT_MORA_LOSE;
                        break;
                    //剪刀
                    case 2:
                        result = MAZE_EVENT_RESULT_MORA_DRAW;
                        break;
                    default:
                        result = MAZE_EVENT_RESULT_MORA_WIN;
                        break;
                }
                break;
            default:
                switch (pcm->extra[0])
                {
                    //石头
                    case 1:
                        result = MAZE_EVENT_RESULT_MORA_WIN;
                        break;
                    //剪刀
                    case 2:
                        result = MAZE_EVENT_RESULT_MORA_LOSE;
                        break;
                    default:
                        result = MAZE_EVENT_RESULT_MORA_DRAW;
                        break;
                }
                break;
        }
    }
    //robj.push_back( Pair("result", result) );
    boost::shared_ptr<base_maze_event_result> r = getEventResult(result);
    pcm->add_event_score(r, robj);
    pcm->Save();
    pcm->SaveMapState();
    robj.push_back( Pair("you", type) );
    robj.push_back( Pair("he", pcm->extra[0]) );
    return HC_SUCCESS;
}

//迷宫小游戏猜数字
int mazeMgr::mazeGuessNumber(CharData& cdata, int useGold, int number, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    char_maze* pcm = cm.get();
    if (pcm->m_state == 1 || pcm->m_cur_pos < 1 || pcm->m_cur_pos > iMazeSize)
    {
        return HC_ERROR;
    }
    if (pcm->m_cur_pos_pending != 1 || pcm->m_map_types[pcm->m_cur_pos-1] != MAZE_EVENT_GUESS_NUMBER)
    {
        return HC_ERROR;
    }
    int result = 0;         //0猜中了  1大了 2小了

    //使用金币完成
    if (useGold)
    {
        //扣金币
        if (cdata.addGold(-30) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, 30, gold_cost_for_maze_winguess, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(&cdata,30,gold_cost_for_maze_winguess);
#endif
        cdata.NotifyCharData();
        
        result = 0;
        //设置为一次猜中
        pcm->extra[1] = 1;
    }
    else
    {
        //次数加1
        ++pcm->extra[1];
        if (number > pcm->extra[0])
        {
            result = 1;
        }
        else if (number == pcm->extra[0])
        {
            result = 0;
        }
        else
        {
            result = 2;
        }
        if (result != 0 && pcm->extra[1] >= iMazeGuessNumberTimes)
        {
            result = 3;
        }
    }
    if (result == 0)
    {
        int re = MAZE_EVENT_RESULT_GUESS_NUMBER_5;
        if (pcm->extra[1] <= 6)
        {
            re = MAZE_EVENT_RESULT_GUESS_NUMBER_1;
        }
        else if (pcm->extra[1] <= 8)
        {
            re = MAZE_EVENT_RESULT_GUESS_NUMBER_2;
        }
        else if (pcm->extra[1] <= 10)
        {
            re = MAZE_EVENT_RESULT_GUESS_NUMBER_3;
        }
        else if (pcm->extra[1] <= 15)
        {
            re = MAZE_EVENT_RESULT_GUESS_NUMBER_4;
        }
        boost::shared_ptr<base_maze_event_result> r = getEventResult(re);
        int score = r->score;
        robj.push_back( Pair("double", pcm->m_double_score));
        if (pcm->m_double_score == MAZE_EVENT_RESULT_DOUBLE_SCORE)
        {
            score *= 2;
            pcm->m_double_score = 0;
        }
        else if (pcm->m_double_score == MAZE_EVENT_RESULT_HALF_SCORE)
        {
            score /= 2;
            pcm->m_double_score = 0;
        }
        robj.push_back( Pair("score", score));
        std::string msg = r->msg;
        if (score != r->score)
        {
            msg = r->org_msg;
            str_replace(msg, "$S", LEX_CAST_STR(score));
        }

        str_replace(msg, "$T", LEX_CAST_STR(pcm->extra[1]));
        
        robj.push_back( Pair("emsg", msg));
        robj.push_back( Pair("number", pcm->extra[0]));
        pcm->m_cur_pos_score = score;
        pcm->m_map_results[pcm->m_cur_pos-1] = r->result;
        pcm->m_score += score;
        pcm->m_cur_pos_pending = 0;

        pcm->SaveMapState();
    }
    else if (3 == result)
    {
        pcm->m_cur_pos_pending = 0;
        boost::shared_ptr<base_maze_event_result> r = getEventResult(MAZE_EVENT_RESULT_GUESS_NUMBER_5);
        robj.push_back( Pair("emsg", r->msg));
        robj.push_back( Pair("number", pcm->extra[0]));
        pcm->m_map_results[pcm->m_cur_pos-1] = r->result;

        pcm->SaveMapState();
    }
    pcm->Save();
    robj.push_back( Pair("result", result) );
    robj.push_back( Pair("times", pcm->extra[1]) );
    return HC_SUCCESS;
}    

int mazeMgr::mazeBuy(CharData& cdata, int tid, int count, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    char_maze* pcm = cm.get();
    if (pcm->m_state == 1 || pcm->m_cur_pos < 1 || pcm->m_cur_pos > iMazeSize)
    {
        return HC_ERROR;
    }
    if (pcm->m_cur_pos_pending != 1 || pcm->m_map_types[pcm->m_cur_pos-1] != MAZE_EVENT_DISCOUNT)
    {
        return HC_ERROR;
    }
    int discount = get_item_discount(tid);
    if (discount == 0)
    {
        boost::shared_ptr<base_maze_event_result> r = getEventResult(MAZE_EVENT_RESULT_CANCEL_DISCOUNT);
        pcm->add_event_score(r, robj);
        pcm->SaveMapState();
        pcm->Save();
        return HC_SUCCESS;
    }
    boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(tid);
    if (!bt.get())
    {
        //cout<<"error id"<<endl;
        return HC_ERROR;
    }
    if (bt->gold_to_buy <= 0)
    {
        //cout<<"can not buy "<<tid<<endl;
        return HC_ERROR;
    }

    if (count < 1)
    {
        count = 1;
    }

    if (cdata.m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }

    if (bt->max_size > 0)
    {
        int left = cdata.m_bag.size() - cdata.m_bag.getUsed();
        left *= bt->max_size;
        if (left < count)
        {
            count = left;
        }
    }
    else if (count > 10000)
    {
        count = 10000;
    }

    int gold = count * (discount*bt->gold_to_buy/100);
    if (cdata.addGold(-gold) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }

    //金币消耗统计
    add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, gold, gold_cost_for_treasure+tid, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(&cdata,(discount*bt->gold_to_buy/100),gold_cost_for_buy_daoju,tid,count);
#endif

    //给道具
    int err_code = 0;
    cdata.m_bag.addGem(tid, count,err_code);

    cdata.NotifyCharData();

    json_spirit::Array list;
    boost::shared_ptr<base_maze_event_result> r = getEventResult(MAZE_EVENT_RESULT_BUY_DISCOUNT);
    pcm->add_event_score(r, robj);

    pcm->SaveMapState();
    pcm->Save();
    return HC_SUCCESS;
}

int mazeMgr::mazeLottery(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    char_maze* pcm = cm.get();
    if (pcm->m_state == 1 || pcm->m_cur_pos < 1 || pcm->m_cur_pos > iMazeSize)
    {
        return HC_ERROR;
    }
    if (pcm->m_cur_pos_pending != 1 || pcm->m_map_types[pcm->m_cur_pos-1] != MAZE_EVENT_GEM)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<base_maze_event_result> r = getEventResult(MAZE_EVENT_RESULT_GEM);
    pcm->add_event_score(r, robj);
    pcm->SaveMapState();
    pcm->Save();

    int idx = my_random(0, pcm->m_lottery_list.size()-1);
    //新增buff类道具，抽取概率特殊处理
    if (pcm->m_lottery_list[idx].id == 5001
        || pcm->m_lottery_list[idx].id == 5002
        || pcm->m_lottery_list[idx].id == 5003
        || pcm->m_lottery_list[idx].id == 5004
        || pcm->m_lottery_list[idx].id == 5005)
    {
        idx = my_random(0, pcm->m_lottery_list.size()-1);
    }
    robj.push_back( Pair("pos", idx+1) );

    Item& item = pcm->m_lottery_list[idx];
    giveLoots(&cdata, item, 0, cdata.m_level, 0, NULL, NULL, false, give_maze);
    return HC_SUCCESS;
}

int mazeMgr::mazeAbandon(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_maze> cm = getChar(cdata.m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    if (cm->m_state == 0)
    {
        cm->notifyFail();
    }
    //act统计
    act_to_tencent(&cdata,act_new_maze_abandon,cm->m_curMaze->id,cm->m_star);
    cm->m_curMaze.reset();    
    cm->Clear();
    return HC_SUCCESS;
}

int mazeMgr::mazeQueryBossLoots(CharData& cdata, int id, json_spirit::Object& robj)
{
    boost::shared_ptr<mazeMonster> pm = getMonsterById(id);
    if (pm.get() == NULL)
    {
        return HC_ERROR;
    }
    //掉落列表
    if (pm->m_Item_list.size() > 0)
    {
        json_spirit::Array list;
        std::list<Item>::iterator it_l = pm->m_Item_list.begin();
        while (it_l != pm->m_Item_list.end())
        {
            boost::shared_ptr<json_spirit::Object> p_obj;
            ItemToObj(&(*it_l), p_obj);
            if (p_obj.get())
            {
                list.push_back(*(p_obj.get()));
            }
            ++it_l;
        }
        robj.push_back( Pair("alist", list) );
    }
    return HC_SUCCESS;
}

void mazeMgr::random_maze(std::vector<int>& random_maze)
{
    random_maze = m_maze_template;
    std::random_shuffle ( random_maze.begin(), random_maze.end() );

    /*路口不能堵
    if (random_maze[1] == MAZE_EVENT_MISCHANCE)
    {
        if (random_maze[0] != MAZE_EVENT_MISCHANCE)
        {
            int a = random_maze[0];
            random_maze[0] = random_maze[1];
            random_maze[1] = a;
        }
        else
        {
            for (int i = 2; i < random_maze.size(); ++i)
            {
                if (random_maze[i] != MAZE_EVENT_MISCHANCE)
                {
                    int a = random_maze[i];
                    random_maze[i] = random_maze[1];
                    random_maze[1] = a;
                    break;
                }
            }
        }
    }*/
    random_maze.insert(random_maze.begin(), MAZE_EVENT_START);
    random_maze.push_back(MAZE_EVENT_OUT);
}

boost::shared_ptr<base_maze_event_result> mazeMgr::getEventResult(int result)
{
    assert (result >= 1 && result <= m_results.size());
    return m_results[result-1];
}

boost::shared_ptr<base_maze_event_result> mazeMgr::randomEventResult(int event)
{
    assert (event >= 1 && event <= m_events.size());

    boost::shared_ptr<base_maze_event> e = m_events[event-1];
    return e->random_result();
}

int mazeMgr::combatResult(Combat* pCombat)    //战斗结束
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (combat_maze != pCombat->m_type
        && combat_maze_boss != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_attacker->getCharId());
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<char_maze> cm = getChar(cdata->m_id);
    if (cm.get() && cm->m_curMaze.get())
    {
        cm->combatEnd(pCombat);
    }
    pCombat->AppendResult(pCombat->m_result_obj);

    InsertSaveCombat(pCombat);
    return HC_SUCCESS;
}

void mazeMgr::random_lottery_items(std::vector<Item>& list, int count/* = 3*/)
{
    list.clear();

    std::vector<Item> tlist = m_lottery_items;
    std::random_shuffle ( tlist.begin(), tlist.end() );

    for (std::vector<Item>::iterator it = tlist.begin(); it != tlist.end(); ++it)
    {
        list.push_back(*it);
        if (list.size() >= count)
        {
            break;
        }
    }
}

std::map<int, int>& mazeMgr::get_discount_items()
{
    return m_discount_items;
}

int mazeMgr::get_item_discount(int tid)
{
    if (m_discount_items.find(tid) != m_discount_items.end())
    {
        return m_discount_items[tid];
    }
    return 0;
}

void mazeMgr::getAction(CharData* pc, json_spirit::Array& blist)
{
    if (pc->m_currentStronghold >= iMazeOpenStronghold)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_maze) );
        obj.push_back( Pair("active", 0) );
        int enterTimes = pc->queryExtraData(char_data_type_daily, char_data_daily_maze_times);
        int left = iMazeTimesEveryday - enterTimes;
        boost::shared_ptr<char_maze> cm = getChar(pc->m_id);
        if (cm.get() && cm->m_curMaze.get())
        {
            ++left;
        }
        obj.push_back( Pair("leftNums", left) );
        blist.push_back(obj);
    }
}

boost::shared_ptr<base_maze> mazeMgr::getBaseMaze(int id)
{
    if (id >= 1 && id <= m_mazes.size())
    {
        return m_mazes[id-1];
    }
    boost::shared_ptr<base_maze> empty;
    return empty;
}

//查询迷宫列表 cmd ：queryMazeList
int ProcessQueryMazeList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<mazeMgr>::Instance().getMazeList(*pc, robj);
}

//迷宫难度信息 cmd：queryMazeDetail, id:迷宫id
int ProcessQueryMazeDetail(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    return Singleton<mazeMgr>::Instance().getMazeDetail(id, robj);
}

//进入迷宫 cmd：enterMaze，id：迷宫id，star：星级
int ProcessEnterMaze(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    int star = 1;
    READ_INT_FROM_MOBJ(star,o,"star");
    return Singleton<mazeMgr>::Instance().enterMaze(*pc, id, star, robj);
}

//查询当前迷宫 cmd：queryCurMaze
int ProcessQueryCurMaze(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<mazeMgr>::Instance().queryCurMaze(*pc, robj);
}

//查询迷宫地图信息 cmd：queryMazeMap
int ProcessQueryMazeMap(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<mazeMgr>::Instance().queryMazeMap(*pc, robj);
}

//查询迷宫事件信息 cmd：queryMazeEventTips,type:类别
int ProcessQueryMazeEventTips(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    return Singleton<mazeMgr>::Instance().queryMazeEventTips(type, robj);
}

//查询迷宫状态 cmd：queryCurMazeInfo
int ProcessQueryCurMazeInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<mazeMgr>::Instance().queryCurMazeInfo(*pc, robj);
}

//查询迷宫队伍情况 cmd：queryMazeTeam
int ProcessQueryMazeTeam(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<mazeMgr>::Instance().queryMazeTeam(*pc, robj);    
}

//迷宫移动 cmd：mazeMove，id：目标位置id
int ProcessMazeMove(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    return Singleton<mazeMgr>::Instance().mazeMove(*pc, id, robj);
}

//迷宫恢复 cmd：mazeFull
int ProcessMazeFull(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<mazeMgr>::Instance().mazeFull(*pc, robj);
}

//迷宫跳过 cmd: mazeSkip, id:目标id
int ProcessMazeSkip(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    return Singleton<mazeMgr>::Instance().mazeSkip(*pc, id, robj);
}

//迷宫变更 cmd：mazeChange, id:目标id
int ProcessMazeChange(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    return Singleton<mazeMgr>::Instance().mazeChange(*pc, id, robj);
}

//迷宫重置 cmd：mazeReset
int ProcessMazeReset(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<mazeMgr>::Instance().mazeReset(*pc, robj);
}

//迷宫boss列表 cmd：mazeBossList
int ProcessMazeBossList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<mazeMgr>::Instance().getMazeBossList(*pc, robj);
}

//攻击迷宫boss cmd：mazeKillBoss
int ProcessMazeKillBoss(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<mazeMgr>::Instance().mazeKillBoss(*pc, robj);
}

//迷宫猜数字 cmd：mazeGuessNumber
int ProcessMazeGuessNumber(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int guess = 1;
    READ_INT_FROM_MOBJ(guess,o,"num");
    int useGold = 0;
    READ_INT_FROM_MOBJ(useGold,o,"useGold");
    return Singleton<mazeMgr>::Instance().mazeGuessNumber(*pc, useGold, guess, robj);
}

//迷宫猜拳 cmd：mazeMora
int ProcessMazeMora(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int guess = 1;
    READ_INT_FROM_MOBJ(guess,o,"type");
    return Singleton<mazeMgr>::Instance().mazeMora(*pc, guess, robj);
}

//迷宫购买甩卖商品 cmd：mazeBuy
int ProcessMazeBuy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    
    int tid = 0;
    READ_INT_FROM_MOBJ(tid,o,"id");
    robj.push_back( Pair("id", tid) );
    int count = 1;
    READ_INT_FROM_MOBJ(count,o,"count");
    return Singleton<mazeMgr>::Instance().mazeBuy(*pc, tid, count, robj);
}

//迷宫抽奖 cmd：mazeLottery
int ProcessMazeLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<mazeMgr>::Instance().mazeLottery(*pc, robj);
}

//迷宫查询抽奖物品
int ProcessQueryMazeLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    boost::shared_ptr<char_maze> cm =  Singleton<mazeMgr>::Instance().getChar(pc->m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    char_maze* pcm = cm.get();
    if (pcm->m_cur_pos_pending != 1
        || pcm->m_map_types[pcm->m_cur_pos-1] != MAZE_EVENT_GEM)
    {
        return HC_ERROR;
    }
    json_spirit::Array list;
    for (std::vector<Item>::iterator it = pcm->m_lottery_list.begin();
        it != pcm->m_lottery_list.end();
        ++it)
    {
        Item& item = *it;
        json_spirit::Object obj;
        item.toObj(obj);
        list.push_back(obj);
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//迷宫查询猜数字次数
int ProcessQueryMazeGuessTimes(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    boost::shared_ptr<char_maze> cm =  Singleton<mazeMgr>::Instance().getChar(pc->m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    char_maze* pcm = cm.get();
    if (pcm->m_cur_pos_pending != 1
        || pcm->m_map_types[pcm->m_cur_pos-1] != MAZE_EVENT_GUESS_NUMBER)
    {
        return HC_ERROR;
    }
    robj.push_back( Pair("times", pcm->extra[0]) );
    return HC_SUCCESS;
}

//迷宫查询甩卖商品信息
int ProcessQueryMazeCanBuy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    boost::shared_ptr<char_maze> cm =  Singleton<mazeMgr>::Instance().getChar(pc->m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    char_maze* pcm = cm.get();
    if (pcm->m_cur_pos_pending != 1
        || pcm->m_map_types[pcm->m_cur_pos-1] != MAZE_EVENT_DISCOUNT)
    {
        return HC_ERROR;
    }
    std::map<int, int> d_list =  Singleton<mazeMgr>::Instance().get_discount_items();
    json_spirit::Array list;
    int tmp = 0;
    for (std::map<int, int>::iterator it = d_list.begin(); it != d_list.end(); ++it)
    {
        if (it->first > 0 && (tmp == pcm->extra[0] || tmp == pcm->extra[1]))
        {
            boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(it->first);
            if (bt.get() && bt->gold_to_buy > 0)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("id", bt->id) );
                obj.push_back( Pair("spic", bt->spic) );
                obj.push_back( Pair("name", bt->name) );
                obj.push_back( Pair("quality", bt->quality) );
                obj.push_back( Pair("original", bt->gold_to_buy) );
                obj.push_back( Pair("gold", bt->gold_to_buy * it->second/100) );
                list.push_back(obj);
            }
        }
        ++tmp;
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//迷宫放弃
int ProcessMazeAbandon(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<mazeMgr>::Instance().mazeAbandon(*pc, robj);
}

//迷宫查询当前事件结果
int ProcessMazeQueryCurResult(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    boost::shared_ptr<char_maze> cm = Singleton<mazeMgr>::Instance().getChar(pc->m_id);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    char_maze* pcm = cm.get();
    if (pcm->m_cur_pos <= 1 || pcm->m_cur_pos >= iMazeSize || pcm->m_cur_pos_pending)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<base_maze_event_result> r = pcm->m_handle.getEventResult(pcm->m_map_results[pcm->m_cur_pos-1]);
    std::string msg = r->msg;
    if (pcm->m_cur_pos_score != r->score)
    {
        msg = r->org_msg;
        str_replace(msg, "$S", LEX_CAST_STR(pcm->m_cur_pos_score));
        
    }
    if (r->result >= MAZE_EVENT_RESULT_GUESS_NUMBER_1
        && r->result <= MAZE_EVENT_RESULT_GUESS_NUMBER_4)
    {
        str_replace(msg, "$T", LEX_CAST_STR(pcm->extra[1]));
    }
    robj.push_back( Pair("curId", pcm->m_cur_pos) );
    robj.push_back( Pair("emsg", msg) );
    robj.push_back( Pair("result", r->result) );
    //robj.push_back( Pair("result2", pcm->m_points[pcm->m_cur_pos-1].result) );
    return HC_SUCCESS;
}

//迷宫查询BOSS掉落
int ProcessMazeQueryBossLoots(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return Singleton<mazeMgr>::Instance().mazeQueryBossLoots(*pc, id, robj);
}

//攻击迷宫怪 cmd：mazeKill
int ProcessMazeKill(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int useGold = 0;
    READ_INT_FROM_MOBJ(useGold,o,"useGold");
    return Singleton<mazeMgr>::Instance().mazeKill(*pc, useGold, robj);
}

//迷宫超时了
int ProcessMazeTimeout(json_spirit::mObject& o)
{    
    int cid = 0;
    READ_INT_FROM_MOBJ(cid,o,"cid");
    boost::shared_ptr<char_maze> cm = Singleton<mazeMgr>::Instance().getChar(cid);
    if (NULL == cm.get() || NULL == cm->m_curMaze.get())
    {
        return HC_ERROR;
    }
    char_maze* pcm = cm.get();
    if (pcm->m_timeout <= time(NULL))
    {
        //通知超时失败了
        pcm->notifyFail();
        pcm->m_curMaze.reset();    
        pcm->Clear();
    }
    return HC_SUCCESS;
}

