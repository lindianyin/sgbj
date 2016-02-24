#pragma once

#include <time.h>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include <map>
#include <list>
#include "json_spirit.h"
#include "spls_const.h"
#include <boost/uuid/uuid.hpp>
#include "net.h"

using boost::uint64_t;
using namespace std;
using namespace json_spirit;
using namespace boost;

struct corpsLottery;

enum corps_event_enum
{
    corps_event_add_exp = 1,
    corps_event_new_leader,
    corps_event_appointment,
    corps_event_donate_gold,
    corps_event_join,
    corps_event_fire,
    corps_event_leave,
    corps_event_explore,

    corps_jisi_1 = 100,
    corps_jisi_2,
    corps_jisi_3
};

enum corps_action_enum
{
    corps_action_jisi = 1,
    corps_action_sheji,
    corps_action_tansuo,
    corps_action_yanhui,
    corps_action_lottery,
    corps_action_fighting,
    corps_action_jt_boss
};

class CharData;

/********** ����  **************/

struct corps_member
{
    int cid;                    //��ɫid
    int corps;                    //����id
    int offical;                //ְλ 1 ���ų�  2 �����ų� 0 ��ͨ��Ա
    int contribution;            //�ۼƹ���
    int contribution_day;        //�չ���
    time_t join_time;            //����ʱ��
    time_t recruit_time;        //���˹㲥ʱ��
    int ymsj_can_get;            //ԯ������ܻ�õĹ�ѫ
    boost::shared_ptr<CharData> cdata;

    corps_member()
    {
        cid = 0;
        corps = 0;
        offical = 0;
        contribution = 0;
        contribution_day = 0;
        join_time = 0;
        recruit_time = 0;
        ymsj_can_get = 0;
    }
    int save();
};

struct corps_application
{
    int cid;        //��ɫid
    int corps;        //�������id
    time_t app_time;        //����ʱ��
    std::string message;    //������Ϣ
    boost::shared_ptr<CharData> cdata;
};

struct corpsEvent
{
    time_t inputTime;
    int corps;
    int cid;
    int etype;            //�д����޸� char_corps_events��Ľṹ����ʱ���Ż�
    std::string name;
    std::string msg;
};

struct corpsYanhuiPos
{
    int _id;    //�μӵĽ�ɫid
    int _type;    //��� 1 ���� 2 Э��
    int _spic;    //��ɫͷ��
    std::string _name;    //��ɫ��
};

struct base_corps_action
{
    int _id;                    //�id
    int _needlevel;                //���Ҫ���ŵȼ�
    std::string _name;            //�����
    std::string _award_memo;    //���������
    std::string _memo;            //�����
};

struct splsCorps;

//ԯ�����
struct corps_ymsj
{
    //int m_id;
    boost::shared_ptr<corps_member> m_char[2];
    int m_choice[2];
    time_t m_join_time[2];

    splsCorps& m_corps;
    
    int join(int pos, boost::shared_ptr<corps_member> pc, json_spirit::Object& robj);
    int leave(int cid);
    int choose(int cid, int c);

    void _Done();
    int Done();

    int timeout();

    void refresh();

    time_t next_time;    //�´ο���ʱ��

    boost::uuids::uuid m_choose_timer[2];

    boost::uuids::uuid m_done_timer;

    void load(const std::string& data);

    void choose_timer(int pos);

    void win_timer(int pos);

    void Save();

    corps_ymsj(splsCorps& cp);
};

struct splsCorps
{
public:
    splsCorps(int id, int camp, int level, int exp, int exp_week, int memberLimit, int assistantLimit, const std::string& name, const std::string& flag,
                const std::string& bulletin, const std::string& introduction, int createrId, const std::string& qqGroup, const std::string& ymsj_data);
    int load();
    corps_member* addMember(int cid);
    boost::shared_ptr<corps_member> getMember(int cid);
    int setOffical(int cid, int offical);
    int setNewLeader(int cid);
    int cancelApplication(int cid);
    bool haveApplication(int cid);
    int removeMember(int cid, int type = 0, const std::string& who = "");
    void addEvent(int cid, const std::string& name, const std::string& name2, int type);
    void save();
    void sort();
    int jiSi(CharData* pc, int type);        //��Ա���м���
    int JoinYanhui(CharData* pc, int pos);    //�μ����
    int queryYanhuiPerson();                //��ѯ�������
    int inviteSomeone(CharData* pc, int pos);    //����·��
    int checkYanhuiSuccess();

    //ÿ�����ñ���ؕ�I
    void dailyReset();
    friend bool compare_corps(boost::shared_ptr<splsCorps> a, boost::shared_ptr<splsCorps> b);
    friend class corpsMgr;
//private:
    int _id;            //Ψһid
    int _level;            //���ŵȼ�
    int _camp;            //������Ӫ
    int _rank;            //����
    int _maxExp;        //��������
    int _exp;            //��ǰ����
    int _expWeek;        //���ܾ���
    int _creater;        //������
    int _memberNums;    //��Ա��
    int _memberLimit;    //��Ա����
    int _assistantLimit;//������������
    std::string _qqGroup;//qqȺ��
    std::string _name;    //������
    std::string _flag;    //���

    std::string _strBulletin;        //����
    std::string _strIntroduction;    //�������

    time_t _createTime; //����ʱ��
    int m_auto_recruit_msg; //�Զ�������ļ��Ϣ����
    time_t m_auto_recruit_msg_time;

    boost::shared_ptr<corps_member> _leader;    //���ų�

    std::list<boost::shared_ptr<corps_member> > _assistant;    //�����ų�

    std::map<int, boost::shared_ptr<corps_member> > _members;    //��Ա
    std::list<boost::shared_ptr<corps_member> > _members_list;    //��Ա

    std::map<int, boost::shared_ptr<corps_application> > _applications;        //�����б�

    std::list<boost::shared_ptr<corpsEvent> > _event_list;    //�¼��б�
    std::list<boost::shared_ptr<corpsEvent> > _js_event_list;    //�����¼��б�

    corpsYanhuiPos _corps_yanhui[iCorpsYanhuiMax];    //�������

    boost::shared_ptr<corpsLottery> m_corpsLottery;

    std::string m_ymsj_data;
    corps_ymsj m_ymsj;    //����ԯ�����
};

struct baseCorps
{
    int _maxExp;
    int _memberLimit;
    int _assistantLimit;
};

class corpsMgr
{
public:
    //��ʼ��
    int init();
    //���Ż��ť
    void getAction(CharData* pc, json_spirit::Array& blist);
    //��������
    int createCorps(int cid, const std::string& name, json_spirit::Object& robj);
    //�鿴�����Ϣ
    int getApplications(int cid, int corps, int page, int nums_per_page, json_spirit::Object& robj);
    //ͨ�����
    int acceptApplication(int cid, int corps, int tcid, json_spirit::Object& robj);
    //�ܾ����
    int rejectApplication(int cid, int corps, int tcid, json_spirit::Object& robj);
    //�ܾ��������
    int rejectAllApplication(int cid, int corps, json_spirit::Object& robj);
    //��ȡ��������
    std::string getCorpsName(int corps);
    //��ȡ���ŵȼ�
    int getCorpsLevel(int corps);

    //��ѯ������Ϣ
    int getCorpsInfo(int cid, int corps, json_spirit::Object& robj);
    //��ѯ��������Ϣ
    int getCorpsDetail(int cid, int corps, json_spirit::Object& robj);
    //��ѯ���ų�Ա
    int getCorpsMembers(int cid, int corps, json_spirit::Object& robj);
    //��ѯ������־
    int getCorpsEvents(int cid, int corps, json_spirit::Object& robj);
    //�˳�����
    int quitCorps(int cid, int corps, json_spirit::Object& robj);
    //�ύ����
    int submitApplication(int cid, int corps, json_spirit::Object& robj);
    //ȡ������
    int cancelApplication(int cid, int corps, json_spirit::Object& robj);
    //����
    int appointment(int cid, int corps, int tcid, int offical, json_spirit::Object& robj);
    //����ı���ų�
    int changeLeader(int cid, int corps, json_spirit::Object& robj);
    //�������˹���
    int recruitMember(int cid, int corps, json_spirit::Object& robj);
    //������Ա
    int fireMember(int cid, int corps, int tcid, json_spirit::Object& robj);
    //��ɢ����
    int dissolve(int cid, int corps, json_spirit::Object& robj);
    //��ѯ�����ų���Ϣ
    int getAssistants(int cid, int corps, json_spirit::Object& robj);
    //��ѯ�����б�
    int getCorpsList(int cid, int pageNo, int numsPerPage, json_spirit::Object& robj);
    //��������
    int searchCorps(int cid, const std::string& leader, const std::string& corpsName, json_spirit::Object& robj);
    //��ѯ���������
    int getApplicationCorps(int cid, json_spirit::Object& robj);
    //��������
    int updateRank();
    //���Ӿ����¼�
    int addEvent(CharData* pc,int type,int param1,int param2);
    //���ŵȼ���Ӧ�ľ���/��������
    int getBaseCorps(int level, int& exp, int& numsLimit, int& assistLimit);
    //���þ�����Ϣ
    int setCorpsInfo(int cid, int corps, const std::string& memo,
                const std::string& intro, const std::string& qq, json_spirit::Object& robj);

    int getCorpsActionLevel(int id);
    int getCorpsActionList(CharData* pc, json_spirit::Object& robj);

    //ɾ����ɫ
    int deleteChar(int cid);

    //��ѯ������Ϣ
    int getJisiInfo(CharData* pc, json_spirit::Object& robj);

    //����
    int jiSi(CharData* pc, int type);

    //�鿴�����Ϣ
    int getYanhui(CharData* pc, json_spirit::Object& robj);

    //�ټ�
    int zhaoJi(CharData* pc);
    //�������
    int JoinYanhui(CharData* pc, int pos);
    //����·�˼������
    int inviteSomeone(CharData* pc, int pos);

    //�Ƿ������ᣬ����λ��
    int inYanhui(CharData* pc);
    std::string zhaojiMsg() {return m_strZhaoMsg;}

    //���Ҿ���
    splsCorps* findCorps(int corps);

    //���þ��ŵȼ�/����
    void setCorpsExp(int corps, int level, int exp, int weekExp);
    //������ҹ���
    void setCharContribution(CharData& cdata, int contribute);

    //ÿ�����ñ��ܻ�þ���
    void weekReset();
    //ÿ�����ñ���ؕ�I
    void dailyReset();

    //Ϊ���޸������ݣ����¾�����������
    void updateCorpsMemberLimit();
    
    void openJisiEvent(int fac, int last_mins);
    int getJisiFac();

    //�������˾���
    void checkRobotCorps();
    //������������
    void createRobotCorps();
    //���û��������ų�
    void setRobotCorpsLeader(int id, int cid);
    //�����رջ����˾��� 1������0�ر�
    void openRobotCorps(int state);

    std::string randomCorpsName();
    std::string randomCorpsMemo();
    
    static corpsMgr* getInstance();
    
private:
    static corpsMgr* m_handle;
    std::map<int, boost::shared_ptr<splsCorps> > m_corps_maps;

    std::map<const std::string, boost::shared_ptr<splsCorps> > m_corps_maps2;

    std::list<boost::shared_ptr<splsCorps> > m_corps_list;
    int max_corps;

    std::vector<baseCorps> m_base_corps;
    std::string m_strZhaoMsg;
    std::vector<base_corps_action> m_base_corps_action;
    int max_corps_action;

    int m_jisi_event_endtime;
    int m_jisi_event_fac;

    int m_total_robot_corps;
    bool m_open_robot_corps;

    std::vector<std::string> m_robot_corps_name;
    std::vector<std::string> m_robot_corps_memo;
};

//ԯ����ꪼ���
int ProcessCorpsYmsjJoin(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ԯ�����ѡ��
int ProcessCorpsYmsjChoose(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ԯ�������Ϣ
int ProcessCorpsYmsjInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//ԯ�������ȡ����
int ProcessCorpsYmsjAward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//ԯ���������
int ProcessCorpsYmsjRefresh(json_spirit::mObject& o);

//ԯ����ꪳ�ʱʤ��
int ProcessCorpsYmsjWin(json_spirit::mObject& o);

//ԯ�����ѡ��
int ProcessCorpsYmsjChoose(json_spirit::mObject& o );

//�������˾���
int ProcessRobotCorps(json_spirit::mObject& o);

