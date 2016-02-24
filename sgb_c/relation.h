#pragma once

#include "boost/smart_ptr/shared_ptr.hpp"
#include <map>
#include <list>
#include "json_spirit.h"
#include <string.h>
#include "net.h"

struct CharData;

//祝贺类型
enum CONGRATULATION_TYPE
{
    CONGRATULATION_LEVEL = 1,
    CONGRATULATION_SHENLING = 2,
    CONGRATULATION_COPY = 3,
    CONGRATULATION_COPY_PERFECT = 4,
    CONGRATULATION_PASS_MAP = 5,
    CONGRATULATION_UPGRADE_EQUIPMENT = 6,
    CONGRATULATION_COMPOUND_QUALITY3_EQUIPMENT = 7,
    CONGRATULATION_COMPOUND_QUALITY4_EQUIPMENT = 8,
    CONGRATULATION_COMPOUND_QUALITY5_EQUIPMENT = 9,
    CONGRATULATION_COMPOUND_QUALITY4_HERO = 10,
    CONGRATULATION_COMPOUND_QUALITY5_HERO = 11,
    CONGRATULATION_GOLDEN_QUALITY4_HERO = 12,
    CONGRATULATION_GOLDEN_QUALITY5_HERO = 13,
    CONGRATULATION_PACK_QUALITY4_HERO = 14,
    CONGRATULATION_PACK_QUALITY5_HERO = 15,
    CONGRATULATION_EPIC_HERO = 16,
    CONGRATULATION_TOP_ARENA = 17,
    CONGRATULATION_VIP = 18,
    CONGRATULATION_SKILL = 19,
    CONGRATULATION_PRESTIGE1 = 20,
    CONGRATULATION_PRESTIGE2 = 21,
    CONGRATULATION_PRESTIGE3 = 22,
    CONGRATULATION_PRESTIGE4 = 23,
    CONGRATULATION_TREASURE_QUALITY5 = 24,
    CONGRATULATION_TREASURE_QUALITY6 = 25,
    CONGRATULATION_DAILY_SCORE = 26,
    CONGRATULATION_MAX
};

struct char_congratulation
{
    char_congratulation()
    {
        id = 0;
        cid = 0;
        from_cid = 0;
        type = 0;
        ctype = 0;
        silver = 0;
        time_stamp = 0;
        memset(param, 0, sizeof(int)*2);
        extra = "";
        from_name = "";
        msg = "";
    }

    void gen_msg();
    int id;
    int cid;        
    int from_cid;
    int type;    //类型 0等待祝贺   1别人祝贺我 2已经祝贺
    int ctype;
    int silver;//祝贺银币
    time_t time_stamp;
    int param[2];
    std::string extra;
    std::string from_name;
    std::string msg;
};

struct char_ralation
{
private:
    bool b_getChar;
    bool b_congratulations_loaded;
    bool b_recved_congratulations_loaded;
    boost::shared_ptr<CharData> m_charData;

    void _add_congratulation(char_congratulation& con);

public:
    char_ralation(int id);
    int m_char_id;
    boost::shared_ptr<CharData> getChar();
    //我的关注
    std::map<int, boost::shared_ptr<char_ralation> > m_my_attentions;
    //我的互相关注
    std::map<int, boost::shared_ptr<char_ralation> > m_my_friends;
    //我的待审核
    std::map<int, boost::shared_ptr<char_ralation> > m_my_pending_review;
    //我的黑名单
    std::map<int, boost::shared_ptr<char_ralation> > m_my_blacklist;
    //我的最近聊天
    std::map<int, boost::shared_ptr<char_ralation> > m_my_recently;
    //推荐好友
    std::map<int, boost::shared_ptr<CharData> > m_my_recommend;
    time_t m_recommend_refresh;
    //加关注
    bool add_attention(boost::shared_ptr<char_ralation> r);
    //加互相关注
    void add_friend(boost::shared_ptr<char_ralation> r);
    //加申请
    bool add_application(boost::shared_ptr<char_ralation> r);
    //加黑名单
    bool add_blacklist(boost::shared_ptr<char_ralation> r);
    //加最近聊天
    void add_recently(boost::shared_ptr<char_ralation> r);

    bool is_attention(int id);
    bool is_friend(int id);
    bool is_application(int id);
    bool is_blacklist(int id);

    //取消关注
    boost::shared_ptr<char_ralation> remove_attention(int id);
    //移除申请
    boost::shared_ptr<char_ralation> remove_application(int id);
    //移除互相关注
    boost::shared_ptr<char_ralation> remove_friend(int id);
    //移除黑名单
    boost::shared_ptr<char_ralation> remove_blacklist(int id);
    //获取推荐好友
    void get_recommend(CharData* cdata);

    /**祝贺系统**/
    void try_load_congratulations();

    //我的祝贺
    std::list<char_congratulation> m_my_congratulations;
    int m_unread_congratulations;    //未读祝贺

    void try_load_recved_congratulations();

    //我收到的祝贺
    std::list<char_congratulation> m_my_recved_congratulations;
    int m_unread_recved_congratulations;    //未读收到祝贺

    //收到祝贺
    void recv_congratulation(int id, int cid, int level, int type, int param);
    //收到等待祝贺
    void add_congratulation(int id, int cid, int type, int param1, int param2);
};

class relationMgr
{
public:
    relationMgr();
    void reload();
    boost::shared_ptr<char_ralation> getRelation(int cid);
    //申请好友
    //接受好友申请
    int acceptApplication(int cid, int aid);
    //接受全部好友申请
    int acceptAllApplication(int cid);
    //申请推荐全部好友
    int acceptAllRecommendApplication(CharData* cdata);
    //拒绝好友申请
    int rejectApplication(int cid, int aid);
    //拒绝全部好友申请
    int rejectAllApplication(int cid);
    //移除关注
    int removeAttention(int cid, int fid);
    //加入黑名单
    int addBlacklist(int cid, int eid);
    //移除黑名单
    int removeBlacklist(int cid, int eid);
    //加入最近聊天
    int addRecently(int cid, int eid);
    //好友数量
    int getFriendsCount(int cid);
    //好友列表
    void getFriendsList(int cid, json_spirit::Object& robj);
    //黑名单列表
    void getBlackList(int cid, json_spirit::Object& robj);
    //申请列表
    void getApplicationList(int cid, json_spirit::Object& robj);
    //最近聊天列表
    void getRecentlyList(int cid, json_spirit::Object& robj);
    //推荐好友列表
    void getRecommendList(CharData* cdata, json_spirit::Object& robj);

    //是否好友
    bool is_my_friend(int cid1, int cid2);
    //是否我的关注
    bool is_my_attention(int cid1, int cid2);
    //是否我的黑名单
    bool is_my_blacklist(int cid1, int cid2);

    //祝贺列表
    void getCongratulationList(int cid, int page, int perPage, json_spirit::Object& robj);
    //接受祝贺列表
    void getRecvedCongratulationList(int cid, json_spirit::Object& robj);
    //祝贺
    int congratulation(int cid, int con_id, json_spirit::Object& robj);

    void postCongradulation(int cid, int type, int param1, int param2);

    int new_con_id() {return ++m_con_id; }
    int new_recv_con_id() {return ++m_recv_con_id; }

    void getCongratulationMsg(int ctype, std::string& msg);
    void getCongratulationMsg2(int ctype, std::string& msg);

private:
    std::map<int, boost::shared_ptr<char_ralation> > m_relations;
    std::vector<std::string> m_base_congratulations;
    std::vector<std::string> m_base_congratulations2;

    volatile int m_recv_con_id;//收到的祝贺自增id
    volatile int m_con_id;    // 祝贺自增id
};

int ProcessGetFriendsList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessDealFriends(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//祝贺好友
int ProcessCongratulation(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//祝贺列表
int ProcessGetCongratulations(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//接收到的祝贺列表
int ProcessGetRecvedCongratulations(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

