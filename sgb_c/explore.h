
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

struct baseCave
{
    int m_id;
    int m_spic;
    int m_open_level;
    int m_open_vip;
    std::string m_name;
    std::string m_memo;
    int m_exp_star;
    int m_silver_star;
    void toObj(json_spirit::Object& obj);
};

//���̽����Ϣ
struct charExplore
{
    charExplore(int cid)
    {
        m_cid = cid;
        m_cave_id = 0;
        m_start_time = 0;
    }
    int m_cid;
    int m_cave_id;
    boost::shared_ptr<baseCave> m_base_cave;
    time_t m_start_time;    //����ʼʱ��
    int start(int cave_id = 0);
    int getReward(CharData* pc, json_spirit::Object& robj);
    void save();
};

class exploreMgr
{
public:
    exploreMgr();
    int getBestCave(CharData* pc);
    boost::shared_ptr<baseCave> getBaseCave(int caveid);
    boost::shared_ptr<charExplore> getCharExploreData(int cid);
    std::map<int,boost::shared_ptr<baseCave> >& getBaseCaves() {return m_caves;}
private:
    std::map<int,boost::shared_ptr<baseCave> > m_caves;//��̽����Ѩ
    std::map<int,boost::shared_ptr<charExplore> > m_char_explore;//���ɨ����Ӣ��Ϣ
};

//��ѯ��Ѩ�б�
int ProcessQueryExploreCaveList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ�һ���Ѩ��Ϣ
int ProcessQueryExploreCaveInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʼ̽��
int ProcessExplore(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ̽������
int ProcessGetExploreReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

