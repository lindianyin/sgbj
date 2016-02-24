#pragma once

#include <vector>
#include <string>
#include <list>
#include <map>
#include <boost/cstdint.hpp>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "loot.h"
#include "json_spirit.h"
#include "data.h"
#include "singleton.h"

//���ɨ����Ϣ
struct charSweep
{
    charSweep(int cid)
    {
        m_cid = cid;
        //m_type = 0;
        m_sweep_id = 0;
        m_start_time = 0;
        m_end_time = 0;
        m_fast_mod = 0;
    }
    //int m_type;//ɨ������
    int m_cid;
    int m_sweep_id;
    int m_fast_mod;        // 1�������ģʽ
    time_t m_start_time;    //����ʼʱ��
    time_t m_end_time;        //�������ʱ��
    std::list<Item> itemlist;//���ɨ������
    int start();
    int speedup();
    int done();
    int stop();
    void save();
    boost::uuids::uuid _uuid;    //��ʱ��Ψһid
};

class sweepMgr
{
public:
    sweepMgr();
    int speedUp(int cid);
    int done(int cid);
    boost::shared_ptr<charSweep> getCharSweepData(int cid);
private:
    std::map<int,boost::shared_ptr<charSweep> > m_sweep_task;//���ɨ����Ӣ��Ϣ
};

//ɨ���Ĵ���
int ProcessSweepDone(json_spirit::mObject& o);
//��ʼɨ��
int ProcessSweep(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

