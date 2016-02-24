#pragma once

#include "data.h"
#include "net.h"
#include "json_spirit.h"
#include "new_combat.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace net;

//��Ӣ��ͼ�½�����
struct baseCopyMap
{
    int m_id;//��ͼid
    std::string m_name;//��ͼ����
    std::list<Item> m_finish_reward;//ͨ�ؽ���
    std::list<Item> m_perfect_reward;//����ͨ�ؽ���
    void load();
};

//��Ӣ����
struct baseCopy
{
    int m_id;       //����Ψһid
    int m_level;    //�����ȼ�
    int m_mapid;    //������ͼ
    int m_spic;     //����ͼƬ
    int m_silver;   //����ӵ�еĳ�ʼ����
    int m_openLevel;
    std::string m_name;
    std::string m_chat;
    //��������
    std::list<Item> m_Item_list;
    boost::shared_ptr<baseStrongholdHeroData> m_hero;//�ؿ�Ӣ��
};

//��Ҹ�������
struct CharCopyData
{
    boost::shared_ptr<baseCopy> m_baseCopy;

    int m_cid;//��ɫid
    int m_copyid;
    int m_result;//��������(������ʷ״̬)
    int m_can_attack;//�ɹ�������
    int m_can_attack_max;//���ɹ�������
    void reset();
    void save();
};

//��ҵ�ͼ��������
struct CharMapCopyData
{
    int m_mapid;//��ͼid
    int m_cid;//��ɫid
    std::list<boost::shared_ptr<CharCopyData> > m_char_copy_list;//��������
    int load();
    void checkFinish();
};

typedef std::map<int, boost::shared_ptr<CharMapCopyData> > m_char_map_copy;//��Ҹ���ͼ��������

//�����̵���Ʒ
struct baseCopyGoods
{
    int id;
    int cost;
    int need_copy_map;
    Item m_item;
};

//����
class copyMgr
{
public:
    copyMgr();
    //ÿ���������й�������
    void dailyUpdate();
    boost::shared_ptr<baseCopy> getCopyById(int copyid);
    boost::shared_ptr<CharCopyData> getCharCopy(int cid, int mapid, int copyid);
    boost::shared_ptr<CharMapCopyData> getCharCopys(int cid, int mapid);
    int getCharCopyMapList(int cid, json_spirit::Object& robj);
    int getCharCopyList(int cid, int mapid, json_spirit::Object& robj);
    //�½ڸ����Ƿ�ȫͨ
    bool isCharMapCopyPassed(int cid, int mapid);
    int getCharCurMap(int cid);
    //��ʼ����Ҹ���
    int initCharMapCopy(int cid, int mapid);
    int ResetCopy(int cid, int mapid, int copyid);
    int AddCopyTimes(int cid, int mapid, int copyid);
    int combatResult(chessCombat* pCombat);    //ս������
    boost::shared_ptr<baseCopyGoods> GetBaseCopyGoods(int id);
    int getCopyShop(int cid, json_spirit::mObject& o, json_spirit::Object& robj);
    int GetCopyFinishReward(int cid, int mapid, int type, json_spirit::Object& robj);
private:
    int m_max_mapid;
    std::vector<boost::shared_ptr<baseCopyMap> > m_copy_maps;    //ȫ����ͼ�½�
    std::vector<boost::shared_ptr<baseCopy> > m_copys;    //ȫ����ͼ��������
    std::map<int, boost::shared_ptr<m_char_map_copy> > m_char_copys;    //ȫ����ҵ�ͼ��������
    std::map<int, boost::shared_ptr<baseCopyGoods> > m_base_copy_goods; //�����̵���Ʒ
};

//��ȡ������ͼ�б�
int ProcessGetCopyMapList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡĳ�½ڸ����б�
int ProcessGetCopyList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ����������Ϣ
int ProcessGetCopyInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����ĳ����
int ProcessResetCopy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����ĳ������������
int ProcessAddCopyTimes(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����̵�
int ProcessQueryCopyShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessBuyCopyShopGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ������ͼͨ�ر���
int ProcessGetCopyFinishReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

