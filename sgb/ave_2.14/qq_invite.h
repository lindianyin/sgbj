
#ifdef QQ_PLAT

#pragma once

#include <map>
#include <list>
#include <utility>
#include "json_spirit.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "base_item.h"
#include "bag.h"
#include "net.h"
#include <vector>
#include <list>
#include "libao.h"

enum SHARE_EVENT
{
    SHARE_EVENT_FIRST_GENERAL = 1,
    SHARE_EVENT_FIRST_MAKE_EQUIPTMENT,
    SHARE_EVENT_FIRST_ENHANCE_EQUIPTMENT_20_LEVEL,
    SHARE_EVENT_FIRST_BOSS,
    SHARE_EVENT_FIRST_CAMP_RACE,
    SHARE_EVENT_FIRST_AREANA,
    SHARE_EVENT_JOIN_CORPS
};

struct base_invite_reward
{
    int id;
    int count;
    boost::shared_ptr<baseLibao> reward;
};

struct base_invited_reward
{
    int id;
    int count;
    boost::shared_ptr<baseLibao> reward;
};

struct base_close_friend_reward
{
    int id;
    int count;
    int level;
    boost::shared_ptr<baseLibao> reward;
};

struct base_share_event
{
    int id;
    int type;
    int param;

    boost::shared_ptr<baseLibao> reward;

    std::string title;
    std::string content;
};

class inviteMgr;

class char_invite
{
public:
    char_invite(inviteMgr&);
    void init(int c, int r, int inv, int inved);

    void add_invite(int count);

    int add_invited(int count = 1);

    void save_share(int id);

    void add_close_friend(int id, int level);
    void save_close_friend();

    void save_invite();
    void save_invite_get();

    void save_invited();
    void save_invited_get();

    void save_recall();

    void set_recall(int s);

    void save();

    void daily_reset();

    int close_friend_get(int id);
    int share_get(int id);
    int invite_get(int id);
    int invited_get(int id);

    void set_invite_get(int id, int s);
    void set_invited_get(int id, int s);
    void set_share_get(int id, int s);
    void set_close_friend_get(int id, int s);

    void load_invite_get(const std::string& s);
    void load_invited_get(const std::string& s);
    void load_share_get(const std::string& s);
    void load_close_friend_get(const std::string& s);

    int get_close_friend_count() { return m_close_friends.size(); }
    int get_recall() { return m_recall; }
    int get_invite() { return m_invite; }
    int get_invited() { return m_invited; }
    void update_close_friend_get(const base_close_friend_reward& r);

    int getCanGet();

    int getCloseFriendState();

    int getShareState();

    int getInviteState();
    
private:
    int m_cid;
    int m_recall;
    int m_invite;
    int m_invited;

    inviteMgr& m_handle;

    std::vector<int> m_invite_geted;
    std::vector<int> m_invited_geted;
    std::vector<int> m_close_friend_geted;

    std::vector<int> m_share_geted;

    std::map<int, int> m_close_friends;

};

class inviteMgr
{
public:
    inviteMgr();

    void load();

    boost::shared_ptr<char_invite> getChar(int cid);
    //���·����¼�
    void update_event(int cid, int type, int praram);
    //��������
    void add_close_friend(int cid, int friend_id, int level);
    //�����������
    //void add_invite(int cid, int count);
    //�ɹ�����
    void add_invited(int cid, int icid);
    //�ɹ��ٻ�
    void add_recall(int cid, int rcid);

    void get_invite_list(char_invite& c, json_spirit::Array& list1, json_spirit::Array& list2);

    void get_share_list(char_invite& c, json_spirit::Array& list);

    void get_close_friend_list(char_invite& c, json_spirit::Array& list);

    void daily_reset();

    //���ܻ�ý����б�
    void getAwards(json_spirit::Array& list);

    //�����Ʒ
    Item random_award(int& add_notice, int& pos);

    //���Ӽ�¼
    void addLotteryNotice(const std::string& name, Item& item);

    int in_base_invite(int i);

    int in_base_invited(int i);    

    //��ѯ�齱����
    int queryLotteryNotice(json_spirit::Object& robj);

    void fixShareData();

    void getAction(CharData* pc, json_spirit::Array& list);

    int getInviteAward(CharData& cdata, int id, json_spirit::Object& robj);
    int getInvitedAward(CharData& cdata, int id, json_spirit::Object& robj);
    int getCloseFriendAward(CharData& cdata, int id, json_spirit::Object& robj);
    int getRecallAward(CharData& cdata, json_spirit::Object& robj);
    int getShareAward(CharData& cdata, int id, json_spirit::Object& robj);

    int getCloseFriendTo(int cid);

    void close_friend_levelup(int cid, int fid, int level);

    size_t share_size() { return m_base_share_event.size(); }
    size_t close_friend_size() { return m_base_close_friend.size(); }
private:

    std::map<int, boost::shared_ptr<char_invite> > m_chars;

    std::vector<base_invite_reward> m_base_invite;
    std::vector<base_invited_reward> m_base_invited;
    std::vector<base_close_friend_reward> m_base_close_friend;

    std::vector<base_share_event> m_base_share_event;

    //�ٻؽ���
    boost::shared_ptr<baseLibao> m_recall_rewards;

    //�齱
    std::vector<Item> m_awards;
    std::vector<int> m_gailvs;
    std::vector<int> m_need_notice;

    json_spirit::Value m_notices_value;

    std::map<int, int> m_close_friend_to_map;
};

//��ѯ�������� cmd��QueryInvite
int ProcessQueryInvite(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ����������Ϣ cmd��QueryInviteInfo
int ProcessQueryInviteInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ��Ϸ���� cmd��queryShareInfo������ɹ��͸�������
int ProcessQueryShareInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ�����ٻ� cmd��queryRecallInfo
int ProcessQueryRecallInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ���ѽ��� cmd��queryCloseFriendInfo
int ProcessQueryCloseFriendInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//������Ϸ���� cmd��doShare��id����������
int ProcessDoShare(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ����齱 cmd��queryInviteLottery
int ProcessQueryInviteLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��������齱���� cmd��queryInviteLotteryNotice
int ProcessQueryInviteLotteryNotice(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����齱 cmd: inviteLottery
int ProcessInviteLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������� cmd��doInvite��count��������Ѹ���
int ProcessDoInvite(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

#endif

