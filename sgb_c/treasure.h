
#pragma once

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
#include "new_combat.hpp"

enum TREASURE_TYPE
{
    TREASURE_INIT = 0,
    TREASURE_WHITE,
    TREASURE_GREEN,
    TREASURE_BLUE,
    TREASURE_PURPLE,
    TREASURE_ORANGE,
    TREASURE_RED,
    TREASURE_MAX = TREASURE_RED,
};

struct base_treasures
{
    int silver;
    int need_min;
    std::string name;
    std::string color_name;
    //����
    std::list<Item> m_Item_list;
};

struct treasure
{
    treasure(int cid, int tid)
    {
        m_cid = cid;
        m_tid = tid;
        m_rob_time = 2;
        m_silver = 0;
        m_needmin = 0;
        m_start_time = time(NULL);
        m_end_time = 0;
    }
    int m_cid;
    int m_tid;
    int m_needmin;
    int m_rob_time;//�ɱ���ȡ����
    int m_silver;
    time_t m_start_time;//��ʼʱ��
    time_t m_end_time;//����ʱ��
    int start();
    int finish();

    boost::uuids::uuid _uuid;    //��ʱ��Ψһid
};

struct char_treasure
{
    int m_cid;
    int m_tid;
    int m_state;//0��ʼ1Ѱ��
    int m_x;
    int m_y;
    int m_refresh_times;//ˢ�´���
    boost::shared_ptr<treasure> m_treasure;//�����еĲر�ͼ
    int getCanRobTimes();
    int getCanStartTimes();
    void save();
    void reset();
};

struct event_log
{
    int m_atk_id;
    int m_def_id;
    int m_tid;
    int m_silver;
};

class treasureMgr
{
public:
    treasureMgr();
    int reload();
    void getButton(CharData* pc, json_spirit::Array& list);
    int getinsence(int cid);
    int getoutsence(int cid);
    int start(int cid);
    int finish(int cid);
    int combatResult(chessCombat* pCombat);
    int robReward(int atk_id, int def_id, int& silver);
    int jobDone(int cid);
    int getList(int cid, json_spirit::Object& robj);
    int getInfo(int cid, int id, json_spirit::Object& robj);
    int broadRobEvent();
    int getRobEvent(json_spirit::Object& robj);
    int getTreasureList(CharData* pc, json_spirit::Object& robj);
    int getTreasureInfo(int tid, json_spirit::Object& robj);
    int getCharTreasureInfo(CharData* pc, json_spirit::Object& robj);
    int refresh(CharData* pc, int type, json_spirit::Object& robj);
    boost::shared_ptr<char_treasure> getCharTreasure(int cid);
    boost::shared_ptr<base_treasures> getBaseTreasure(int tid);
private:
    std::map<int, boost::shared_ptr<char_treasure> > m_char_treasures;//��Ҳر�ͼ
    std::map<int, boost::shared_ptr<base_treasures> > m_base_treasures;//�����ر�ͼ�б�
    std::list<boost::shared_ptr<event_log> > m_event_log;//�¼���¼
    std::map<uint64_t,int> m_uid_list;//���浱ǰ���Ż��ͽ��������б�
};

//�ر�ͼ��ɵĴ���
int ProcessTreasureDone(json_spirit::mObject& o);
//��ȡ������Ϣ
int ProcessGetRobEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ�ر�ͼ�б�
int ProcessGetBaseTreasureInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ�ر�ͼ�б�
int ProcessGetAllTreasureList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ��ɫ��ǰ�ر�ͼ��Ϣ
int ProcessGetCharTreasure(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessDealTreasure(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����еĲر�ͼ�б�
int ProcessGetTreasureList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����еĲر�ͼ��Ϣ
int ProcessGetTreasureInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�뿪�ر�ͼ����
int ProcessQuitTreasure(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

