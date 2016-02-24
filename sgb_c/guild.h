#pragma once

#include <time.h>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include <map>
#include <list>
#include "json_spirit.h"
#include "const_def.h"
#include <boost/uuid/uuid.hpp>
#include "net.h"
#include "item.h"
#include "utils_all.h"
#include "map.h"

using boost::uint64_t;
using namespace std;
using namespace json_spirit;
using namespace boost;

//����ȼ�����(�������ޣ���������)
const int iMaxGuildLevel = 20;
const int iGuildData[iMaxGuildLevel][2] =
{
    {20000, 40},
    {45000, 45},
    {100000, 50},
    {220000, 55},
    {360000, 60},
    {520000, 65},
    {700000, 70},
    {900000, 75},
    {1120000, 80},
    {1360000, 85},
    {1620000, 90},
    {1900000, 95},
    {2200000, 100},
    {2520000, 105},
    {2860000, 110},
    {3220000, 115},
    {3600000, 120},
    {4000000, 125},
    {4420000, 130},
    {4860000, 135}
};
const int iGuildOpenLevel = 16;

enum GUILD_EVENT
{
    GUILD_EVENT_DONATE_GOLD = 1,
    GUILD_EVENT_DONATE_SILVER,
    GUILD_EVENT_NEW_LEADER,
    GUILD_EVENT_NEW_ASS,
    GUILD_EVENT_ADD_MEMBER,
    GUILD_EVENT_LEAVE_MEMBER,
    GUILD_EVENT_FIRE_MEMBER,
};

enum GUILD_ACTION
{
    GUILD_ACTION_SKILL = 1,
    GUILD_ACTION_MOSHEN,
};

enum GUILD_OFFICE
{
    GUILD_OFFICE_NORMAL = 0,
    GUILD_OFFICE_ASS,
    GUILD_OFFICE_LEADER,
};

class CharData;

//��ҹ����������(�˻�����)
struct CharGuildData
{
    boost::shared_ptr<CharData> cdata;
    int m_cid;//��ɫid
    int m_gid;//����id
    int m_offical;//ְλ 1 �᳤2 ���᳤ 0 ��ͨ��Ա
    int m_contribution;//�ۼƹ���
    time_t m_join_time;//����ʱ��

    CharGuildData()
    {
        m_cid = 0;
        m_gid = 0;
        m_offical = 0;
        m_contribution = 0;
        m_join_time = 0;
    }
    int save();
};

//��ҹ����������(�˻ᱣ��)
struct CharGuildForeverData
{
    boost::shared_ptr<CharData> cdata;
    int m_cid;//��ɫid
    int m_gid;//����id
    std::map<int,int> m_guild_box_state;//����ɾͱ�����ȡ���
    int m_guild_skill_level[4];

    CharGuildForeverData()
    {
        m_cid = 0;
        m_gid = 0;
        for (int i = 0; i < 4; ++i)
        {
            m_guild_skill_level[i] = 0;
        }
    }
    void getSkillAdd(int& attack, int& defense, int& magic, int& hp);
    void clearSkill();
    int save();
};

struct GuildApplication
{
    int cid;        //��ɫid
    int gid;        //�������id
    time_t app_time;        //����ʱ��
    std::string message;    //������Ϣ
    boost::shared_ptr<CharData> cdata;
};

struct GuildEvent
{
    time_t inputTime;
    int gid;
    int cid;
    std::string name;
    std::string message;
};

struct Guild
{
public:
    Guild(int id, int level, int exp, const std::string& name,
                const std::string& bulletin, const std::string& qqGroup);
    int load();
    boost::shared_ptr<CharGuildData> addMember(int cid);
    boost::shared_ptr<CharGuildData> getMember(int cid);
    int setOffical(int cid, int offical);
    int setNewLeader(int cid);
    int removeMember(int cid, int type = 0, const std::string& who = "");
    bool haveApplication(int cid);
    int cancelApplication(int cid);
    void addEvent(int cid, const std::string& name, const std::string& name2, int type, int extra = 0);
    void toSimpleObj(json_spirit::Object& obj);
    void toObj(json_spirit::Object& robj);
    void save();
    void saveMoshenData();
    void sort();

    int m_id;//Ψһid
    int m_level;//���ŵȼ�
    int m_rank;//����
    int m_exp;//��ǰ����
    int m_maxExp;//��������
    int m_memberNums;//��Ա��
    int m_memberLimit;//��Ա����
    std::string m_qqGroup;//qqȺ��
    std::string m_name;//������
    std::string m_strBulletin;        //����

    time_t m_createTime; //����ʱ��

    boost::shared_ptr<CharGuildData> m_leader;    //�᳤
    std::list<boost::shared_ptr<CharGuildData> > m_assistant;    //���᳤

    std::map<int, boost::shared_ptr<CharGuildData> > m_members;    //��Ա
    std::list<boost::shared_ptr<CharGuildData> > m_members_list;    //��Ա(�����Զ������������)
    std::map<int, boost::shared_ptr<GuildApplication> > m_applications;        //�����б�

    std::list<boost::shared_ptr<GuildEvent> > m_event_list;    //�¼��б�

    std::vector<int> m_moshen_list;//ħ��ͨ�����
};

struct baseGuildAction
{
    int id;                    //�id
    int needlevel;                //���Ҫ���ŵȼ�
    std::string name;            //�����
    std::string memo;            //�����
};

struct baseGuildBox
{
    int id;//����id
    int type;//1ÿ��2�ɾ�
    int needlevel;//������Ҫ����ȼ�
    int needcontribution;//������Ҫ���ṱ��
    std::list<Item> reward;//����
    void loadReward();
};

struct baseGuildSkill
{
    int type;//1����2����3ħ��4Ѫ��
    int level;//�ȼ�
    int needcontribution;//������Ҫ���ṱ��
    int add;//�ӳ�
};

//����ħ������
struct baseGuildMoshen
{
    int m_id;       //ħ��id
    int m_spic;     //ͼƬ
    int m_silver;   //��ʼ����
    std::string m_name;
    std::string m_chat;
    //����
    std::list<Item> m_Item_list;
    boost::shared_ptr<baseStrongholdHeroData> m_hero;//Ӣ��
    void loadReward();
    void toObj(json_spirit::Object& obj);
};

class guildMgr
{
public:
    guildMgr();
    //���ҹ���
    Guild* getGuild(int gid);
    //��������
    int createGuild(int cid, const std::string& name, json_spirit::Object& robj);
    //�鿴�����Ϣ
    int getApplications(int cid, int gid, int page, int nums_per_page, json_spirit::Object& robj);
    //ͨ�����
    int acceptApplication(int cid, int gid, int tcid);
    //�ܾ����
    int rejectApplication(int cid, int gid, int tcid);
    //�ܾ��������
    int rejectAllApplication(int cid, int gid);
    //��ȡ��������
    std::string getGuildName(int gid);
    //��ȡ����ȼ�
    int getGuildLevel(int gid);
    //��ѯ������Ϣ
    int getGuildInfo(int cid, int gid, json_spirit::Object& robj);
    //��ѯ�����Ա
    int getGuildMembers(int cid, int gid, int page, int nums_per_page, json_spirit::Object& robj);
    //��ѯ������־
    int getGuildEvents(int cid, int gid, json_spirit::Object& robj);
    //�˳�����
    int quitGuild(int cid, int gid);
    //�ύ����
    int submitApplication(int cid, int gid);
    //ȡ������
    int cancelApplication(int cid, int gid);
    //����
    int appointment(int cid, int gid, int tcid, int offical);
    //ÿ�ո���
    void dailyUpdate();
    //������Ա
    int fireMember(int cid, int gid, int tcid);
    //��ɢ����
    int dissolve(int cid, int gid);
    //��ѯ�����б�
    int getGuildList(int cid, int page, int nums_per_page, json_spirit::Object& robj);
    //��������
    int updateRank();
    //����
    int donate(int cid, int gid, int add, int type);
    //���ù�����Ϣ
    int setGuildInfo(int cid, int gid, const std::string& memo, const std::string& qq);
    //��ȡÿ�ձ�����Ϣ
    int getGuildDailyBoxList(int cid, int gid, json_spirit::Object& robj);
    //��ȡ����ÿ�ձ���
    int getGuildDailyBoxReward(int cid, int gid, int boxid, json_spirit::Object& robj);
    //��ѯ����ɾͱ���
    int getGuildBoxList(int cid, int gid, int page, int nums_per_page, json_spirit::Object& robj);
    //��ȡ����ɾͱ���
    int getGuildBoxReward(int cid, int gid, int boxid, json_spirit::Object& robj);
    //��ȡ�������Ἴ��
    boost::shared_ptr<baseGuildSkill> getGuildSkill(int type, int level);
    int getGuildSkillList(int cid, int gid, json_spirit::Object& robj);
    int upgradeGuildSkill(int cid, int gid, int type);

    int getGuildActionLevel(int id);
    int getGuildActionList(CharData* pc, json_spirit::Object& robj);
    void addDonateEvent(int cid, const std::string& name, int extra);

    boost::shared_ptr<baseGuildMoshen> getGuildMoshen(int id);
    int getGuildMoshenInfo(int cid, int gid, int moshen_id, json_spirit::Object& robj);
    int combatResult(chessCombat* pCombat);

    //���þ��ŵȼ�/����
    void setGuildExp(int gid, int level, int exp);
    //������ҹ���
    void setCharContribution(CharData& cdata, int contribute);
private:
    std::map<int, boost::shared_ptr<Guild> > m_guild_maps;
    std::map<const std::string, boost::shared_ptr<Guild> > m_guild_maps2;
    std::list<boost::shared_ptr<Guild> > m_guild_list;
    int max_guild;

    std::list<boost::shared_ptr<GuildEvent> > m_event_list;    //ȫ�������¼��б�

    std::vector<baseGuildAction> m_base_guild_action;
    int max_guild_action;

    std::map<int, boost::shared_ptr<baseGuildBox> > m_guild_dailybox;
    std::map<int, boost::shared_ptr<baseGuildBox> > m_guild_box;

    std::map<std::pair<int,int>, boost::shared_ptr<baseGuildSkill>, compare_pair> m_guild_skills;

    std::vector<boost::shared_ptr<baseGuildMoshen> > m_guild_moshen;    //ħ���б�
};


//�����б�
int ProcessGetGuildList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�������
int ProcessDealJoinGuild(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��������
int ProcessCreateGuild(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������Ϣ
int ProcessGetGuildInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����
int ProcessDonate(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ɢ����
int ProcessDissolveGuild(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�˳�����
int ProcessQuitGuild(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//���ù�����Ϣ
int ProcessSetGuildInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ�����б����
int ProcessGetGuildActionList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ���ᱦ���б�
int ProcessGetGuildBoxList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//���ᱦ����ȡ
int ProcessGetGuildBoxReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����Ա��Ϣ
int ProcessGetGuildMemberList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��Աְλ����
int ProcessDealGuildMember(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��־
int ProcessGetGuildEventList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����б�
int ProcessGetGuildApplyList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��������
int ProcessDealGuildApply(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ���Ἴ���б�
int ProcessGetGuildSkillList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�������Ἴ��
int ProcessUpgradeGuildSkill(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ����ħ����Ϣ
int ProcessGetGuildMoshenInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

