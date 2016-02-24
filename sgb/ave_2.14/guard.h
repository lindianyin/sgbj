
#ifndef _GUARD_H_
#define _GUARD_H_

#include <vector>
#include <list>
#include <map>
#include <boost/cstdint.hpp>

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "data.h"
#include "combat.h"

struct base_goods
{
    int silver;
    int prestige;
    int supply;
    int need_min;
    std::string name;
    std::string color_name;
};

//生辰纲
struct goods
{
    goods(int cid, int gid)
    {
        m_cid = cid;
        m_gid = gid;
        m_rob_time = 2;
        m_silver = 0;
        m_prestige = 0;
        m_needmin = 0;
        m_start_time = time(NULL);
        m_end_time = 0;
        m_last_rob_cid = 0;
        m_help_cid = 0;
    }
    int m_cid;
    int m_gid;
    int m_needmin;
    int m_rob_time;//可被劫取次数
    int m_silver;
    int m_prestige;
    int m_supply;
    int m_last_rob_cid;//上次成功劫取的玩家
    int m_help_cid;//护送玩家id
    int m_reward_more;    //奖励加成
    time_t m_start_time;//护送开始时间
    time_t m_end_time;//护送结束时间
    int start();
    int finish();

    boost::uuids::uuid _uuid;    //定时器唯一id
};

struct char_goods
{
    int m_cid;
    int m_gid;
    time_t m_cooltime;//冷却结束时间
    int m_robtime;//可劫取次数
    int m_guardtime;//可护送次数
    int m_state;//0初始1护送
    boost::shared_ptr<goods> m_guard_goods;//护送中的生辰纲
    int m_helptime;//帮助好友护送次数
    time_t m_helpwaittime;//等候帮助请求截止时间
    int m_helper_info;//0普通-1请求护送>0表示护送玩家id

    int getCanRobtime();
    int getCanHelptime();
    int getCanGuardtime();
};

struct event_log
{
    int m_atk_id;
    int m_def_id;
    int m_gid;
    int silver;
    int prestige;
    int supply;
};

struct last_week_Rewards
{
    int rank;
    int cid;
    int state;//0未领取1已领取
};

struct robRankRewards
{
    int rank;
    int prestige;
    int treasure_id;
    int treasure_num;
    std::string memo;
};

struct robScore
{
    int cid;
    int score;
    //std::string name;
    //int level;
    //std::string crop;
};

class guardMgr
{
public:
    int reload();
    int save(int cid);
    static guardMgr* getInstance();
    int getinsence(int cid);
    int getoutsence(int cid);
    int getRobScoreList(int cid, json_spirit::Object& robj);
    int getRobRankRewardsList(json_spirit::Object& robj);
    bool getRewardState(int cid, int& rank);
    int getRobRankRewards(int cid, json_spirit::Object& robj);
    int raceAwards();
    void getAction(CharData* pc, json_spirit::Array& blist);
    int getActionMemo(std::string& memo);
    int Start(int m_level, int cid);
    int Finish(int cid);
    int inspire(int cid, json_spirit::Object& robj);
    int getInspire(int cid, json_spirit::Object& robj);
    int clearInspire(int cid);
    int Rob(int atk_id, int def_id, json_spirit::Object& robj);
    int combatResult(Combat* pCombat);
    int RobReward(int atk_id, int def_id, int& silver, int& prestige, int& supply);
    int GuardDone(int cid);
    int GetGuardTaskList(int cid, json_spirit::Object& robj);
    int GetGuardTaskInfo(int cid, int id, json_spirit::Object& robj);
    int BroadRobGoodsEvent();
    int GetRobGoodsEvent(json_spirit::Object& robj);
    int GetGuardGoodsList(CharData* pc, json_spirit::Object& robj);
    int RefreshGoods(CharData* cd, int type, json_spirit::Object& robj);
    int SpeedRobCD(int cid);
    void openGuardEvent(int fac, int last_mins);
    int getGuardEvent();
    //设置每天可以帮忙护送,劫粮，运粮的次数
    void setGuardTimes(int help, int rob, int guard);
    boost::shared_ptr<char_goods> GetCharGoods(int cid);
    boost::shared_ptr<base_goods> GetBaseGoods(int gid);
    int resetAll();

    //删除角色
    int deleteChar(int cid);
    //刷次数，测试用
    int guardtest(int cid);
    //开放生辰纲
    boost::shared_ptr<char_goods> open(int cid);
    
    int GetGuardFriendsList(CharData* pc, json_spirit::Object& robj);
    int ApplyGuardHelp(CharData* pc, int friend_id);
    int ApplyGuardCancel(int cid);
    int AnswerGuardHelp(int cid, int friend_id, int type);
    
private:
    static guardMgr* m_handle;
    std::map<int, boost::shared_ptr<char_goods> > m_char_goods;//玩家护送生辰纲
    std::map<int, boost::shared_ptr<base_goods> > m_base_goods;//基础生辰纲列表
    std::list<boost::shared_ptr<event_log> > m_event_log;//事件记录
    std::map<uint64_t,int> m_uid_list;//储存当前打开着护送界面的玩家列表
    std::map<int, int> m_inspire_map;    //鼓舞列表
    std::list<robScore> m_topten_score;//劫镖积分排行
    int m_topten_min_score;
    std::map<int, int> m_score_maps;//玩家的劫镖积分
    std::vector<robRankRewards> m_base_rewards;    //排名奖励
    std::vector<last_week_Rewards> m_last_rewards;    //排名奖励
    int m_guard_reward_more;    //护送粮饷奖励加成
    time_t m_guard_reward_end_time;
};

#endif

