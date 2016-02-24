#pragma once

#include "boost/smart_ptr/shared_ptr.hpp"
#include <map>
#include <list>
#include "json_spirit.h"
#include <string.h>

struct CharData;

//好友动态消息类型
enum FRIEND_NEWS_ENUM
{
    FRIEND_NEWS_ADD_ME = 1,            //好友加我
    FRIEND_NEWS_I_ADD = 2,            //我加好友
    FRIEND_NEWS_JOIN_CORPS = 3,        //加入军团
    FRIEND_NEWS_INVITE_GUARD_SUCCESS = 4,    //邀请好友护送成功
    FRIEND_NEWS_BE_INVITE_GUARD_SUCCESS = 5//好友邀请我护送成功
};

//仇敌动态消息类型
enum ENEMY_NEWS_ENUM
{
    ENEMY_NEWS_ADD_ENEMY = 1,
    ENEMY_NEWS_ENEMY_GUARD = 2,
    ENEMY_NEWS_ROB_ENEMY_FAIL = 3,
    ENEMY_NEWS_SERVANT_ENEMY_FAIL = 4,
    ENEMY_NEWS_SERVANT_ENEMY_SUCCESS = 5,
    ENEMY_NEWS_ROB_ENEMY_SUCCESS = 6
};

//祝贺类型
enum CONGRATULATION_TYPE
{
    CONGRATULATION_MAKE_EQUIP = 1,
    CONGRATULATION_REBORN_HORSE1 = 2,
    CONGRATULATION_REBORN_HORSE2 = 3,
    CONGRATULATION_REBORN_HORSE3 = 4,
    CONGRATULATION_REBORN_HORSE4 = 5,
    CONGRATULATION_REBORN_HORSE5 = 6,
    CONGRATULATION_LEVEL = 7,
    CONGRATULATION_FRIST_QUALITY1_GENERAL = 8,
    CONGRATULATION_FRIST_QUALITY2_GENERAL = 9,
    CONGRATULATION_FRIST_QUALITY3_GENERAL = 10,
    CONGRATULATION_FRIST_QUALITY4_GENERAL = 11,
    CONGRATULATION_VIP_LEVEL_1 = 12,
    CONGRATULATION_VIP_LEVEL_2 = 13,
    CONGRATULATION_VIP_LEVEL_3 = 14,
    CONGRATULATION_ENHANCE_EQUIPMENT = 15,
    CONGRATULATION_MIFA = 16,
    CONGRATULATION_FRIST_RACE_WIN = 17,
    CONGRATULATION_TOP_RACER = 18,
    CONGRATULATION_PASS_STAGE = 19,
    CONGRATULATION_WASH_1 = 20,
    CONGRATULATION_WASH_2 = 21,
    CONGRATULATION_WASH_3 = 22,
    CONGRATULATION_FIRST_BAOSHI = 23,
    CONGRATULATION_REBORN = 24,
    CONGRATULATION_ELITE = 25,
    CONGRATULATION_MAX
};

class my_enemy
{
public:
    my_enemy()
    {
        enemy_id = 0;
        hate = 0;
        b_getChar = false;
    }
    my_enemy(int id, int h)
    {
        enemy_id = id;
        hate = h;
        b_getChar = false;
    }
    my_enemy(const my_enemy& e)
    {
        b_getChar = e.b_getChar;
        enemy_id = e.enemy_id;
        hate = e.hate;
        m_charData = e.m_charData;
    }
    boost::shared_ptr<CharData> getChar();
    int getId() { return enemy_id; }
    int getHate() { return hate; }
    int addHate(int h) { hate += h; return hate; }
    int setHate(int h) { hate = h; return hate; }
    bool is_real_enemy();
private:
    boost::shared_ptr<CharData> m_charData;
    bool b_getChar;
    int enemy_id;
    int hate;
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
        memset(param, 0, sizeof(int)*4);
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
    int param[4];
    std::string extra;
    std::string from_name;
    std::string msg;
};

//仇敌动态
struct char_enemy_news
{
    char_enemy_news()
    {
        id = 0;
        cid = 0;
        eid = 0;
        type = 0;
        time_stamp = 0;
        ename = "";
        msg = "";
    }
    void gen_msg();
    int id;
    int cid;
    int eid;
    int type;
    time_t time_stamp;
    std::string ename;
    std::string msg;
};

struct char_ralation
{
private:
    bool b_getChar;
    bool b_friend_news_loaded;
    bool b_enemy_news_loaded;
    bool b_congratulations_loaded;
    bool b_recved_congratulations_loaded;
    bool b_enemy_loaded;
    boost::shared_ptr<CharData> m_charData;

    void _add_congratulation(char_congratulation& con);

public:
    char_ralation(int id);
    int m_char_id;

    boost::shared_ptr<CharData> getChar();
    //我的关注
    std::map<int, boost::shared_ptr<char_ralation> > m_my_attentions;

    //我的听众
    std::map<int, boost::shared_ptr<char_ralation> > m_my_listeners;

    //我的互相关注
    std::map<int, boost::shared_ptr<char_ralation> > m_my_friends;

    //我的待审核
    std::map<int, boost::shared_ptr<char_ralation> > m_my_pending_review;

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

    //增加好友动态
    void add_friend_news(int id, int cid, const std::string& name, int type, const std::string& param);

    void try_load_enemy_news();
    //我的仇敌动态
    std::list<char_enemy_news> m_enemy_news;

    void add_enemy_news(int id, int cid, const std::string& name, int type);

    //我的仇敌
    std::map<int, my_enemy> m_my_enemys;

    //谁把我当仇敌
    std::list<int> m_who_hateme;

    void try_load_enemys();

    //加关注
    bool add_attention(boost::shared_ptr<char_ralation> r);

    //加听众
    void add_listener(boost::shared_ptr<char_ralation> r);

    //加互相关注
    void add_friend(boost::shared_ptr<char_ralation> r);

    //加申请
    bool add_application(boost::shared_ptr<char_ralation> r);

    bool is_attention(int id);
    bool is_listener(int id);
    bool is_friend(int id);
    bool is_application(int id);
    bool is_enemy(int id);

    //取消关注
    boost::shared_ptr<char_ralation> remove_attention(int id);
    //移除听众
    boost::shared_ptr<char_ralation> remove_listener(int id);
    //移除申请
    boost::shared_ptr<char_ralation> remove_application(int id);
    //移除互相关注
    boost::shared_ptr<char_ralation> remove_friend(int id);

    //移除仇敌
    bool remove_enemy(int eid);
    //移除谁仇恨我
    bool remove_hate_me(int id);

    void add_hate_me(int id);

    //增加对谁的仇恨,如果变成仇敌返回true
    bool addHate(int eid, int hate);
};

class relationMgr
{
public:
    relationMgr();
    void reload();
    boost::shared_ptr<char_ralation> getRelation(int cid);
    //申请好友
    int submitApplication(int cid, const std::string& name);
    //接受好友申请
    int acceptApplication(int cid, int aid);
    //接受全部好友申请
    int acceptAllApplication(int cid);
    //拒绝好友申请
    int rejectApplication(int cid, int aid);
    //拒绝全部好友申请
    int rejectAllApplication(int cid);
    //移除关注
    int removeAttention(int cid, int fid);
    //移除仇敌
    int removeEnemy(int cid, int eid);
    //好友数量
    int getFriendsCount(int cid);
    //好友列表
    void getFriendsList(int cid, json_spirit::Object& robj);
    //仇敌列表
    void getEnemyList(int cid, json_spirit::Object& robj);
    //申请列表
    void getApplicationList(int cid, json_spirit::Object& robj);
    //祝贺列表
    void getCongratulationList(int cid, int page, int perPage, json_spirit::Object& robj);
    //接受祝贺列表
    void getRecvedCongratulationList(int cid, int page, int perPage, json_spirit::Object& robj);
    //好友动态
    void getFriendNewsList(int cid, int page, int perPage, json_spirit::Object& robj);

    //仇敌动态
    void getEnemyNewsList(int cid, int page, int perPage, json_spirit::Object& robj);

    //是否好友
    bool is_my_friend(int cid1, int cid2);

    //是否我的关注
    bool is_my_attention(int cid1, int cid2);

    //是否我的听众
    bool is_my_listener(int cid1, int cid2);

    //是否我的仇敌
    bool is_my_enemy(int cid1, int cid2);

    //祝贺
    int congratulation(int cid, int con_id, json_spirit::Object& robj);

    void postCongradulation(int cid, int type, int param1, int param2);

    int new_con_id() {return ++m_con_id; }
    int new_recv_con_id() {return ++m_recv_con_id; }
    int new_enemy_news_id() { return ++m_enemy_news_id; }

    void getCongratulationMsg(int ctype, std::string& msg);
    void getCongratulationMsg2(int ctype, std::string& msg);
    void getFriendMsg(int type, std::string& msg);
    void getEnemyMsg(int type, std::string& msg);

    //广播好友动态
    void postFriendInfos(int cid, const std::string& name, int type, int id, const std::string& param);
    //广播仇敌动态
    void postEnemyInfos(int cid, const std::string& name, int eid, const std::string& ename, int type);

    //增加仇恨
    void addHate(int cid, int to, int hate);
private:
    std::map<int, boost::shared_ptr<char_ralation> > m_relations;

    std::vector<std::string> m_base_congratulations;
    std::vector<std::string> m_base_congratulations2;
    std::vector<std::string> m_base_friend_news;
    std::vector<std::string> m_base_enemy_news;

    volatile int m_recv_con_id;//收到的祝贺自增id
    volatile int m_con_id;    // 祝贺自增id
    volatile int m_enemy_news_id;        //仇敌动态自增id
};

