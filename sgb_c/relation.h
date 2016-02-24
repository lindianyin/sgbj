#pragma once

#include "boost/smart_ptr/shared_ptr.hpp"
#include <map>
#include <list>
#include "json_spirit.h"
#include <string.h>
#include "net.h"

struct CharData;

//ף������
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
    int type;    //���� 0�ȴ�ף��   1����ף���� 2�Ѿ�ף��
    int ctype;
    int silver;//ף������
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
    //�ҵĹ�ע
    std::map<int, boost::shared_ptr<char_ralation> > m_my_attentions;
    //�ҵĻ����ע
    std::map<int, boost::shared_ptr<char_ralation> > m_my_friends;
    //�ҵĴ����
    std::map<int, boost::shared_ptr<char_ralation> > m_my_pending_review;
    //�ҵĺ�����
    std::map<int, boost::shared_ptr<char_ralation> > m_my_blacklist;
    //�ҵ��������
    std::map<int, boost::shared_ptr<char_ralation> > m_my_recently;
    //�Ƽ�����
    std::map<int, boost::shared_ptr<CharData> > m_my_recommend;
    time_t m_recommend_refresh;
    //�ӹ�ע
    bool add_attention(boost::shared_ptr<char_ralation> r);
    //�ӻ����ע
    void add_friend(boost::shared_ptr<char_ralation> r);
    //������
    bool add_application(boost::shared_ptr<char_ralation> r);
    //�Ӻ�����
    bool add_blacklist(boost::shared_ptr<char_ralation> r);
    //���������
    void add_recently(boost::shared_ptr<char_ralation> r);

    bool is_attention(int id);
    bool is_friend(int id);
    bool is_application(int id);
    bool is_blacklist(int id);

    //ȡ����ע
    boost::shared_ptr<char_ralation> remove_attention(int id);
    //�Ƴ�����
    boost::shared_ptr<char_ralation> remove_application(int id);
    //�Ƴ������ע
    boost::shared_ptr<char_ralation> remove_friend(int id);
    //�Ƴ�������
    boost::shared_ptr<char_ralation> remove_blacklist(int id);
    //��ȡ�Ƽ�����
    void get_recommend(CharData* cdata);

    /**ף��ϵͳ**/
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
};

class relationMgr
{
public:
    relationMgr();
    void reload();
    boost::shared_ptr<char_ralation> getRelation(int cid);
    //�������
    //���ܺ�������
    int acceptApplication(int cid, int aid);
    //����ȫ����������
    int acceptAllApplication(int cid);
    //�����Ƽ�ȫ������
    int acceptAllRecommendApplication(CharData* cdata);
    //�ܾ���������
    int rejectApplication(int cid, int aid);
    //�ܾ�ȫ����������
    int rejectAllApplication(int cid);
    //�Ƴ���ע
    int removeAttention(int cid, int fid);
    //���������
    int addBlacklist(int cid, int eid);
    //�Ƴ�������
    int removeBlacklist(int cid, int eid);
    //�����������
    int addRecently(int cid, int eid);
    //��������
    int getFriendsCount(int cid);
    //�����б�
    void getFriendsList(int cid, json_spirit::Object& robj);
    //�������б�
    void getBlackList(int cid, json_spirit::Object& robj);
    //�����б�
    void getApplicationList(int cid, json_spirit::Object& robj);
    //��������б�
    void getRecentlyList(int cid, json_spirit::Object& robj);
    //�Ƽ������б�
    void getRecommendList(CharData* cdata, json_spirit::Object& robj);

    //�Ƿ����
    bool is_my_friend(int cid1, int cid2);
    //�Ƿ��ҵĹ�ע
    bool is_my_attention(int cid1, int cid2);
    //�Ƿ��ҵĺ�����
    bool is_my_blacklist(int cid1, int cid2);

    //ף���б�
    void getCongratulationList(int cid, int page, int perPage, json_spirit::Object& robj);
    //����ף���б�
    void getRecvedCongratulationList(int cid, json_spirit::Object& robj);
    //ף��
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

    volatile int m_recv_con_id;//�յ���ף������id
    volatile int m_con_id;    // ף������id
};

int ProcessGetFriendsList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessDealFriends(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ף�غ���
int ProcessCongratulation(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ף���б�
int ProcessGetCongratulations(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//���յ���ף���б�
int ProcessGetRecvedCongratulations(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

