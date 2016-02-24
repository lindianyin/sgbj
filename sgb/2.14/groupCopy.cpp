
#include "groupCopy.h"

#include "spls_errcode.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "utils_lang.h"
#include "net.h"
#include "army.h"
#include "groupCombat.h"
#include "spls_timer.h"
#include "daily_task.h"
#include "relation.h"
#include "singleton.h"

#include "statistics.h"

using namespace net;

#define INFO(x) //cout<<x

//extern std::string strExp;
//extern std::string strCounts;

extern void InsertSaveDb(const std::string& sql);
extern Database& GetDb();

extern int getSessionChar(session_ptr& psession, CharData* &pc);
extern int getSessionChar(session_ptr& psession, boost::shared_ptr<CharData>& cdata);

extern std::string strGroupCopyBeKicked;
extern std::string strGongxun;
const std::string strNotifyTeamRemove = "{\"cmd\":\"getMultiBossList\",\"type\":1,\"s\":200,\"groupId\":$G}";

const std::string strNotifyTeamAdd = "{\"cmd\":\"getMultiBossList\",\"type\":2,\"s\":200,\"groupId\":$G,\"name\":\"$N\",\"nums1\":1,\"nums2\":$M}";

const std::string strNotifyTeamChange = "{\"cmd\":\"getMultiBossList\",\"type\":3,\"s\":200,\"groupId\":$G,\"nums1\":$1,\"nums2\":$2}";

const std::string strNotifyGroupCopyOpen = "{\"cmd\":\"updateAction\",\"type\":2,\"s\":200,\"active\":1}";
const std::string strNotifyGroupCopyClose = "{\"cmd\":\"updateAction\",\"type\":2,\"s\":200,\"active\":0}";

static std::string g_notify_be_kicked_msg = "";

groupCopyMgr* groupCopyMgr::m_handle = NULL;
groupCopyMgr* groupCopyMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new groupCopyMgr();
        m_handle->load();
    }
    return m_handle;
}

groupCopyMgr::groupCopyMgr()
{
    m_state = 0;
    _uuid = boost::uuids::nil_uuid();
}

int groupCopyMgr::load()
{
    {
        Query q(GetDb());
        q.get_result("select id,copyName,level,mapid,attackTimes,prestige,gongxun from base_group_copy where 1 order by id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            boost::shared_ptr<groupCopy> spCopy;
            groupCopy* pCopy = new groupCopy();
            pCopy->_id = q.getval();
            pCopy->_name = q.getstr();
            pCopy->_level = q.getval();
            pCopy->_mapid = q.getval();
            pCopy->_attackTimes = q.getval();
            pCopy->_prestige_reward = q.getval();
            pCopy->_gongxun_reward = q.getval();
            pCopy->_memo = "";
            pCopy->_maxMember = iMaxGroupCopyMembers;
            pCopy->_maxTeamId = 0;
            pCopy->_spic = pCopy->_id;
            //pCopy->_exp_reward = (pCopy->_id * 300 + 75);
            spCopy.reset(pCopy);
            //���������������� memo
            pCopy->_memo = lootMgr::getInstance()->getGroupCopyLoots(pCopy->_id, ",");

            _groupCopys.push_back(spCopy);
        }
        q.free_result();
    }
    
    //json_spirit::Array copyList;

    std::vector<boost::shared_ptr<groupCopy> >::iterator it = _groupCopys.begin();    //���и����б�
    while (it != _groupCopys.end())
    {
        groupCopy* pCopy = it->get();
        if (pCopy)
        {
            pCopy->load();
            //json_spirit::Object copyObj;
            //copyObj.push_back( Pair("id", pCopy->_id) );
            //copyObj.push_back( Pair("spic", pCopy->_id) );
            //copyObj.push_back( Pair("name", pCopy->_name) );
            //copyList.push_back(copyObj);
        }
        ++it;
    }

    //json_spirit::Object robj;
    //robj.push_back( Pair("cmd", "getMapMultiBoss") );
    //robj.push_back( Pair("s", "200") );
    //robj.push_back( Pair("id", "$D") );
    //robj.push_back( Pair("list", copyList) );
    //m_copy_list_msg = json_spirit::write(robj);

    g_notify_be_kicked_msg = strSystemMsgFormat;
    str_replace(g_notify_be_kicked_msg, "$M", strGroupCopyBeKicked, true);
    str_replace(g_notify_be_kicked_msg, "$T", LEX_CAST_STR(1), true);
    return 0;
}

int groupCopyMgr::getCopyList(session_ptr& psession, CharData* pc, int mapid)     //��ѯ�����б�
{
    bool bMapPassed = false;
    int max_map = 0;
    json_spirit::Array copyList;
    json_spirit::Array copyList2;
    std::vector<boost::shared_ptr<groupCopy> >::iterator it = _groupCopys.begin();    //���и����б�
    while (it != _groupCopys.end())
    {
        groupCopy* pCopy = it->get();
        //ֻ��ʾ��ʣ�๥��������
        if (pCopy && pc->isMapPassed(pCopy->_id))
        {
            if (pCopy->getLeftTimes(pc->m_id))
            {
                json_spirit::Object copyObj;
                copyObj.push_back( Pair("id", pCopy->_id) );
                copyObj.push_back( Pair("spic", pCopy->_id) );
                copyObj.push_back( Pair("name", pCopy->_name) );
                copyList.push_back(copyObj);

                max_map = pCopy->_id;
                if (max_map == mapid)
                {
                    bMapPassed = true;
                }
            }
            else
            {
                copyList2.push_back(pCopy->_id);
            }
        }
        ++it;
    }

    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "getMapMultiBoss") );
    robj.push_back( Pair("s", 200) );
    if (bMapPassed)
    {
        robj.push_back( Pair("id", mapid) );
    }
    else
    {
        robj.push_back( Pair("id", max_map) );
    }
    robj.push_back( Pair("list", copyList) );
    robj.push_back( Pair("list2", copyList2) );

    psession->send(json_spirit::write(robj));    
    return HC_SUCCESS_NO_RET;
}

groupCopy* groupCopyMgr::getCopy(int copyId)
{
    if (copyId >= 1 && copyId <= (int)_groupCopys.size())
    {
        return _groupCopys[copyId-1].get();
    }
    else
    {
        return NULL;
    }
}

int groupCopyMgr::leaveCopy(int cid, net::session_ptr& psession) //�뿪����
{
    for (std::vector<boost::shared_ptr<groupCopy> >::iterator it = _groupCopys.begin(); it != _groupCopys.end(); ++it)
    {
        if (it->get())
        {
            groupCopy* pCopy = it->get();
            groupCopyChar* pGroupChar = pCopy->_getCopyChar(cid, psession, false);
            if (pGroupChar)
            {
                INFO("************** find group copy char *******************"<<endl);
                //ԭ���ж���ģ��뿪����
                if (pGroupChar->_team_id > 0)
                {
                    INFO("************** leave team "<<pGroupChar->_team_id<<" *******************"<<endl);
                    pCopy->leaveTeam(cid, pGroupChar->_team_id, psession);
                }
                pCopy->_copyChars.erase(cid);
            }
        }
    }
    return 0;
}

int groupCopyMgr::enterCopy(int cid, int copyid, net::session_ptr& psession)
{
    for (std::vector<boost::shared_ptr<groupCopy> >::iterator it = _groupCopys.begin(); it != _groupCopys.end(); ++it)
    {
        groupCopy* pCopy = it->get();
        if (pCopy)
        {
            if (pCopy->_id != copyid)
            {
                groupCopyChar* pGroupChar = pCopy->_getCopyChar(cid, psession, false);
                if (pGroupChar)
                {
                    INFO("************** find group copy char *******************"<<endl);
                    //ԭ���ж���ģ��뿪����
                    if (pGroupChar->_team_id > 0)
                    {
                        INFO("************** leave team "<<pGroupChar->_team_id<<" *******************"<<endl);
                        pCopy->leaveTeam(cid, pGroupChar->_team_id, psession);
                    }
                    pCopy->_copyChars.erase(cid);
                }
            }
            else
            {
                (void)pCopy->_getCopyChar(cid, psession, true);
            }
        }
    }
    return 0;
}

//����ÿ���˹�������
int groupCopyMgr::reset()
{
    std::vector<boost::shared_ptr<groupCopy> >::iterator it = _groupCopys.begin();    //���и����б�
    while (it != _groupCopys.end())
    {
        groupCopy* pCopy = it->get();
        if (pCopy)
        {
            pCopy->reset();
        }
        ++it;
    }
    return HC_SUCCESS;
}

//����ĳ�˹�������
int groupCopyMgr::reset(int cid)
{
    std::vector<boost::shared_ptr<groupCopy> >::iterator it = _groupCopys.begin();    //���и����б�
    while (it != _groupCopys.end())
    {
        groupCopy* pCopy = it->get();
        if (pCopy)
        {
            pCopy->reset(cid);
        }
        ++it;
    }
    return HC_SUCCESS;
}

int groupCopyMgr::close()
{
    if (m_state)
    {
        splsTimerMgr::getInstance()->delTimer(_uuid);
        _uuid = boost::uuids::nil_uuid();
        m_state = 0;
        for (std::vector<boost::shared_ptr<groupCopy> >::iterator it = _groupCopys.begin(); it != _groupCopys.end(); ++it)
        {
            if (it->get())
            {
                it->get()->close();
            }
        }
        //�㲥ͼ��ı�
        //GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(strNotifyGroupCopyClose, 999);
    }
    return HC_SUCCESS;
}

int groupCopyMgr::open(int openMins)
{
    if (!m_state)
    {
        m_state = 1;
        //�_���r���ù����Δ�
        //reset();
        //�㲥ͼ��ı�
        //GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(strNotifyGroupCopyOpen, 999);

        if (openMins)
        {
            //���r�P�]����
            json_spirit::mObject mobj;
            mobj["cmd"] = "scheduleEvent";
            mobj["event"] = "closeGroupCopy";

            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(openMins*60, 1,mobj,1));
            _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
        }
    }
    return HC_SUCCESS;
}

int groupCopyMgr::getAllCopyCanAttackTimes(int cid, int& total_attack_times)
{
    int can_attack_times = 0;
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return can_attack_times;
    for (std::vector<boost::shared_ptr<groupCopy> >::iterator it = _groupCopys.begin(); it != _groupCopys.end(); ++it)
    {
        if (it->get() && cdata->isMapPassed((*it)->_id))
        {
            groupCopy* pCopy = it->get();
            can_attack_times += pCopy->getLeftTimes(cid);
            total_attack_times += pCopy->_attackTimes;
        }
    }
    return can_attack_times;
}

/*������ڸ���id��team id
int groupCopyMgr::getCopyIn(int cid, int& team_id)
{
    std::map<int, int>::iterator it = _char_in_copys.find(cid);
    if (it != _char_in_copys.end())
    {
        int copy_id = *it;
        groupCopy* cp = getCopy(copy_id);
        if (NULL == cp)
        {
            team_id = 0;
            return 0;
        }
        session_ptr psession;
        groupCopyChar* pGroupChar = cp->_getCopyChar(cid, psession, false);
        if (pGroupChar)
        {
            team_id = pGroupChar->_team_id;
        }
        else
        {
            team_id = 0;
        }
        return copy_id;
    }
    else
    {
        team_id = 0;
        return 0;
    }
}*/

CharData* groupCopyTeam::leader()             //�ӳ�
{
    return _memberList[0].get();
}

int groupCopyTeam::members()
{
    int counts = 0;
    for (int i = 0; i < iMaxGroupCopyMembers; ++i)
    {
        if (_memberList[i].get())
        {
            ++counts;
        }
    }
    return counts;
}

int groupCopyTeam::add(boost::shared_ptr<CharData> cdata, int b_friend)
{
    for (int i = 0; i < iMaxGroupCopyMembers; ++i)
    {
        if (!_memberList[i].get())
        {
            _memberList[i] = cdata;
            _bfriend[i] = b_friend;
            //cout << "cid = " << cdata->m_id << " join team " << _id << " pos " << i << endl; 
            return i+1;
        }
    }
    return 0;
}

int groupCopyTeam::getMemberPos(int cid)    //��Աλ�� 1��2��3 �����ھ���0
{
    for (int i = 0; i < iMaxGroupCopyMembers; ++i)
    {
        if (_memberList[i].get() && _memberList[i].get()->m_id == cid)
        {
            return i+1;
        }
    }
    return 0;
}

int groupCopyTeam::leave(int cid, net::session_ptr& psession)             //��Ա�뿪
{
    int pos = getMemberPos(cid);
    INFO("************** groupCopyTeam::leave(),cid"<<cid<<",pos"<<pos<<"! *******************"<<endl);
    if (pos == 1)
    {
        INFO("************** groupCopyTeam::leave() is leader *******************"<<endl);
        for (int i = 0; i < iMaxGroupCopyMembers; ++i)
        {
            if (_memberList[i].get())
            {
                _copyHandle._setTeam(_memberList[i].get()->m_id, 0, psession);
                _memberList[i].reset();
            }
        }
    }
    else if (pos > 0)
    {
        INFO("************** groupCopyTeam::leave() is member *******************"<<endl);
        for (int i = 0; i < iMaxGroupCopyMembers; ++i)
        {
            if (_memberList[i].get() && _memberList[i].get()->m_id == cid)
            {
                _memberList[i].reset();
                _copyHandle._setTeam(cid, 0, psession);
                break;
            }
        }
    }
    return members();
}

void groupCopyTeam::getMembersDetail(json_spirit::Array& member_list)
{
    INFO("************** groupCopyTeam::getMembersDetail() members "<<members()<<"*******************"<<endl);
    for (int i = 0; i < iMaxGroupCopyMembers; ++i)
    {
        if (_memberList[i].get())
        {
            CharData* pc = _memberList[i].get();
            if (pc)
            {
                INFO("************** groupCopyTeam::getMembersDetail() members id= "<<pc->m_id<<"*******************"<<endl);
                json_spirit::Object obj;
                obj.push_back( Pair("index", i + 1) );
                obj.push_back( Pair("id", pc->m_id) );
                obj.push_back( Pair("name", pc->m_name) );
                obj.push_back( Pair("level", pc->m_level) );
                obj.push_back( Pair("spic", pc->m_spic) );
                member_list.push_back(obj);
            }
            else
            {
                INFO("************** groupCopyTeam::getMembersDetail() members NULL *******************"<<endl);
            }
        }
    }
}

groupCopyTeam::groupCopyTeam(groupCopy& c)
:_copyHandle(c)
{
}

int groupCopy::load()
{
    {
        Query q(GetDb());
        q.get_result("select army_id,name,level,spic,hit,crit,shipo,parry,resist_hit,resist_crit,resist_shipo,resist_parry from base_group_copy_armys where copy_id=" + LEX_CAST_STR(_id) + " order by army_id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int army_id = q.getval();
            if (army_id >= 1 && army_id <= iMaxGroupCopyArmys)
            {
                groupCopyArmy* pArmy = new groupCopyArmy;
                pArmy->m_armyId = army_id;
                pArmy->m_copyId = _id;
                pArmy->m_name = q.getstr();
                pArmy->m_level = q.getval();
                pArmy->m_spic = q.getval();
                //����hit,crit,shipo,parry,resist_hit,resist_crit,resist_shipo,resist_parry
                pArmy->m_combat_attribute.special_resist(special_attack_dodge, 10 * q.getval());
                pArmy->m_combat_attribute.special_attack(special_attack_baoji, 10 * q.getval());
                pArmy->m_combat_attribute.special_attack(special_attack_shipo, 10 * q.getval());
                pArmy->m_combat_attribute.special_attack(special_attack_parry, 10 * q.getval());
                pArmy->m_combat_attribute.special_attack(special_attack_dodge, 10 * q.getval());
                pArmy->m_combat_attribute.special_resist(special_attack_baoji, 10 * q.getval());
                pArmy->m_combat_attribute.special_resist(special_attack_shipo, 10 * q.getval());
                pArmy->m_combat_attribute.special_resist(special_attack_parry, 10 * q.getval());
                pArmy->m_combat_attribute.enable();

                boost::shared_ptr<groupCopyArmy> spArmy(pArmy);
                _armys[army_id-1] = spArmy;
            }
        }
        q.free_result();

        q.get_result("select cid,count(*) from char_copy_attack where type=1 and copyId=" + LEX_CAST_STR(_id) + " group by cid");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int cid = q.getval();
            int counts = q.getval();
            _attackTimesMaps[cid] = counts;
        }
        q.free_result();
    }
    for (int i = 0; i < iMaxGroupCopyArmys; ++i)
    {
        if (_armys[i].get())
        {
            _armys[i]->load();
        }
    }
    return 0;
}

//����ÿ���˹�������
int groupCopy::reset()
{
    InsertSaveDb("delete from char_copy_attack where type=1 and copyId=" + LEX_CAST_STR(_id));
    _attackTimesMaps.clear();
    return HC_SUCCESS;
}

//����ĳ�˹�������
int groupCopy::reset(int cid)
{
    InsertSaveDb("delete from char_copy_attack where type=1 and copyId=" + LEX_CAST_STR(_id) + " and cid=" +LEX_CAST_STR(cid));
    _attackTimesMaps.erase(cid);
    return HC_SUCCESS;
}

//�رո�����������ж���
int groupCopy::close()
{
    _teams.clear();        //�����Ķ���
    _copyChars.clear();    //����Ľ�ɫ
    return HC_SUCCESS;
}

int groupCopy::createTeam(boost::shared_ptr<CharData> cdata, groupCopyChar* pGroupChar)    //��������
{
    groupCopyTeam* pTeam = new groupCopyTeam(*this);
    boost::shared_ptr<groupCopyTeam> spTeam(pTeam);
    pTeam->_id = ++_maxTeamId;
    pTeam->_maxMember = _maxMember;
    pTeam->_bAutoAttack = 0;
    pTeam->_memberList[0] = cdata;
    pTeam->_bfriend[0] = 0;
    _teams[pTeam->_id] = spTeam;

    pGroupChar->_team_id = pTeam->_id;
    
    //�㲥����������Ϣ
    _broadTeamAdd(pTeam->_id, cdata->m_name);
    //�����ڷ��Ͷ�������
    json_spirit::Array member_list;
    pTeam->getMembersDetail(member_list);
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "getMultiBossMembers") );
    robj.push_back( Pair("s", 200) );
    robj.push_back( Pair("id", _id) );
    robj.push_back( Pair("groupId", pTeam->_id) );
    robj.push_back( Pair("list", member_list) );
    robj.push_back( Pair("type", pTeam->_bAutoAttack) );
    _broadTeamMembers(pTeam->_id, json_spirit::write(robj));
    return pTeam->_id;
}

int groupCopy::joinTeam(boost::shared_ptr<CharData> cdata, int team_id, net::session_ptr& psession, int b_friend)    //�������
{
    if (!cdata.get())
    {
        return -1;
    }
    INFO("************** groupCopy::joinTeam() "<<team_id<<"*******************"<<endl);
    if (team_id > 0)
    {
        groupCopyTeam* pTeam = _getTeam(team_id);
        if (pTeam)
        {
            INFO("************** groupCopy::joinTeam() find team "<<pTeam->_id<<"*******************"<<endl);
            //�Ѿ��ڶ������� ���� �Ѿ���Ա
            if ((int)pTeam->members() >= pTeam->_maxMember)
            {
                INFO("************** groupCopy::joinTeam() full."<<pTeam->_id<<"*******************"<<endl);
                return -1;
            }
            else if (pTeam->getMemberPos(cdata->m_id))
            {
                
                INFO("************** groupCopy::joinTeam() already in cid:"<<cdata->m_id<<",pos:"<<pTeam->getMemberPos(cdata->m_id)<<",team:"<<team_id<<"*******************"<<endl);
            }
            else
            {
                pTeam->add(cdata, b_friend);
                if (b_friend == 0 && psession.get())
                    _setTeam(cdata->m_id, pTeam->_id, psession);
                //�㲥����仯
                _broadTeamChange(pTeam->_id, pTeam->members(), pTeam->_maxMember);
                //��Ա�ڲ��㲥������
                json_spirit::Array member_list;
                pTeam->getMembersDetail(member_list);
                json_spirit::Object robj;
                robj.push_back( Pair("cmd", "getMultiBossMembers") );
                robj.push_back( Pair("s", 200) );
                robj.push_back( Pair("id", _id) );
                robj.push_back( Pair("groupId", pTeam->_id) );
                robj.push_back( Pair("list", member_list) );
                robj.push_back( Pair("type", pTeam->_bAutoAttack) );
                _broadTeamMembers(pTeam->_id, json_spirit::write(robj));
            }
            return pTeam->_id;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        if (0 == team_id && _teams.size() > 0)
        {
            int team_idx = my_random(1, _teams.size());
            int temp = 0;
            for (std::map<int, boost::shared_ptr<groupCopyTeam> >::iterator it = _teams.begin(); it != _teams.end(); ++it)
            {
                ++temp;
                if (temp == team_idx)
                {
                    groupCopyTeam* pTeam = it->second.get();
                    if (pTeam)
                    {
                        //�Ѿ��ڶ������� ���� �Ѿ���Ա
                        if ((int)pTeam->members() >= pTeam->_maxMember)
                        {
                            return -1;
                        }
                        else if (pTeam->getMemberPos(cdata->m_id))
                        {
                            ;
                        }
                        else
                        {
                            pTeam->add(cdata);
                            if (b_friend == 0 && psession.get())
                                _setTeam(cdata->m_id, pTeam->_id, psession);
                            //�㲥����仯
                            _broadTeamChange(pTeam->_id, pTeam->members(), pTeam->_maxMember);
                            //��Ա�ڲ��㲥������
                            json_spirit::Array member_list;
                            pTeam->getMembersDetail(member_list);
                            json_spirit::Object robj;
                            robj.push_back( Pair("cmd", "getMultiBossMembers") );
                            robj.push_back( Pair("s", 200) );
                            robj.push_back( Pair("id", _id) );
                            robj.push_back( Pair("groupId", pTeam->_id) );
                            robj.push_back( Pair("list", member_list) );
                            robj.push_back( Pair("type", pTeam->_bAutoAttack) );
                            _broadTeamMembers(pTeam->_id, json_spirit::write(robj));
                        }
                        return pTeam->_id;
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
        }
        return -1;
    }
}

int groupCopy::leaveTeam(int cid, int team_id, net::session_ptr& psession)    //�뿪����
{
    groupCopyTeam* pTeam = _getTeam(team_id);
    if (pTeam)
    {
        INFO("************** find team *******************"<<endl);
        //����û���ˣ�ɾ������
        if (0 == pTeam->leave(cid, psession))
        {
            INFO("************** delete team *******************"<<endl);
            //�㲥�����Ƴ�
            _broadTeamRemove(team_id);
            _teams.erase(team_id);
        }
        else
        {
            //�㲥����仯
            _broadTeamChange(team_id, pTeam->members(), pTeam->_maxMember);

            //�����ڷ��Ͷ�������
            json_spirit::Array member_list;
            pTeam->getMembersDetail(member_list);
            json_spirit::Object robj;
            robj.push_back( Pair("cmd", "getMultiBossMembers") );
            robj.push_back( Pair("s", 200) );
            robj.push_back( Pair("id", _id) );
            robj.push_back( Pair("groupId", pTeam->_id) );
            robj.push_back( Pair("list", member_list) );
            robj.push_back( Pair("type", pTeam->_bAutoAttack) );
            _broadTeamMembers(pTeam->_id, json_spirit::write(robj));
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

int groupCopy::attackCopy(int team_id, net::session_ptr& psession)    //��ʼ����
{
    INFO("************** groupCopy::attackCopy(),"<<team_id<<" *******************"<<endl);
    groupCopyTeam* pTeam = _getTeam(team_id);
    if (pTeam)
    {
        INFO("************** groupCopy::attackCopy(), find team ! *******************"<<endl);

         if (pTeam->members() < iMinGroupCopyMembers)
        {
            INFO("************** groupCopy::attackCopy(), need more member ! *******************"<<endl);
            return HC_ERROR_GROUP_COPY_NEED_MORE_MEMBER;
        }
        int ret = groupCombatMgr::getInstance()->AttackGroupCopy(team_id, _id, &pTeam->_bfriend[0]);
        if (HC_SUCCESS == ret)
        {
            _broadTeamRemove(team_id);
            for (int i = 0; i < iMaxGroupCopyMembers; ++i)
            {
                if (pTeam->_memberList[i].get())
                {
                    if (pTeam->_bfriend[i] == 0)
                    {
                        //���Ӹ�����������
                        ++_attackTimesMaps[pTeam->_memberList[i].get()->m_id];
                        InsertSaveDb("insert into char_copy_attack (cid,type,copyId,time) values (" + LEX_CAST_STR(pTeam->_memberList[i].get()->m_id)
                            + ",1," + LEX_CAST_STR(_id) + ",unix_timestamp())");
                        _setTeam(pTeam->_memberList[i].get()->m_id, 0, psession);
                        //�ճ�����
                        dailyTaskMgr::getInstance()->updateDailyTask(*(pTeam->_memberList[i]),daily_task_group_copy);
                        //actͳ��
                        act_to_tencent(pTeam->_memberList[i].get(),act_new_group_copy,_id);
                    }
                    pTeam->_memberList[i].reset();
                }
            }
            _teams.erase(team_id);
        }
        return ret;
    }
    else
    {
        return HC_ERROR;
    }
}

int groupCopy::getLeftTimes(int cid)    //ʣ�๥������
{
    int leftTimes = 0;
    std::map<int, int>::iterator it = _attackTimesMaps.find(cid);
    if (it != _attackTimesMaps.end())
    {
        leftTimes = _attackTimes - it->second;
    }
    else
    {
        leftTimes = _attackTimes;
    }
    if (leftTimes < 0)
    {
        leftTimes = 0;
    }
    return leftTimes;
}

int groupCopy::_setTeam(int cid, int team_id, net::session_ptr& psession)
{
    groupCopyChar* pc = _getCopyChar(cid, psession, false);
    if (pc)
    {
        pc->_team_id = team_id;
    }
    return team_id;
}

//��ѯ�����ڽ�ɫ
groupCopyChar* groupCopy::_getCopyChar(int cid, net::session_ptr& psession, bool bCreateIfNotexist = false)
{
    groupCopyChar* pGroupCopyChar = NULL;
    std::map<int, boost::shared_ptr<groupCopyChar> >::iterator it = _copyChars.find(cid);    //����Ľ�ɫ
    if (it != _copyChars.end())
    {
        pGroupCopyChar = it->second.get();
    }
    if (!pGroupCopyChar && bCreateIfNotexist && psession.get())
    {
        pGroupCopyChar = new groupCopyChar;
        boost::shared_ptr<groupCopyChar> spGroupChar(pGroupCopyChar);
        pGroupCopyChar->_cid = cid;
        pGroupCopyChar->_team_id = 0;
        pGroupCopyChar->_psession = psession;
        _copyChars[cid] = spGroupChar;
        INFO("************** create group copy char,cid "<<cid<<",copy id "<<_id<<"*******************"<<endl);
    }
    return pGroupCopyChar;
}

//��ѯ��������
groupCopyTeam* groupCopy::_getTeam(int team_id)
{
    std::map<int, boost::shared_ptr<groupCopyTeam> >::iterator it = _teams.find(team_id);
    if (it != _teams.end())
    {
        return it->second.get();
    }
    else
    {
        return NULL;
    }
}

int groupCopy::loadTeamArmy(int team_id, std::list<army_data*>& alist)    //������Ҷ���Ĳ���
{
    INFO("************** loadTeamArmy() *******************"<<endl);
    groupCopyTeam* pTeam = _getTeam(team_id);
    if (pTeam)
    {
        INFO("************** loadTeamArmy() find team *******************"<<endl);
        for (int i = 0; i < iMaxGroupCopyMembers; ++i)
        {
            if (pTeam->_memberList[i].get())
            {
                INFO("************** loadTeamArmy() load char "<<pTeam->_memberList[i].get()->m_id<<"*******************"<<endl);
                army_data* pArmy_data_a = new army_data;
                pArmy_data_a->LoadCharactor(pTeam->_memberList[i].get()->m_id);
                alist.push_back(pArmy_data_a);
            }
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int groupCopy::loadNpcArmy(int team_id, std::list<army_data*>& dlist)    //����npc����
{
    INFO("************** loadNpcArmy() *******************"<<endl);
    for (int i = 0; i < iMaxGroupCopyArmys; ++i)
    {
        if (_armys[i].get())
        {
            INFO("************** loadNpcArmy() "<<(i+1)<<" *******************"<<endl);
            army_data* pArmy_data_a = new army_data;
            pArmy_data_a->loadGroupCopy(_id, team_id, _armys[i].get());
            dlist.push_back(pArmy_data_a);
        }        
    }
    return HC_SUCCESS;
}

//�㲥�������Ƴ�
int groupCopy::_broadTeamRemove(int team_id)
{
    INFO("************** groupCopy::_broadTeamRemove() "<<team_id<<" *******************"<<endl);
    std::string msg = strNotifyTeamRemove;
    str_replace(msg, "$G", LEX_CAST_STR(team_id));
    return _broadMsg(msg);
}

//�㲥���½�����
int groupCopy::_broadTeamAdd(int team_id, const std::string& name)
{
    INFO("************** groupCopy::_broadTeamAdd() "<<team_id<<" *******************"<<endl);
    std::string msg = strNotifyTeamAdd;
    str_replace(msg, "$G", LEX_CAST_STR(team_id));
    str_replace(msg, "$N", name);
    str_replace(msg, "$M", LEX_CAST_STR(_maxMember));
    return _broadMsg(msg);
}

//�㲥�����������仯
int groupCopy::_broadTeamChange(int team_id, int nums1, int nums2)
{
    INFO("************** groupCopy::_broadTeamChange() "<<team_id<<" *******************"<<endl);
    std::string msg = strNotifyTeamChange;
    str_replace(msg, "$G", LEX_CAST_STR(team_id));
    str_replace(msg, "$1", LEX_CAST_STR(nums1));
    str_replace(msg, "$2", LEX_CAST_STR(nums2));

    return _broadMsg(msg);
}

//�㲥�������Ա
int groupCopy::_broadTeamMembers(int team_id, const std::string& msg)
{
    int counts = 0;
    for (std::map<int, boost::shared_ptr<groupCopyChar> >::iterator it = _copyChars.begin(); it != _copyChars.end(); ++it)    //����Ľ�ɫ
    {
        groupCopyChar* pChar = it->second.get();
        if (pChar && pChar->_team_id == team_id && pChar->_psession.get())
        {
            ++counts;
            pChar->_psession->send(msg);
        }
    }
    INFO("************** groupCopy::_broadTeamMembers() "<<team_id<<","<<counts<<" *******************"<<endl);
    return counts;
}

//�㲥��Ϣ
int groupCopy::_broadMsg(const std::string& msg)
{
    int counts = 0;
    for (std::map<int, boost::shared_ptr<groupCopyChar> >::iterator it = _copyChars.begin(); it != _copyChars.end(); ++it)    //����Ľ�ɫ
    {
        groupCopyChar* pChar = it->second.get();
        if (pChar && pChar->_psession.get())
        {
            ++counts;
            pChar->_psession->send(msg);
        }
    }
    INFO("************** groupCopy::_broadMsg() "<<counts<<" *******************"<<endl);
    return counts;
}

//��������
int groupCopy::_getTeamDetail(int& groupid, int& autoAttack, int cid, json_spirit::Array& member_list, net::session_ptr& psession)
{
    if (0 == groupid)
    {
        groupCopyChar* pCopyChar = _getCopyChar(cid, psession, false);
        if (pCopyChar)
        {
            groupid = pCopyChar->_team_id;
        }
    }
    groupCopyTeam* pTeam = _getTeam(groupid);
    if (!pTeam)
    {
        return HC_ERROR_NO_GROUP_COPY_TEAM;
    }
    autoAttack = pTeam->_bAutoAttack;
    pTeam->getMembersDetail(member_list);
    return HC_SUCCESS;
}

int groupCopyArmy::load()
{
    Query q(GetDb());
    q.get_result("select pos,name,stype,hp,attack,pufang,cefang,str,wisdom from base_group_copy_armys_generals where copy_id=" + LEX_CAST_STR(m_copyId) + " and army_id=" + LEX_CAST_STR(m_armyId)+ " order by pos");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int pos = q.getval();
        if (pos >= 9)
        {
            pos = 9;
        }
        else if (pos < 1)
        {
            pos = 1;
        }
        boost::shared_ptr<StrongholdGeneralData> sg;
        if (!(m_generals[pos-1].get()))
        {
            sg.reset(new (StrongholdGeneralData));
            m_generals[pos-1] = sg;
        }
        else
        {
            sg = m_generals[pos-1];
        }
        sg->m_pos = pos;
        sg->m_name = q.getstr();
        sg->m_spic = 1;
        sg->m_stype = q.getval();
        sg->m_hp = q.getval();
        sg->m_attack = q.getval();
        sg->m_pufang = q.getval();
        sg->m_cefang = q.getval();
        sg->m_str = q.getval();
        sg->m_int = q.getval();
        sg->m_level = m_level;

        sg->m_baseSoldier = GeneralDataMgr::getInstance()->GetBaseSoldier(sg->m_stype);
    }
    q.free_result();
    return 0;
}


//��ѯ�����б� cmd:getMapMultiBoss //���ݵ�ǰ���������ѯ����ID���б�
int ProcessGetGroupCopyList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (!groupCopyMgr::getInstance()->isOpen())
    {
        return HC_ERROR_GROUP_COPY_NOT_OPEN;
    }
    int mapid = 0;
    READ_INT_FROM_MOBJ(mapid,o,"mapId");
    return groupCopyMgr::getInstance()->getCopyList(psession, cdata, mapid);
}

//��ѯ���������б� cmd:getMultiBossList //��ö��˸����б�
int ProcessGetGroupCopyTeamList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    json_spirit::Array team_list;
    std::map<int, boost::shared_ptr<groupCopyTeam> >::iterator it = pCopy->_teams.begin();        //�����Ķ���
    while (it != pCopy->_teams.end())
    {
        groupCopyTeam* pTeam = it->second.get();
        if (pTeam)
        {            
            CharData* pLeader = pTeam->leader();
            if (pLeader)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("groupId", pTeam->_id) );
                obj.push_back( Pair("name", pLeader->m_name) );
                obj.push_back( Pair("state", pTeam->getMemberPos(cdata->m_id) ? 1 : 0) );
                obj.push_back( Pair("nums1", pTeam->members()) );
                obj.push_back( Pair("nums2", pTeam->_maxMember) );
                team_list.push_back(obj);
            }
        }
        ++it;
    }
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("list", team_list) );
    return HC_SUCCESS;
}

//��ѯ���������Ա��Ϣ cmd:getMultiBossMembers //��ö�Ա��Ϣ
int ProcessGetGroupCopyTeamDetail(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    INFO("************** ProcessGetGroupCopyTeamDetail() *******************"<<endl);
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    int groupid = 0;
    READ_INT_FROM_MOBJ(groupid,o,"groupId");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    int type = 2;
    json_spirit::Array member_list;
    ret = pCopy->_getTeamDetail(groupid, type, cdata->m_id, member_list, psession);
    if (ret != HC_SUCCESS)
    {
        return ret;
    }
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("groupId", groupid) );
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("list", member_list) );
    return HC_SUCCESS;
}

//cmd:getLeftAttackTime //��ѯʣ��ɹ�������
int ProcessGetGroupCopyLeftTimes(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    int leftTimes = pCopy->getLeftTimes(cdata->m_id);
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("time", leftTimes) );
    return HC_SUCCESS;
}

//cmd:getMultiBossInfo//��ö��˸�����Ϣ
int ProcessGetGroupCopyInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    //��ѯ������Ϣʱ���븱��
    //groupCopyMgr::getInstance()->enterCopy(cdata->m_id, id, psession);
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("level", pCopy->_level) );
    robj.push_back( Pair("spic", pCopy->_spic) );
    std::string reward = "";
    if (pCopy->_prestige_reward > 0)
    {
        reward = LEX_CAST_STR(pCopy->_prestige_reward) + strPrestige;
    }
    if (pCopy->_gongxun_reward > 0)
    {
        if (pCopy->_prestige_reward > 0)
        {
            reward += ",";
        }
        reward += LEX_CAST_STR(pCopy->_gongxun_reward) + strGongxun; 
    }
    robj.push_back( Pair("reward", reward) );
    robj.push_back( Pair("drop", pCopy->_memo) );
    return HC_SUCCESS;
}

//cmd:getAutoAttackBoss //��ѯ�Ƿ������Զ�ս��  !!!!!!!!!!!!!!!����Ҫ�޸�!
int ProcessGetGroupCopyAutoAttack(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    robj.push_back( Pair("id", id) );
    //robj.push_back( Pair("needLevel", pCopy->_needLev) );
    robj.push_back( Pair("spic", pCopy->_spic) );
    robj.push_back( Pair("drop", pCopy->_memo) );
    return HC_SUCCESS;
}

//cmd:changeMultiBoss //�л�����
int ProcessEnterGroupCopy(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    INFO("************** ProcessEnterGroupCopy() *******************"<<endl);
    
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    if (!groupCopyMgr::getInstance()->isOpen())
    {
        return HC_ERROR_GROUP_COPY_NOT_OPEN;
    }
    //��ͼδͨ�أ��޷���������
    if (!cdata->isMapPassed(id))
    {
        return HC_ERROR_GROUP_COPY_CAN_NOT_ATTACK;
    }
    groupCopyMgr::getInstance()->enterCopy(cdata->m_id, id, psession);
    robj.push_back( Pair("id", id) );
    return HC_SUCCESS;
}

//cmd:closeMultiBoss//�رո������棬����ע������Ϣ
int ProcessLeaveGroupCopy(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    INFO("************** ProcessLeaveGroupCopy *******************"<<endl);
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    if (0 == id)
    {
        return HC_SUCCESS;
    }
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }

    groupCopyChar* pGroupChar = pCopy->_getCopyChar(cdata->m_id, psession, false);
    if (pGroupChar)
    {
        INFO("************** find group copy char *******************"<<endl);
        //ԭ���ж���ģ��뿪����
        if (pGroupChar->_team_id > 0)
        {
            INFO("************** leave team "<<pGroupChar->_team_id<<" *******************"<<endl);
            pCopy->leaveTeam(cdata->m_id, pGroupChar->_team_id, psession);
        }
        pCopy->_copyChars.erase(cdata->m_id);
    }

    robj.push_back( Pair("id", id) );
    return HC_SUCCESS;
}

//cmd:createMultiBoss //�������˸�������
int ProcessCreateGroupCopyTeam(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    INFO("************** ProcessCreateGroupCopyTeam *******************"<<endl);
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata.get())
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    if (pCopy->getLeftTimes(cdata->m_id) <= 0)
    {
        return HC_ERROR_NO_GROUP_COPY_ATTACK_TIME;
    }
    groupCopyChar* pGroupCopyChar = pCopy->_getCopyChar(cdata->m_id, psession, false);
    if (pGroupCopyChar)
    {
        //��������
        if (pGroupCopyChar->_team_id == 0)
        {
            (void)pCopy->createTeam(cdata, pGroupCopyChar);
        }
        robj.push_back( Pair("id", id) );
        robj.push_back( Pair("tid", pGroupCopyChar->_team_id) );
    }
    else
    {
        return HC_ERROR_GROUP_COPY_ENTER_FIRST;
    }
    return HC_SUCCESS;
}

//cmd:leaveMultiBoss //�뿪���ɢ����
int ProcessLeaveGroupCopyTeam(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    INFO("************** ProcessLeaveGroupCopyTeam *******************"<<endl);
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        INFO("*************** ERROR get char from session fail **********************"<<endl);
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    groupCopyChar* pGroupCopyChar = pCopy->_getCopyChar(cdata->m_id, psession, false);
    if (!pGroupCopyChar)
    {
        INFO("*************** ERROR group copy char NULL **********************"<<endl);
        return HC_ERROR_GROUP_COPY_ENTER_FIRST;
    }
    else
    {
        INFO("************** find group copy char *******************"<<endl);
        //�뿪����
        if (pGroupCopyChar->_team_id)
        {
            INFO("************** leave team "<<pGroupCopyChar->_team_id<<" *******************"<<endl);
            pCopy->leaveTeam(cdata->m_id, pGroupCopyChar->_team_id, psession);
        }
    }

    robj.push_back( Pair("id", id) );
    return HC_SUCCESS;
}

//cmd:joinMultiBoss //�������
int ProcessJoinGroupCopyTeam(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    INFO("************** ProcessJoinGroupCopyTeam() *******************"<<endl);
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata.get())
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    if (pCopy->getLeftTimes(cdata->m_id) <= 0)
    {
        return HC_ERROR_NO_GROUP_COPY_ATTACK_TIME;
    }
    groupCopyChar* pGroupCopyChar = pCopy->_getCopyChar(cdata->m_id, psession, false);
    if (pGroupCopyChar)
    {
        INFO("************** ProcessJoinGroupCopyTeam() find group copy char *******************"<<endl);

        int tid = 0;
        READ_INT_FROM_MOBJ(tid,o,"groupId");
        if ((pGroupCopyChar->_team_id && tid == pGroupCopyChar->_team_id)
            || (pCopy->_teams.size() == 1 && tid == 0 && pGroupCopyChar->_team_id))
        {
            ;
        }
        else
        {
            //�뿪����
            if (pGroupCopyChar->_team_id)
            {
                pCopy->leaveTeam(cdata->m_id, pGroupCopyChar->_team_id, psession);
            }        
            if (pCopy->joinTeam(cdata, tid, psession) < 0)
            {
                ret = HC_ERROR_INVALID_GROUP_COPY_TEAM;
                tid = 0;
            }
            else
            {
                tid = pGroupCopyChar->_team_id;

                groupCopyTeam* pTeam = pCopy->_getTeam(tid);
                if (pTeam)
                {
                    INFO("************** ProcessJoinGroupCopyTeam() ,auto:"<<pTeam->_bAutoAttack<<",max:"<<pTeam->_maxMember<<",cur:"<<pTeam->members()<<" *******************"<<endl);
                }
                //�����Զ�����
                if (pTeam && pTeam->_bAutoAttack && pTeam->_maxMember <= (int)pTeam->members())
                {
                    INFO("************** ProcessJoinGroupCopyTeam() auto attack *******************"<<endl);
                    robj.push_back( Pair("cmd", "setAutoJoinBoss") ); 
                    robj.push_back( Pair("id", id) );
                    robj.push_back( Pair("groupId", tid) );
                    robj.push_back( Pair("s", 200) ); 
                    psession->send(json_spirit::write(robj));                
                    pCopy->attackCopy(pGroupCopyChar->_team_id, psession);
                    return HC_SUCCESS_NO_RET;
                }
            }
        }
        robj.push_back( Pair("id", id) );
        robj.push_back( Pair("groupId", tid) );
    }
    else
    {
        ret = HC_ERROR_GROUP_COPY_ENTER_FIRST;
    }
    return ret;
}

//cmd:attackMultiBoss    //��������
int ProcessAttackGroupCopy(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    INFO("************** ProcessAttackGroupCopy() *******************"<<endl);
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    groupCopyChar* pGroupCopyChar = pCopy->_getCopyChar(cdata->m_id, psession, false);
    if (!pGroupCopyChar)
    {
        ret = HC_ERROR_GROUP_COPY_ENTER_FIRST;;
    }
    else
    {
        //��������
        if (pGroupCopyChar->_team_id)
        {
            groupCopyTeam* pTeam = pCopy->_getTeam(pGroupCopyChar->_team_id);
            if (pTeam)
            {
                CharData* pc = pTeam->leader();
                if (!pc || pc->m_id != cdata->m_id)
                {
                    return HC_ERROR_GROUP_COPY_NOT_LEADER;
                }
                ret = pCopy->attackCopy(pGroupCopyChar->_team_id, psession);
            }
            else
            {
                ret = HC_ERROR_NO_GROUP_COPY_TEAM;
            }
        }
        else
        {
            ret = HC_ERROR_NO_GROUP_COPY_TEAM;
        }
    }
    return ret;
}

//cmd:setAutoAttackBoss //���������Զ�ս��
int ProcessSetAutoAttack(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    INFO("************** ProcessSetAutoAttack() *******************"<<endl);
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    groupCopyChar* pGroupCopyChar = pCopy->_getCopyChar(cdata->m_id, psession, false);
    if (!pGroupCopyChar)
    {
        ret = HC_ERROR_GROUP_COPY_ENTER_FIRST;;
    }
    else
    {
        if (pGroupCopyChar->_team_id)
        {
            groupCopyTeam* pTeam = pCopy->_getTeam(pGroupCopyChar->_team_id);
            if (pTeam)
            {
                CharData* pc = pTeam->leader();
                if (!pc || pc->m_id != cdata->m_id)
                {
                    return HC_ERROR_GROUP_COPY_NOT_LEADER;
                }
                int type = 1;
                READ_INT_FROM_MOBJ(type,o,"type");
                if (type)
                {
                    pTeam->_bAutoAttack = 1;
                    if ((int)pTeam->members() >= pTeam->_maxMember)
                    {
                        //���������Զ�����
                        pCopy->attackCopy(pGroupCopyChar->_team_id, psession);
                        return HC_SUCCESS_NO_RET;
                    }
                }
                else
                {
                    pTeam->_bAutoAttack = 0;
                }
                robj.push_back( Pair("type", pTeam->_bAutoAttack) );
            }
            else
            {
                ret = HC_ERROR_NO_GROUP_COPY_TEAM;
            }
        }
    }
    return ret;
}

//cmd:changeMultiBossIndex //�ı��Ա��ţ����µ���һλ����:    id//����id    roleId//��ԱID����:��
int ProcessChangeGroupCopyTeamMemberPos(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    INFO("************** ProcessChangeGroupCopyTeamMemberPos() *******************"<<endl);
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    groupCopyChar* pGroupCopyChar = pCopy->_getCopyChar(cdata->m_id, psession, false);
    if (!pGroupCopyChar)
    {
        ret = HC_ERROR_GROUP_COPY_ENTER_FIRST;;
    }
    else
    {
        if (pGroupCopyChar->_team_id)
        {
            groupCopyTeam* pTeam = pCopy->_getTeam(pGroupCopyChar->_team_id);
            if (pTeam)
            {
                CharData* pc = pTeam->leader();
                if (!pc || pc->m_id != cdata->m_id)
                {
                    return HC_ERROR_GROUP_COPY_NOT_LEADER;
                }
                int cid = 0;
                READ_INT_FROM_MOBJ(cid,o,"roleId");
                int pos = pTeam->getMemberPos(cid);
                if (pos > 1 && pos <= iMaxGroupCopyMembers)
                {
                    boost::shared_ptr<CharData> pre_cdata[iMaxGroupCopyMembers];
                    int pre_b_friend[iMaxGroupCopyMembers];
                    for (int i = 0; i < iMaxGroupCopyMembers; ++i)
                    {
                        pre_cdata[i] = pTeam->_memberList[i];
                        pre_b_friend[i] = pTeam->_bfriend[i];
                    }
                    for (int i = 1; i < iMaxGroupCopyMembers; ++i)
                    {
                        int to_pos = i + 1;
                        if (to_pos >= iMaxGroupCopyMembers)
                        {
                            to_pos = 1;
                        }
                        pTeam->_bfriend[to_pos] = pre_b_friend[i];
                        pTeam->_memberList[to_pos] = pre_cdata[i];
                    }
                    //�����ڷ��Ͷ�������
                    json_spirit::Array member_list;
                    pTeam->getMembersDetail(member_list);
                    json_spirit::Object robj;
                    robj.push_back( Pair("cmd", "getMultiBossMembers") );
                    robj.push_back( Pair("s", 200) );
                    robj.push_back( Pair("id", pCopy->_id) );
                    robj.push_back( Pair("groupId", pTeam->_id) );
                    robj.push_back( Pair("list", member_list) );
                    robj.push_back( Pair("type", pTeam->_bAutoAttack) );
                    pCopy->_broadTeamMembers(pTeam->_id, json_spirit::write(robj));
                }
                else
                {
                    ret = HC_ERROR;
                }
            }
        }
        else
        {
            ret = HC_ERROR_NO_GROUP_COPY_TEAM;
        }
    }
    return ret;
}

//cmd:fireMultiBoss //�߳���Ա
int ProcessFireGroupCopyMember(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    INFO("************** ProcessFireGroupCopyMember() *******************"<<endl);
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    groupCopyChar* pGroupCopyChar = pCopy->_getCopyChar(cdata->m_id, psession, false);
    if (!pGroupCopyChar)
    {
        ret = HC_ERROR_GROUP_COPY_ENTER_FIRST;
    }
    else
    {
        if (pGroupCopyChar->_team_id)
        {
            groupCopyTeam* pTeam = pCopy->_getTeam(pGroupCopyChar->_team_id);
            if (pTeam)
            {
                CharData* pc = pTeam->leader();
                if (!pc || pc->m_id != cdata->m_id)
                {
                    return HC_ERROR_GROUP_COPY_NOT_LEADER;
                }
                int cid = 0;
                READ_INT_FROM_MOBJ(cid,o,"roleId");
                int pos = pTeam->getMemberPos(cid);
                if (pos > 1 && pos <= iMaxGroupCopyMembers && pTeam->_memberList[pos-1].get())
                {
                    session_ptr psession2;
                    groupCopyChar* pGroupCopyCharKick = pCopy->_getCopyChar(pTeam->_memberList[pos-1]->m_id, psession2, false);
                    pCopy->leaveTeam(cid, pTeam->_id, psession2);
                    if (pGroupCopyCharKick && pGroupCopyCharKick->_psession.get())
                    {
                        json_spirit::Object robj;
                        robj.push_back( Pair("cmd", "getMultiBossMembers") );
                        robj.push_back( Pair("s", 200) );
                        robj.push_back( Pair("id", pCopy->_id) );
                        robj.push_back( Pair("groupId", 0) );
                        pGroupCopyCharKick->_psession->send(json_spirit::write(robj));

                        pGroupCopyCharKick->_psession->send(g_notify_be_kicked_msg);
                    }
                }
                else
                {
                    ret = HC_ERROR;
                }
            }
        }
        else
        {
            ret = HC_ERROR_NO_GROUP_COPY_TEAM;
        }
    }
    return ret;
}

//�ټ����˸���
int ProcessGroupCopyZhaoji(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    groupCopyChar* pGroupCopyChar = pCopy->_getCopyChar(cdata->m_id, psession, false);
    if (!pGroupCopyChar)
    {
        ret = HC_ERROR_GROUP_COPY_ENTER_FIRST;;
    }
    else
    {
        //�����ж�
        if (pGroupCopyChar->_team_id)
        {
            groupCopyTeam* pTeam = pCopy->_getTeam(pGroupCopyChar->_team_id);
            if (pTeam)
            {
                for (int i = 0; i < iMaxGroupCopyMembers; ++i)
                {
                    if (pTeam->_memberList[i].get() && pTeam->_memberList[i]->m_id == cdata->m_id)
                    {
                        boost::shared_ptr<OnlineUser> account = psession->user();
                        if (!account.get())
                        {
                            return HC_ERROR_LOGIN_FIRST;
                        }
                        std::string inviteMsg = strGroupCopyInvite;
                        str_replace(inviteMsg, "$P", MakeCharNameLink(cdata->m_name));
                        str_replace(inviteMsg, "$N", pTeam->_copyHandle._name);
                        str_replace(inviteMsg, "$L", LEX_CAST_STR(pTeam->_copyHandle._level));
                        str_replace(inviteMsg, "$G", LEX_CAST_STR(pTeam->_id));
                        str_replace(inviteMsg, "$B", LEX_CAST_STR(pTeam->_copyHandle._id));
                        str_replace(inviteMsg, "$M", LEX_CAST_STR(pTeam->_copyHandle._mapid));
                        GeneralDataMgr::getInstance()->broadCastSysMsg(inviteMsg, -1);
                        //account->m_onlineCharactor->WorldChat(inviteMsg,false);
                        return HC_SUCCESS;
                    }
                }
                return HC_ERROR;
            }
            else
            {
                ret = HC_ERROR_NO_GROUP_COPY_TEAM;
            }
        }
        else
        {
            ret = HC_ERROR_NO_GROUP_COPY_TEAM;
        }
    }
    return ret;
}

//����ټ�����
int ProcessGroupCopyZhaojiFriend(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(id);
    if (!pCopy)
    {
        return HC_ERROR_INVALID_GROUP_COPY_ID;
    }
    //cout << "cid = " << cdata->m_id << " in copyid = " << id << "is call friend" << endl;
    groupCopyChar* pGroupCopyChar = pCopy->_getCopyChar(cdata->m_id, psession, false);
    if (!pGroupCopyChar)
    {
        ret = HC_ERROR_GROUP_COPY_ENTER_FIRST;;
    }
    else
    {
        //�����ж�
        if (pGroupCopyChar->_team_id)
        {
            //cout << "team_id = " << pGroupCopyChar->_team_id << endl;
            groupCopyTeam* pTeam = pCopy->_getTeam(pGroupCopyChar->_team_id);
            if (pTeam)
            {
                if (pTeam->_maxMember == (int)pTeam->members())
                {
                    return HC_SUCCESS;
                }
                std::vector<boost::shared_ptr<CharData> > char_friends_list;
                char_friends_list.clear();

                boost::shared_ptr<char_ralation> my_rl = Singleton<relationMgr>::Instance().getRelation(cdata->m_id);
                std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_friends.begin();
                while (it != my_rl->m_my_friends.end())
                {
                    boost::shared_ptr<CharData> cda = it->second->getChar();
                    if (cda.get() && cda->m_level >= pCopy->_level)
                    {
                        char_friends_list.push_back(cda);
                    }
                    ++it;
                }
                if (char_friends_list.size() == 0)
                    return HC_ERROR_NEED_FRIEND;
                //δ��Աһֱ��
                while (char_friends_list.size() > 0)
                {
                    int random_index = my_random(0, char_friends_list.size() - 1);
                    boost::shared_ptr<CharData> frd = char_friends_list[random_index];
                    if (frd.get() == NULL || pTeam->getMemberPos(frd->m_id) != 0)
                    {
                        ;
                    }
                    else
                    {
                        session_ptr p_tmp;
                        p_tmp.reset();
                        if (pCopy->joinTeam(frd, pGroupCopyChar->_team_id, p_tmp, 1) < 0)
                        {
                            return HC_ERROR_INVALID_GROUP_COPY_TEAM;
                        }
                        else
                        {
                            //cout << "join success!!!" << endl;
                            if (pTeam->_maxMember <= (int)pTeam->members())
                            {
                                if (pTeam->_bAutoAttack)
                                {
                                    INFO("************** ProcessJoinGroupCopyTeam() auto attack *******************"<<endl);
                                    robj.push_back( Pair("cmd", "setAutoJoinBoss") ); 
                                    robj.push_back( Pair("id", id) );
                                    robj.push_back( Pair("groupId", pGroupCopyChar->_team_id) );
                                    robj.push_back( Pair("s", 200) ); 
                                    psession->send(json_spirit::write(robj));                
                                    pCopy->attackCopy(pGroupCopyChar->_team_id, psession);
                                    return HC_SUCCESS_NO_RET;
                                }
                                else
                                {
                                    break;
                                }
                            }                                
                        }
                    }
                    char_friends_list.erase(char_friends_list.begin() + random_index);
                }
                return HC_SUCCESS;
            }
            else
            {
                ret = HC_ERROR_NO_GROUP_COPY_TEAM;
            }
        }
        else
        {
            ret = HC_ERROR_NO_GROUP_COPY_TEAM;
        }
    }
    return ret;
}

