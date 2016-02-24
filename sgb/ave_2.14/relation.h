#pragma once

#include "boost/smart_ptr/shared_ptr.hpp"
#include <map>
#include <list>
#include "json_spirit.h"
#include <string.h>

struct CharData;

//���Ѷ�̬��Ϣ����
enum FRIEND_NEWS_ENUM
{
    FRIEND_NEWS_ADD_ME = 1,            //���Ѽ���
    FRIEND_NEWS_I_ADD = 2,            //�ҼӺ���
    FRIEND_NEWS_JOIN_CORPS = 3,        //�������
    FRIEND_NEWS_INVITE_GUARD_SUCCESS = 4,    //������ѻ��ͳɹ�
    FRIEND_NEWS_BE_INVITE_GUARD_SUCCESS = 5//���������һ��ͳɹ�
};

//��ж�̬��Ϣ����
enum ENEMY_NEWS_ENUM
{
    ENEMY_NEWS_ADD_ENEMY = 1,
    ENEMY_NEWS_ENEMY_GUARD = 2,
    ENEMY_NEWS_ROB_ENEMY_FAIL = 3,
    ENEMY_NEWS_SERVANT_ENEMY_FAIL = 4,
    ENEMY_NEWS_SERVANT_ENEMY_SUCCESS = 5,
    ENEMY_NEWS_ROB_ENEMY_SUCCESS = 6
};

//ף������
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
    int type;    //���� 0�ȴ�ף��   1����ף���� 2�Ѿ�ף��
    int ctype;
    int silver;//ף������
    time_t time_stamp;
    int param[4];
    std::string extra;
    std::string from_name;
    std::string msg;
};

//��ж�̬
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
    //�ҵĹ�ע
    std::map<int, boost::shared_ptr<char_ralation> > m_my_attentions;

    //�ҵ�����
    std::map<int, boost::shared_ptr<char_ralation> > m_my_listeners;

    //�ҵĻ����ע
    std::map<int, boost::shared_ptr<char_ralation> > m_my_friends;

    //�ҵĴ����
    std::map<int, boost::shared_ptr<char_ralation> > m_my_pending_review;

    void try_load_congratulations();

    //�ҵ�ף��
    std::list<char_congratulation> m_my_congratulations;
    int m_unread_congratulations;    //δ��ף��

    void try_load_recved_congratulations();

    //���յ���ף��
    std::list<char_congratulation> m_my_recved_congratulations;
    int m_unread_recved_congratulations;    //δ���յ�ף��

    //�յ�ף��
    void recv_congratulation(int id, int cid, int level, int type, int param);
    //�յ��ȴ�ף��
    void add_congratulation(int id, int cid, int type, int param1, int param2);

    //���Ӻ��Ѷ�̬
    void add_friend_news(int id, int cid, const std::string& name, int type, const std::string& param);

    void try_load_enemy_news();
    //�ҵĳ�ж�̬
    std::list<char_enemy_news> m_enemy_news;

    void add_enemy_news(int id, int cid, const std::string& name, int type);

    //�ҵĳ��
    std::map<int, my_enemy> m_my_enemys;

    //˭���ҵ����
    std::list<int> m_who_hateme;

    void try_load_enemys();

    //�ӹ�ע
    bool add_attention(boost::shared_ptr<char_ralation> r);

    //������
    void add_listener(boost::shared_ptr<char_ralation> r);

    //�ӻ����ע
    void add_friend(boost::shared_ptr<char_ralation> r);

    //������
    bool add_application(boost::shared_ptr<char_ralation> r);

    bool is_attention(int id);
    bool is_listener(int id);
    bool is_friend(int id);
    bool is_application(int id);
    bool is_enemy(int id);

    //ȡ����ע
    boost::shared_ptr<char_ralation> remove_attention(int id);
    //�Ƴ�����
    boost::shared_ptr<char_ralation> remove_listener(int id);
    //�Ƴ�����
    boost::shared_ptr<char_ralation> remove_application(int id);
    //�Ƴ������ע
    boost::shared_ptr<char_ralation> remove_friend(int id);

    //�Ƴ����
    bool remove_enemy(int eid);
    //�Ƴ�˭�����
    bool remove_hate_me(int id);

    void add_hate_me(int id);

    //���Ӷ�˭�ĳ��,�����ɳ�з���true
    bool addHate(int eid, int hate);
};

class relationMgr
{
public:
    relationMgr();
    void reload();
    boost::shared_ptr<char_ralation> getRelation(int cid);
    //�������
    int submitApplication(int cid, const std::string& name);
    //���ܺ�������
    int acceptApplication(int cid, int aid);
    //����ȫ����������
    int acceptAllApplication(int cid);
    //�ܾ���������
    int rejectApplication(int cid, int aid);
    //�ܾ�ȫ����������
    int rejectAllApplication(int cid);
    //�Ƴ���ע
    int removeAttention(int cid, int fid);
    //�Ƴ����
    int removeEnemy(int cid, int eid);
    //��������
    int getFriendsCount(int cid);
    //�����б�
    void getFriendsList(int cid, json_spirit::Object& robj);
    //����б�
    void getEnemyList(int cid, json_spirit::Object& robj);
    //�����б�
    void getApplicationList(int cid, json_spirit::Object& robj);
    //ף���б�
    void getCongratulationList(int cid, int page, int perPage, json_spirit::Object& robj);
    //����ף���б�
    void getRecvedCongratulationList(int cid, int page, int perPage, json_spirit::Object& robj);
    //���Ѷ�̬
    void getFriendNewsList(int cid, int page, int perPage, json_spirit::Object& robj);

    //��ж�̬
    void getEnemyNewsList(int cid, int page, int perPage, json_spirit::Object& robj);

    //�Ƿ����
    bool is_my_friend(int cid1, int cid2);

    //�Ƿ��ҵĹ�ע
    bool is_my_attention(int cid1, int cid2);

    //�Ƿ��ҵ�����
    bool is_my_listener(int cid1, int cid2);

    //�Ƿ��ҵĳ��
    bool is_my_enemy(int cid1, int cid2);

    //ף��
    int congratulation(int cid, int con_id, json_spirit::Object& robj);

    void postCongradulation(int cid, int type, int param1, int param2);

    int new_con_id() {return ++m_con_id; }
    int new_recv_con_id() {return ++m_recv_con_id; }
    int new_enemy_news_id() { return ++m_enemy_news_id; }

    void getCongratulationMsg(int ctype, std::string& msg);
    void getCongratulationMsg2(int ctype, std::string& msg);
    void getFriendMsg(int type, std::string& msg);
    void getEnemyMsg(int type, std::string& msg);

    //�㲥���Ѷ�̬
    void postFriendInfos(int cid, const std::string& name, int type, int id, const std::string& param);
    //�㲥��ж�̬
    void postEnemyInfos(int cid, const std::string& name, int eid, const std::string& ename, int type);

    //���ӳ��
    void addHate(int cid, int to, int hate);
private:
    std::map<int, boost::shared_ptr<char_ralation> > m_relations;

    std::vector<std::string> m_base_congratulations;
    std::vector<std::string> m_base_congratulations2;
    std::vector<std::string> m_base_friend_news;
    std::vector<std::string> m_base_enemy_news;

    volatile int m_recv_con_id;//�յ���ף������id
    volatile int m_con_id;    // ף������id
    volatile int m_enemy_news_id;        //��ж�̬����id
};

