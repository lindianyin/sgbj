#pragma once

#include "data.h"
#include "net.h"
#include "json_spirit.h"
#include "new_combat.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace net;

//����������
struct baseShenling
{
    int m_id;       //id
    int m_level;    //����
    int m_silver;   //��ʼ����
    std::string m_chat;
    //����
    std::list<Item> m_Item_list;
    boost::shared_ptr<baseStrongholdHeroData> m_hero;//פ��Ӣ��
    void toObj(json_spirit::Object& obj);
};

//�������������
struct CharShenling
{
    int m_cid;//��ɫid
    int m_sid;//��ǰ���Ĳ���(week_reset)
    int m_magics[3];//��Ӧ����
    void load();
    void refreshSkill();
    void reset();
    void save();
};

//�������̵���Ʒ
struct baseShenlingGoods
{
    int id;
    int cost;
    Item m_item;
};

//����
class shenlingMgr
{
public:
    shenlingMgr();
    //ÿ���������в���
    void weekUpdate();
    boost::shared_ptr<baseShenling> getShenlingById(int sid);
    boost::shared_ptr<CharShenling> getCharShenling(int cid);
    int getCharShenlingInfo(int cid, json_spirit::Object& robj);
    int getShenlingList(int cid, json_spirit::Object& robj);
    int refreshSkill(int cid);
    int reset(int cid, bool needgold = false);
    int buyTimes(int cid);
    int combatResult(chessCombat* pCombat);    //ս������
    boost::shared_ptr<baseShenlingGoods> GetBaseShenlingGoods(int id);
    int getShenlingShop(json_spirit::mObject& o, json_spirit::Object& robj);
private:
    std::vector<boost::shared_ptr<baseShenling> > m_shenling;    //ȫ������������
    std::map<int, boost::shared_ptr<CharShenling> > m_char_shenling;    //ȫ���������������
    std::map<int, boost::shared_ptr<baseShenlingGoods> > m_base_shenling_goods; //�������̵���Ʒ
};

//��ȡ�������б�
int ProcessGetShenlingList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ����������
int ProcessGetShenlingInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ˢ������������
int ProcessRefreshSkill(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����������
int ProcessResetShenling(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������ս����
int ProcessBuyShenlingTimes(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����������̵�
int ProcessQueryShenlingShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������������Ʒ
int ProcessBuyShenlingShopGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

