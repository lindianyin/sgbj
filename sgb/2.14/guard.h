
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

//������
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
    int m_rob_time;//�ɱ���ȡ����
    int m_silver;
    int m_prestige;
    int m_supply;
    int m_last_rob_cid;//�ϴγɹ���ȡ�����
    int m_help_cid;//�������id
    int m_reward_more;    //�����ӳ�
    time_t m_start_time;//���Ϳ�ʼʱ��
    time_t m_end_time;//���ͽ���ʱ��
    int start();
    int finish();

    boost::uuids::uuid _uuid;    //��ʱ��Ψһid
};

struct char_goods
{
    int m_cid;
    int m_gid;
    time_t m_cooltime;//��ȴ����ʱ��
    int m_robtime;//�ɽ�ȡ����
    int m_guardtime;//�ɻ��ʹ���
    int m_state;//0��ʼ1����
    boost::shared_ptr<goods> m_guard_goods;//�����е�������
    int m_helptime;//�������ѻ��ʹ���
    time_t m_helpwaittime;//�Ⱥ���������ֹʱ��
    int m_helper_info;//0��ͨ-1������>0��ʾ�������id

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
    int state;//0δ��ȡ1����ȡ
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
    //����ÿ����԰�æ����,�����������Ĵ���
    void setGuardTimes(int help, int rob, int guard);
    boost::shared_ptr<char_goods> GetCharGoods(int cid);
    boost::shared_ptr<base_goods> GetBaseGoods(int gid);
    int resetAll();

    //ɾ����ɫ
    int deleteChar(int cid);
    //ˢ������������
    int guardtest(int cid);
    //����������
    boost::shared_ptr<char_goods> open(int cid);
    
    int GetGuardFriendsList(CharData* pc, json_spirit::Object& robj);
    int ApplyGuardHelp(CharData* pc, int friend_id);
    int ApplyGuardCancel(int cid);
    int AnswerGuardHelp(int cid, int friend_id, int type);
    
private:
    static guardMgr* m_handle;
    std::map<int, boost::shared_ptr<char_goods> > m_char_goods;//��һ���������
    std::map<int, boost::shared_ptr<base_goods> > m_base_goods;//�����������б�
    std::list<boost::shared_ptr<event_log> > m_event_log;//�¼���¼
    std::map<uint64_t,int> m_uid_list;//���浱ǰ���Ż��ͽ��������б�
    std::map<int, int> m_inspire_map;    //�����б�
    std::list<robScore> m_topten_score;//���ڻ�������
    int m_topten_min_score;
    std::map<int, int> m_score_maps;//��ҵĽ��ڻ���
    std::vector<robRankRewards> m_base_rewards;    //��������
    std::vector<last_week_Rewards> m_last_rewards;    //��������
    int m_guard_reward_more;    //�������ý����ӳ�
    time_t m_guard_reward_end_time;
};

#endif

