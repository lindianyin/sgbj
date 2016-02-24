#pragma once

#include "data.h"
#include "net.h"
#include "json_spirit.h"
#include "combat.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace net;


struct base_ZST_Stronghold;
struct base_ZST_Stage;
struct base_ZST_Map;

//ս��̨�ݵ�
struct base_ZST_Stronghold
{
    int _id;    //id
    int _star;
    int _stageid;//����id
    int _mapid;//��ͼid
    int _pos;
    int _level;        //�ȼ�
    int _spic;    //ͷ��
    std::string _name;    //����
    int needAttack;

    //�ؿ��Ŀ���
    combatAttribute m_combat_attribute;

    boost::shared_ptr<StrongholdGeneralData> m_generals[9];//�����佫

    int load();
};

//ս��̨����
struct base_ZST_Stage
{
    int id;
    int mapid;
    std::string name;
    int spic;
    int needAttack;
    boost::shared_ptr<base_ZST_Map> _baseMap;
    boost::shared_ptr<base_ZST_Stronghold> _baseStrongholds[25];
};

//ս��̨��ͼ
struct base_ZST_Map
{
    int id;
    std::string name;
    boost::shared_ptr<base_ZST_Stage> stages[10];    //��ͼ�еĳ���
};

struct CharZSTStageData
{
    int m_cid;
    int m_mapid;
    int m_stageid;
    int m_state;//����״̬0���ɹ���1�ɹ���2�Ѿ�����
    int m_result_star;
    boost::shared_ptr<base_ZST_Stage> m_baseStage;
    std::vector<int> m_stronghold_state;        //���ؿ�״̬0�ɹ���1�Ѿ�����
    std::vector<int> m_stronghold_star;    //���ؿ��Ǽ�
    void load();
    void Save();
    int getStar();
};

typedef std::map<int, boost::shared_ptr<CharZSTStageData> > CharZSTMapData;

//ս��̨�е��佫����
struct char_zst_general
{
    int pos;
    int id;
    int gid;
    int cid;
    int spic;
    int level;
    int color;
    int b_nickname;
    std::string name;

    //���䡢������ͳ��
    int m_str;
    int m_int;
    int m_tongyu;

    int m_org_attack;
    int m_org_wu_fang;
    int m_org_ce_fang;

    //����
    int m_attack;
    int m_wu_fang;
    int m_ce_fang;

    //ԭʼ
    int m_org_hp_max;    //ԭʼ��Ѫ��
    int m_org_hp_hurt;//ԭʼ����Ѫ��

    combatAttribute m_combat_attribute; //ս������

    int m_inspired;
    
    //��ǰѪ��
    int m_hp_org;    //ԭʼѪ��
    int m_hp_hurt;    //����Ѫ��

    void Save();
};

struct char_zst
{
    std::map<int, boost::shared_ptr<CharZSTMapData> > CharZSTMapsData;//ս��̨�ؿ�����
    int m_cid;                //��ɫid
    int m_total_star;        //������
    int m_star_update_time; //�Ǽ�����ʱ��
    int m_cur_star_reward;  //��ǰ�����Ǽ�����id
    int m_cur_map;        //��ǰ��ս��ͼ
    int m_cur_stage;        //��ǰ��ս����
    std::list<char_zst_general> m_generals;    //����������佫

    class zstMgr& m_handle;

    char_zst(int cid, zstMgr& z)
    :m_cid(cid)
    ,m_handle(z)
    {
        m_total_star = 0;
        m_star_update_time = 0;
        m_cur_map = 0;
        m_cur_stage = 0;
        m_cur_star_reward = 0;
        m_generals.clear();
    };

    //�����佫
    void reset_generals(int cid);
    int reset_stage(int mapid, int stageid);
    //ˢ���Ǽ�
    int refreshStar(CharData& cdata, int mapid, int stageid, int type, json_spirit::Object& robj);
    int updateTotalStar();
    bool checkFinish(int mapid);
    int challenge(CharData& cdata, int mapid, int stageid, int pos, json_spirit::Object& robj);
    void combatEnd(Combat* pCombat);
    void Clear();
    void Save();
    void SaveGenerals();
    void load();
};

//ս��̨�Ǽ�����
struct total_star_rewards
{
    int _needstar;
    std::list<Item> _rewards;
    std::string toString(int level);
};

class zstMgr
{
public:
    zstMgr();
    boost::shared_ptr<char_zst> getChar(int cid);
    int queryZstMapInfo(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj);
    int queryZstStageInfo(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj);
    int refreshZstStar(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj);
    int buyZstChallenge(CharData& cdata, json_spirit::Object& robj);
    int getZstStarReward(CharData& cdata, json_spirit::Object& robj);
    int queryZstStarReward(CharData& cdata, json_spirit::Object& robj);
    int getZstMapReward(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj);
    int ZstChallenge(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj);

    int combatResult(Combat* pCombat);

    void getAction(CharData* pc, json_spirit::Array& blist);
    void resetAll();
    int GetStageMemo(int mapid, int stageid, std::string& name);
    int GetMapMemo(int mapid, std::string& name);
    boost::shared_ptr<base_ZST_Stronghold> getBaseZSTStronghold(int id);
    boost::shared_ptr<base_ZST_Stage> getBaseZSTStage(int mapid, int stageid);

private:
    int max_map;
    std::map<int, boost::shared_ptr<char_zst> > m_char_datas;
    std::map<int, boost::shared_ptr<base_ZST_Stronghold> > m_stronghold_data_map;//ս��̨�ؿ�����
    boost::shared_ptr<base_ZST_Map> m_base_maps[10];    //������ͼ
    std::map<int, boost::shared_ptr<total_star_rewards> > m_total_star_rewards;//�Ǽ�����
};

//��ѯս��̨��ͼ���� cmd ��queryZstMapInfo
int ProcessQueryZstMapInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯս��̨�������� cmd ��queryZstStageInfo
int ProcessQueryZstStageInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ˢ��ս��̨�Ǽ�cmd��refreshZstStar
int ProcessRefreshZstStar(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����ս��̨��ս���� cmd��buyZstChallenge
int ProcessBuyZstChallenge(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡս��̨�Ǽ����� cmd��getZstStarReward
int ProcessGetZstStarReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯս��̨�Ǽ����� cmd��queryZstStarReward
int ProcessQueryZstStarReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡս��̨��ͼ���� cmd��getZstMapReward
int ProcessGetZstMapReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��սս��̨ cmd��ZstChallenge
int ProcessZstChallenge(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

