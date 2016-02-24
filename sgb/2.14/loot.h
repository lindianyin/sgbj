#ifndef _LOOT_H
#define _LOOT_H

#include "boost/smart_ptr/shared_ptr.hpp"
#include <list>
#include <map>
#include "base_item.h"
#include "data.h"
#include "json_spirit.h"

int getMapSilver(int mapid, int level, bool elite);    //��ͼid���ؿ��ȼ����Ƿ�Ӣ

class lootGroup;

struct lootChance
{
    int chance;            //���֮��
    int lootGroupId;    //����֮���ε��䣬0��ʾ����Ʒ

    int score;    //����
    boost::shared_ptr<lootGroup> _lootGroup;
    Item item;
    lootChance()
    {
        chance = 0;
        lootGroupId = 0;
        score = 0;
        item.type = 0;
        item.id = 0;
        item.nums = 0;
    }
};

//������
class lootGroup
{
public:
    lootGroup(int id);
    ~lootGroup();
    int reload();
    int getLoots(std::list<Item>& loots, int depth, int extra = 0);
    int getLoots(Item& item, int depth, int extra = 0);

    int rand_Item_list(const Item& item, std::list<Item>& items, int size);
    int Item_list(std::list<Item>& items);
    std::string getLootList(const std::string& dim);
    int updateChance(int& org);
    friend class lootMgr;
private:
    std::list<lootChance> m_all_loots;
    int _id;
};

struct multiLootChance
{
    int chance;            //���֮��

    int score;    //����
    Item item;
    multiLootChance()
    {
        chance = 0;
        score = 0;
        item.type = 0;
        item.id = 0;
        item.nums = 0;
    }
};

//������
class multiLootGroup
{
public:
    multiLootGroup(int id);
    ~multiLootGroup();
    int reload();
    int getLoots(std::list<Item>& loots, int extra = 0);

    friend class lootMgr;
private:
    std::list<multiLootChance> m_all_loots;
    int _id;
};

struct lootPlaceInfo
{
    int mapId;
    int stageId;
    int pos;
    int type;//�ؿ�1����Ӣս��2
    std::string mapName;
    std::string stageName;
    std::string strongholdName;

    json_spirit::Object info;
};

class lootMgr
{
public:
    lootMgr();
    ~lootMgr();
    //���ɹؿ����� extra ���������ʣ�ֻ�Ե�һ��������
    int getStrongholdLoots(int id, std::list<Item>& items, int extra = 0);
    //��Ӣս�۵���
    int getEliteCombatsLoots(int id, std::list<Item>& loots, int extra = 0);
    //ͨ�ؽ���
    int getStageLoots(int id, std::list<Item>& loots, json_spirit::Object& robj);
    //���˸�������
    int getGroupCopyLoots(int id, std::list<Item>& loots, int extra = 0);
    //���˸�������
    std::string getGroupCopyLoots(int id, const std::string&);
    //�Թ�boss����
    int getMazeBossLoots(int id, std::list<Item>& loots, int extra);
    //�Թ�boss����
    int getMazeBossLootsInfo(int id, std::list<Item>& loots);
    //�ؿ�������Ϣ
    boost::shared_ptr<Item> getStrongholdLootInfo(int id);
    //��Ӣ�ؿ�������Ϣ
    int getEliteCombatsLootInfo(int id, std::list<Item>& loots);
    //�����������
    int getWorldItemFall(std::list<Item>& items);
    //�����������
    int getWorldItemFall(Item& item);
    //�ؿ������������
    int getBoxItemFall(int level, std::list<Item>& items);

    //÷�������齱
    int getLotteryItem(std::list<Item>& items, int extra = 0);
    //�����ʾ12�ֶ���
    int getLotteryRandItems(const Item& item, std::list<Item>& rand_items);

    //���عؿ�������
    boost::shared_ptr<lootGroup> getSecondLootgroup(int id);
    //���س��������б�
    void getLootList(int id, int num, json_spirit::Object& robj);

    //��ȡ�������
    int getBoxLoots(int id, std::list<Item>& loots, int extra);
    //��ȡ��������б�
    int getBoxLootsInfo(int id, std::list<Item>& items_list);

    //ս��̨��������
    int getZSTLoots(int mapid, int stageid, int star, std::list<Item>& loots, int extra);
    //ս��̨��������
    int getZSTLootsInfo(int mapid, int stageid, int star, std::list<Item>& loots);
    //ս��̨��ͼ����
    int getZSTMapLoots(int mapid, std::list<Item>& loots, int extra);
    //ս��̨��������
    int getZSTMapLootsInfo(int mapid, std::list<Item>& loots);
    
    //��db��������Ϣ
    int reload(int id);

    int getLootPlace(int type, int id, json_spirit::Object& robj);
    
    static lootMgr* getInstance();

    void addLootPlace(int type, int id, int stronghold);
    void addLootPlace_E(int type, int id, int elite);

    void updaeLootPlace();
private:
    std::map<int, int> m_stronghold_limits;        //�ؿ���������
    std::map<int, boost::shared_ptr<lootGroup> > m_stronghold_loot_groups;    //�ؿ�����
    std::map<int, boost::shared_ptr<lootGroup> > m_sub_loot_groups;            //�����еĶ��ε���
    std::map<int, boost::shared_ptr<lootGroup> > m_stage_loot_groups;            //ͨ�ؽ�������
    boost::shared_ptr<lootGroup> m_world_loot;        //�������
    std::map<int, boost::shared_ptr<lootGroup> > m_groupCopy_loot_groups;        //���˸�������
    std::map<int, boost::shared_ptr<lootGroup> > m_elitecombats_loot_groups;    //�ؿ�����

    std::map<int, boost::shared_ptr<multiLootGroup> > m_maze_loot_groups;    //�Թ�����
    std::map<int, boost::shared_ptr<multiLootGroup> > m_zst_loot_groups;    //ս��̨��������
    std::map<int, boost::shared_ptr<multiLootGroup> > m_zst_map_loot_groups;    //ս��̨��ͼ����

    std::map<int, boost::shared_ptr<lootPlaceInfo> > m_gem_loot_places;
    std::map<int, boost::shared_ptr<lootPlaceInfo> > m_equipment_loot_places;

    //÷�������齱��Ʒ
    boost::shared_ptr<lootGroup> m_lottery_awards;        //÷��������Ʒ
    
    std::map<int, boost::shared_ptr<lootGroup> > m_box_loot_groups;    //�������
    
    static lootMgr* m_handle;
};

#endif

