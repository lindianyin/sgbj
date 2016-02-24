#pragma once

#include "data.h"
#include "net.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace net;

struct groupCopy;
struct army_data;

//��������
struct groupCopyArmy
{
    int m_copyId;        //����id
    int m_armyId;         //�ؿ�id
    int m_stateNum;    //״̬����
    int m_level;        //�ȼ�
    int m_spic;            //ͷ��id

    std::string m_name;//������

    //�ؿ��Ŀ���
    combatAttribute m_combat_attribute;

    boost::shared_ptr<StrongholdGeneralData> m_generals[9];

    int load();
};

//���˸�����Ҷ���
struct groupCopyTeam
{
    int _id;            //����id
    int _maxMember;    //�������
    int _bAutoAttack;    //�����Զ�ս��
    boost::shared_ptr<CharData> _memberList[iMaxGroupCopyMembers];    //��Ա�б�
    int _bfriend[iMaxGroupCopyMembers];    //0��ʾ�������1��ʾ����ĺ������

    CharData* leader();                //�ӳ�
    int getMemberPos(int cid);    //��Աλ�� 1��2��3
    int leave(int cid, net::session_ptr& psession);                //��Ա�뿪
    void getMembersDetail(json_spirit::Array& members_list);
    int members();
    int add(boost::shared_ptr<CharData>, int b_friend = 0);
    groupCopy& _copyHandle;

    groupCopyTeam(groupCopy&);
};

//���˸����еĽ�ɫ
struct groupCopyChar
{
    int _cid;            //��ɫid
    int _team_id;        //����id
    net::session_ptr _psession;    //�Ự���
};

//���˸���
struct groupCopy
{
    int _id;    //����id
    int _mapid;//��ͼid
    int _level;//�Ƽ��ȼ�
    int _spic;    //ͼƬ
    int _attackTimes;    //�ɹ�������
    int _maxMember;    //�������
    int _maxTeamId;    //������id
    int _prestige_reward;//������
    int _gongxun_reward;        //��ѫ����
    
    std::string _name;    //����
    std::string _memo;    //����

    std::map<int, boost::shared_ptr<groupCopyTeam> > _teams;        //�����Ķ���
    std::map<int, boost::shared_ptr<groupCopyChar> > _copyChars;    //����Ľ�ɫ

    std::map<int, int> _attackTimesMaps;

    boost::shared_ptr<groupCopyArmy> _armys[iMaxGroupCopyArmys];    //����npc

    int createTeam(boost::shared_ptr<CharData> cdata, groupCopyChar*);    //��������
    int joinTeam(boost::shared_ptr<CharData> cdata, int team_id, net::session_ptr& psession, int b_friend = 0);    //�������
    int leaveTeam(int cid, int team_id, net::session_ptr& psession);    //�뿪����
    int attackCopy(int team_id, net::session_ptr& psession);    //��ʼ����
    int enterCopy(int cid);    //���븱��
    int leaveCopy(int cid);    //�뿪����

    int getLeftTimes(int cid);    //ʣ�๥������

    int loadTeamArmy(int team_id, std::list<army_data*>& alist);    //������Ҷ���Ĳ���
    int loadNpcArmy(int team_id, std::list<army_data*>& dlist);    //����npc����

    int _setTeam(int cid, int team_id, net::session_ptr& psession);

    //��ѯ�����ڽ�ɫ
    groupCopyChar* _getCopyChar(int cid, net::session_ptr&, bool);

    //��ѯ��������
    groupCopyTeam* _getTeam(int team_id);

    //�㲥�������Ƴ�
    int _broadTeamRemove(int team_id);
    //�㲥���½�����
    int _broadTeamAdd(int team_id, const std::string& name);
    //�㲥�����������仯
    int _broadTeamChange(int team_id, int nums1, int nums2);
    //�㲥�������Ա
    int _broadTeamMembers(int team_id, const std::string& msg);
    //�㲥��Ϣ
    int _broadMsg(const std::string& msg);

    //��������
    int _getTeamDetail(int& team_id, int& autoAttack, int cid, json_spirit::Array& member_list, net::session_ptr& psession);
    int load();

    //ÿ�����5������ÿ���˹�������
    int reset();
    int reset(int cid);

    //�ر�
    int close();
};

//���˸�������
class groupCopyMgr
{
public:
    groupCopyMgr();
    ~groupCopyMgr();

    int leaveCopy(int cid, net::session_ptr&);    //�뿪����

    int getCopyList(session_ptr& psession, CharData* pc, int mapid);        //��ѯ�����б�

    groupCopy* getCopy(int copyId);

    int enterCopy(int cid, int copyid, net::session_ptr&);

    int load();
    //ÿ�����5������ÿ���˹�������
    int reset();
    int reset(int cid);

    void getAction(CharData* pc, json_spirit::Array& blist);

    int close();

    int open(int);

    int getAllCopyCanAttackTimes(int cid, int& total_attack_times);

    bool isOpen() {return m_state;}

    //������ڸ���id��team id
    int getCopyIn(int cid, int& team_id);

    static groupCopyMgr* getInstance();
private:
    static groupCopyMgr* m_handle;

    int m_state;    // 1 ������� 0 �ر�
    boost::uuids::uuid _uuid;    //��ʱ��Ψһid

    std::vector<boost::shared_ptr<groupCopy> > _groupCopys;    //���и����б�

    //������ڸ���
    std::map<int, int> _char_in_copys;
    //std::string m_copy_list_msg;
};

//��ѯ�����б� cmd:getMapMultiBoss //���ݵ�ǰ���������ѯ����ID���б�
int ProcessGetGroupCopyList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ���������б� cmd:getMultiBossList //��ö��˸����б�
int ProcessGetGroupCopyTeamList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ���������Ա��Ϣ cmd:getMultiBossMembers //��ö�Ա��Ϣ
int ProcessGetGroupCopyTeamDetail(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getLeftAttackTime //��ѯʣ��ɹ�������
int ProcessGetGroupCopyLeftTimes(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getMultiBossInfo//��ö��˸�����Ϣ
int ProcessGetGroupCopyInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getAutoAttackBoss //��ѯ�Ƿ������Զ�ս��  !!!!!!!!!!!!!!!����Ҫ�޸�!
int ProcessGetGroupCopyAutoAttack(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:changeMultiBoss //�л�����
int ProcessEnterGroupCopy(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:closeMultiBoss//�رո������棬����ע������Ϣ
int ProcessLeaveGroupCopy(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:createMultiBoss //�������˸�������
int ProcessCreateGroupCopyTeam(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:leaveMultiBoss //�뿪���ɢ����
int ProcessLeaveGroupCopyTeam(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:joinMultiBoss //�������
int ProcessJoinGroupCopyTeam(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:attackMultiBoss    //��������
int ProcessAttackGroupCopy(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:setAutoAttackBoss //���������Զ�ս��
int ProcessSetAutoAttack(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:changeMultiBossIndex //�ı��Ա��ţ����µ���һλ����:    id//����id    roleId//��ԱID����:��
int ProcessChangeGroupCopyTeamMemberPos(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:fireMultiBoss //�߳���Ա
int ProcessFireGroupCopyMember(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessGroupCopyZhaoji(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessGroupCopyZhaojiFriend(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

