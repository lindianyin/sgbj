
#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "item.h"
#include "new_combat.hpp"

struct baseStronghold;
struct baseStage;
struct baseMap;
struct CharData;

enum STRONGHOLD_TYPE
{
    STRONGHOLD_TYPE_INIT = 0,
    STRONGHOLD_TYPE_NORMAL = 1,
    STRONGHOLD_TYPE_EXPLORE = 2,
    STRONGHOLD_TYPE_BOX = 3,
    STRONGHOLD_TYPE_CAPTURE = 4,
};

struct Pos
{
    Pos()
    {
        m_x = 0;
        m_y = 0;
    };
    Pos(int x, int y)
    {
        m_x = x;
        m_y = y;
    };
    int m_x;
    int m_y;
};


//�ؿ��佫����
struct baseStrongholdHeroData
{
    std::string m_name; //����
    int m_spic;         //ͼƬ
    int m_star;         //�Ǽ�
    int m_quality;      //Ʒ��
    int m_race;         //����
    int m_level;        //�ȼ�
    int m_hp;           //Ѫ��
    int m_attack;       //����
    int m_defense;      //����
    int m_magic;        //ħ��
    //boost::shared_ptr<specialSkill> m_speSkill;
    void toObj(json_spirit::Object& obj);
};

//��ͼ-����-�ؿ�
//�ؿ�����
struct baseStronghold
{
    int m_id;       //�ؿ�Ψһid
    int m_level;    //�ؿ��ȼ�
    int m_mapid;    //������ͼ
    int m_stageid;  //��������
    int m_pos;      //����λ��
    int m_spic;     //�ؿ�ͼƬ
    int m_type;     //�ؿ�����(��ͨ��̽�������䣬��²)
    int m_need_times;    //ռ����Ҫ�Ĺ������
    int m_silver;   //�ؿ�ӵ�еĳ�ʼ����
    int m_cost_gold;//��ȯͨ������
    int m_base_exp;
    int m_base_prestige;
    std::string m_name;
    std::string m_chat;//����
    std::string m_memo;
    std::string m_tips;

    int m_guide_id;    //����ؿ��Ķ�Ӧ����

    //��ͼ������
    Pos m_station;

    //�ؿ�·��
    std::vector<Pos> m_path;

    //�ؿ�ս��ָ������
    std::vector<int> m_card_rank;

    //ս������
    combatAttribute m_combat_attribute;

    //�ؿ�����
    boost::shared_ptr<json_spirit::Object> m_loot;
    boost::shared_ptr<Item> m_Item;

    boost::shared_ptr<baseStrongholdHeroData> m_hero;//�ؿ�Ӣ��
    boost::shared_ptr<baseStage> m_baseStage;//�ؿ���������
    ~baseStronghold() {std::cout << "destroying a baseStronghold" << std::endl; }
};

//�����½�
struct baseStage
{
    int m_id;       //����id(��ͼ��)
    int m_mapid;    //������ͼ
    int m_spic;
    int m_minlevel;
    int m_maxlevel;
    std::string m_name;//������
    int m_size;
    double m_prestige_fac[4];//����ϵ��

    std::vector<boost::shared_ptr<baseStronghold> > m_baseStrongholds;//�����ؿ�
    boost::shared_ptr<baseMap> m_baseMap;//����������ͼ
};

//��ͼ
struct baseMap
{
    int m_id;       //��ͼΨһid
    std::string m_name;//��ͼ��
    std::string m_memo;
    double m_exp_fac;//����ϵ��
    std::vector<boost::shared_ptr<baseStage> > m_baseStages;//��ͼ�����½�
};

//��ҹؿ���������
struct CharStrongholdData
{
    int m_cid;  //��ɫid
    int m_state;//�ؿ�״̬-2�������ڣ�-1����δ������������ܹ�������
    boost::shared_ptr<baseStronghold> m_baseStronghold;

    CharStrongholdData(int cid)
    :m_cid(cid)
    {};
    bool isPassed();
    int state();
    int exp_reward();
    int prestige_reward(int race);
};

//��ҳ�����������
struct CharStageData
{
    int m_cid;
    boost::shared_ptr<baseStage> m_baseStage;
    std::vector<boost::shared_ptr<CharStrongholdData> > m_strongholds;//�����ؿ�
    void save();
};

typedef std::map<int, boost::shared_ptr<CharStageData> > CharMapData;

//��ҽ�������
struct CharTempoData
{
    CharData& m_charData;
    std::map<int, boost::shared_ptr<CharMapData> > CharMapsData;
    CharTempoData(CharData& c)
    :m_charData(c)
    {
    };
    int load(int cid, int loadMap);
    //�����ɫ�ؿ�����
    int InitCharTempo(int mapid);
    int update(int stronghold, bool bBroad);
    int Save();
    bool isMapPassed(int mapid);
    boost::shared_ptr<CharStrongholdData> getDestStronghold();//����Ŀǰ�ɴ�Ĺؿ�
    int get_stage_finish_loot(int mapid, int stageid , json_spirit::Object& robj);
    bool check_stage_finish(int mapid, int stageid);
};

class mapMgr
{
public:
    mapMgr();
    int GetStrongholdid(int mapid, int stageid, int pos);//��ȡ�ؿ�id��
    int GetStrongholdPos(int& mapid, int& stageid, int strongholdid);//��ȡ�ؿ�λ��
    int GetMapMemo(int mapid, std::string& name, std::string& memo);//��ȡ��ͼ����
    boost::shared_ptr<baseStronghold> GetBaseStrongholdData(int strongholdid);    //��ȡ�����ؿ�
    boost::shared_ptr<baseStronghold> GetBaseStrongholdData(int mapid, int stageid, int pos);    //��ȡ�����ؿ�
    boost::shared_ptr<baseMap> GetBaseMap(int mapid);//��õ�ͼ
    boost::shared_ptr<CharStrongholdData> GetCharStrongholdData(int cid, int mapid, int stageid, int strongholdpos);    //��ȡ��ɫ�ؿ�״̬
    int combatResult(chessCombat* pCombat);    //ս������
private:
    std::map<int, boost::shared_ptr<baseStronghold> > m_base_stronghold_map;//�����ؿ�����
    std::vector<boost::shared_ptr<baseMap> > m_base_maps;    //������ͼ
};

//��ʾ��ҵ�ͼ��������
int ProcessCharMapTempo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʾ��ҳ����ؿ�����
int ProcessCharStageTempo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʾ��ҵ����ؿ���Ϣ
int ProcessCharStronghold(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������ͼ�ؿ�����
int ProcessGetStrongholdBox(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡͨ�ؽ���
int ProcessGetStageFinishLoot(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯͨ�ؽ���
int ProcessCheckStageFinish(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȯͨ��
int ProcessGoldAttackStronghold(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

