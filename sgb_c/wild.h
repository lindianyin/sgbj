#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "new_combat.hpp"
#include "net.h"

struct CharData;

struct base_wild_city_info
{
    int pos;
    std::string name;
    int x;
    int y;
};

struct wild_city
{
    int m_id;
    int m_owner_cid;//ռ�����
    int m_defense_hid;//����Ӣ��
    int m_levy_start;//˰�տ�ʼʱ��(���ó��ؿ�ʼ����)
    int m_fight;//���ս��״̬
    bool m_get_notify;//����ȡ֪ͨ���
    bool m_notify;//����5k֪ͨ���
    std::vector<int> m_viewer;//�ɿ��������
    void reset();
    void toObj(json_spirit::Object& robj);
    void save();
    void broadCastInfo();
    void addViewer(int cid);
    void removeViewer(int cid);
    int levy_get();
};

struct CharWildCitys
{
    CharData& m_chardata;
    std::vector<int> m_view_citys;

    CharWildCitys(CharData& c)
    :m_chardata(c)
    {
    };
    int load();
    int wildLevy(int id, json_spirit::Object& robj);//˰��Ұ��ǳ�
    int wildDefense(int purpose, int id, int hid, json_spirit::Object& robj);
    void wildViewRefresh();
    bool checkView(int id);
    int getOwnCnt();
    void save();
};

class wildMgr
{
public:
    wildMgr();
    boost::shared_ptr<base_wild_city_info> getWildInfo(int pos);
    wild_city* getWildCity(int id);
    int getRandomWildCity();
    void checkWildCity();
    int combatResult(chessCombat* pCombat);    //ս������
private:
    int m_max_wild_city_id;
    std::vector<boost::shared_ptr<wild_city> > m_wild_citys;
    std::vector<boost::shared_ptr<base_wild_city_info> > base_wildinfo_list;//����������Ϣ
};

//��ȡ����ǳ��б�
int ProcessQueryWildCitys(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Ǳ���˰
int ProcessWildCityLevy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//Ұ��ǳ����ó���
int ProcessWildCityDefense(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

