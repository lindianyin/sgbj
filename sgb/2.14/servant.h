
#ifndef _SERVANT_H_
#define _SERVANT_H_

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

enum servant_type
{
    type_free = 0,
    type_master = 1,
    type_servant = 2
};

struct baseInteract
{
    baseInteract()
    {
        m_id = 0;
        m_type = 0;
        m_name = "";
        m_memo = "";
        m_memo2 = "";
    }
    int m_id;//����id
    int m_type;//�������1-����2-�Ҷ�
    std::string m_name;
    std::string m_memo;
    std::string m_memo2;
};

struct charServant
{
    charServant()
    {
        m_cid = 0;
        m_type = 0;
        m_catch_time = 0;
        m_buy_catch_time = 0;
        m_interact_time = 0;
        m_rescue_time = 0;
        m_sos_time = 0;
        m_be_sos_time_f = 0;
        m_be_sos_time_c = 0;
        m_resist_time = 0;
        m_buy_resist_time = 0;
        m_get_num = 0;
        m_interact_cooltime = 0;
        m_master_id = 0;
        m_output = 0;
        m_left = 0;
        m_start_time = 0;
        m_end_time = 0;
        m_fight_with = 0;
    }
    int m_cid;    //���id
    int m_type;    //������0-������1-���ˣ�2-�Ҷ�
    int m_catch_time;        //ץ������
    int m_buy_catch_time;    //���չ���ץ������
    int m_interact_time;    //��������
    int m_rescue_time;        //��ȴ���
    int m_sos_time;        //��ȴ���
    int m_be_sos_time_f;        //��������ȴ���
    int m_be_sos_time_c;        //��������ȴ���
    int m_resist_time;        //��������
    int m_buy_resist_time;//���չ��򷴿�����
    int m_get_num;            //���ջ�õ���ʯ�������ӼҶ����
    time_t m_interact_cooltime;//������ȴ���ʱ��

    int m_master_id;        //����id
    int m_fight_with;        //����˭ս��

    std::list<int> m_servant_list;//�Ҷ�
    std::list<int> m_loser_list;//���°ܽ�
    std::list<int> m_enemy_list;//����֮��
    std::list<std::string> m_event_list;//�¼�
    std::list<int> m_rescue_list;//����б�

    int m_output;//�Ͷ�����(�������ū��)
    int m_left;//ʣ��Ҷ��������
    time_t m_start_time;//��ץ���ɻʼʱ��
    time_t m_end_time;//�ɻ����ʱ��
    int start();
    int done();
    int stop();

    boost::uuids::uuid _uuid;    //��ʱ��Ψһid
};

class servantMgr
{
public:
    int reload();
    int getAction(CharData* cdata, json_spirit::Array& elist);
    int Save_data(int cid);
    int Save_enemy_list(int cid);
    int Save_loser_list(int cid);
    int Save_event_list(int cid);
    int Save_rescue_list(int cid);
    int Done(int cid);
    static servantMgr* getInstance();
    int ServantListInfo(int cid, json_spirit::Object& robj);
    bool CheckServant(int master_id, int servant_id);
    bool CheckRescue(int cid, int rescue_cid);
    bool RemoveServant(int master_id, int servant_id);
    bool CheckCanList(int master_id, int cid);
    bool RemoveFromCanList(int master_id, int cid);
    int addServantEvent(int cid, std::string msg);
    int addServantEnemy(int cid, int eid);
    int addServantLoser(int cid, int eid);
    int getServantType(int cid);
    int getServantInfo(CharData& cData, json_spirit::Object& robj);
    int getServantEventsList(CharData& cData, json_spirit::Object& robj);
    int getServantList(CharData& cData, int type, int purpose, json_spirit::Object& robj);
    int getMax(CharData& cData);
    int DealServantAction(CharData& cData, int type, int extra_type, int id, int aid, json_spirit::Object& robj);
    int getInteractionList(CharData& cData, json_spirit::Object& robj);
    boost::shared_ptr<baseInteract> getInteraction(int id);
    boost::shared_ptr<charServant> GetCharServant(int cid);
    boost::shared_ptr<charServant> open(int cid);
    void deleteChar(int cid);
    void resetAll();
    int combatResult(Combat* pCombat);
    void setFactor(int fac);
    void servantRealReward(int &get);
private:
    static servantMgr* m_handle;
    std::map<uint64_t,boost::shared_ptr<charServant> > m_charServant_list;
    std::list<boost::shared_ptr<baseInteract> > m_base_Interact_list;

    //�Ҷ�����ϵ��
    int m_servant_factor;
};

#endif

