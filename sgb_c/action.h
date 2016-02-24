
#pragma once

#include <vector>
#include <list>
#include "libao.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

const int iActionSignGold = 50;
const int iActionSignScore = 5;
const int iActionSignScoreMax = 100;

const int iActionOnlineLibaoMax = 5;
const int iActionOnlineLibaoSec[iActionOnlineLibaoMax] = {900,900,1800,3600,3600};

struct CharData;

enum time_limit_action
{
    action_seven = 1,
    action_equipt_level = 2,
    action_hero_star = 3,
    action_hero_pack = 4,
    action_hero = 5,
    action_stronghold = 6,
};

/**************ǩ���****************/

struct sign_action
{
    //ÿ��ǩ�����
    std::map<int, boost::shared_ptr<baseLibao> > m_single_rewards;
    //�ۼ�ǩ�����
    std::map<int, boost::shared_ptr<baseLibao> > m_rewards;
    bool in_list(int days);
};

struct char_sign_data
{
    sign_action& m_handle;
    int m_cid;

    time_t m_generate_time;
    struct tm m_tm_generate;
    //ǩ������
    std::map<int, int> m_sign_data;
    time_t m_sign_time;
    std::map<int, int> m_getted;

    int m_total_sign;

    time_t m_time_now;
    struct tm m_tm_now;

    int m_canGet;
    
    char_sign_data(sign_action& h, int cid);
    void save();
    int getAwards(CharData& cdata, int days, json_spirit::Object& robj);
    int doSign(int day, json_spirit::Object& robj);
    int getSignAction();
    void checkReset();
};

//�����̵���Ʒ
struct baseSignShopGoods
{
    int needscore;
    Item reward;//����
};

/**************��������****************/

struct char_online_libao
{
    int id;
    int cid;
    int total_online_time;//��Ҫ������ʱ��
    int total_time;//�ܳ���ʱ��
    int state;//״̬0������ȡ1����ȡ2����ȡ
    void save();
};

struct char_online_libao_data
{
    int m_cid;
    std::vector<char_online_libao> m_online_libaos;//�������
    std::vector<Item> m_get_item;//�Ѿ���ȡ
    boost::uuids::uuid _uuid;    //��ʱ��Ψһid
    
    char_online_libao_data(int cid)
    {
        m_cid = cid;
    }
    void save_get(int id);
    bool del_timer();
    int getAwards(CharData& cdata, json_spirit::Object& robj);
    void updateState(int type);
    int getOnlineLibaoState();
    int getOnlineLibaoCnt();
};

/**************�������ջ****************/

struct seven_action
{
    //ÿ���Ӧ����
    std::map<int, boost::shared_ptr<baseLibao> > m_rewards;
};

struct char_seven_data
{
    seven_action& m_handle;
    int m_cid;
    int m_cur_day;
    int m_canGet;
    std::map<int, int> m_geted;
    std::vector<int> m_finish_list;
    
    char_seven_data(seven_action& h, int cid);
    int getAwards(CharData& cdata, int days, json_spirit::Object& robj);
    void save();
    void check(CharData* pc);
};

/**************����ǿ���****************/

struct equipt_level_action
{
    //ǿ���ȼ���Ӧ����
    std::map<int, boost::shared_ptr<baseLibao> > m_rewards;
};

struct char_equipt_level_data
{
    equipt_level_action& m_handle;
    int m_cid;
    int m_canGet;
    std::map<int, int> m_geted;
    std::vector<int> m_finish_list;
    
    char_equipt_level_data(equipt_level_action& h, int cid);
    int getAwards(CharData& cdata, int level, json_spirit::Object& robj);
    void check(CharHeroData* ch);
    void save();
};

/**************�������ǻ****************/

struct hero_star_action
{
    //�Ǽ���Ӧ����
    std::map<int, boost::shared_ptr<baseLibao> > m_rewards;
    bool in_list(int star);
};

struct char_hero_star_data
{
    hero_star_action& m_handle;
    int m_cid;
    int m_canGet;
    std::map<int, int> m_geted;
    std::vector<int> m_finish_list;
    
    char_hero_star_data(hero_star_action& h, int cid);
    int getAwards(CharData& cdata, int star, json_spirit::Object& robj);
    void heroStar(int star);
    void save();
};

/**************����Ӣ�ۻ****************/

struct hero_action
{
    //Ӣ�۶�Ӧ����
    std::map<int, boost::shared_ptr<baseLibao> > m_rewards;
    bool in_list(int id);
};

struct char_hero_data
{
    hero_action& m_handle;
    int m_cid;
    int m_canGet;
    std::map<int, int> m_geted;
    std::vector<int> m_finish_list;
    
    char_hero_data(hero_action& h, int cid);
    int getAwards(CharData& cdata, int id, json_spirit::Object& robj);
    void heroGet(int id);
    void save();
};

/**************������ػ****************/

struct stronghold_action
{
    //�ؿ���Ӧ����
    std::map<int, boost::shared_ptr<baseLibao> > m_rewards;
    bool in_list(int id);
};

struct char_stronghold_data
{
    stronghold_action& m_handle;
    int m_cid;
    int m_canGet;
    std::map<int, int> m_geted;
    std::vector<int> m_finish_list;
    
    char_stronghold_data(stronghold_action& h, int cid);
    int getAwards(CharData& cdata, int id, json_spirit::Object& robj);
    void stronghold(int id);
    void save();
};

class actionMgr
{
public:
    actionMgr();
    int getTimeLimitActionState(CharData* pc);
    void notifyTimeLimitActionState(CharData* pc);
    void getTimeLimitActionList(CharData* pc, json_spirit::Object& robj);
    void notifySignState(CharData* pc);
    void getButton(CharData* pc, json_spirit::Array& list);

    /**************ǩ���****************/
    void getSignInfo(char_sign_data& e, json_spirit::Object& robj);
    char_sign_data* getCharSignData(int cid);
    std::vector<baseSignShopGoods>& getSignShopGoods() {return m_sign_shop_goods;}
private:
    sign_action m_sign_action;
    std::map<int, boost::shared_ptr<char_sign_data> > m_char_sign_actions;
    std::vector<baseSignShopGoods> m_sign_shop_goods;        //��Ʒ�б�
    
    /**************��������****************/
public:
    void getOnlineLibaoInfo(char_online_libao_data& e, json_spirit::Object& robj);
    char_online_libao_data* getCharOnlineLibaoData(int cid);
    void updateOnlineLibaoAction(int cid);
    void resetOnlineLibaoAll();
private:
    std::map<int, boost::shared_ptr<char_online_libao_data> > m_char_online_actions;

    /**************���ջ****************/
public:
    bool isSevenActionOpen(CharData* pc);
    void getSevenAction(CharData* pc, char_seven_data& e, json_spirit::Object& robj);
    char_seven_data* getCharSevenData(int cid);
    void updateSevenAction(CharData* pc);
private:
    seven_action m_seven_action;
    std::map<int, boost::shared_ptr<char_seven_data> > m_char_seven_actions;

    /**************ǿ���****************/
public:
    bool isEquiptLevelActionOpen(CharData* pc);
    void getEquiptLevelAction(CharData* pc, char_equipt_level_data& e, json_spirit::Object& robj);
    char_equipt_level_data* getCharEquiptLevelData(int cid);
    void updateEquiptLevelAction(CharHeroData* ch);
private:
    equipt_level_action m_equipt_level_action;
    std::map<int, boost::shared_ptr<char_equipt_level_data> > m_char_equipt_level_actions;
    
    /**************���ǻ****************/
public:
    bool isHeroStarActionOpen(CharData* pc);
    void getHeroStarAction(CharData* pc, char_hero_star_data& e, json_spirit::Object& robj);
    char_hero_star_data* getCharHeroStarData(int cid);
    void updateHeroStarAction(int cid, int star);
private:
    hero_star_action m_hero_star_action;
    std::map<int, boost::shared_ptr<char_hero_star_data> > m_char_hero_star_actions;
    
    /**************˺���****************/
public:
    bool isHeroPackActionOpen(CharData* pc);
    void getHeroPackAction(CharData* pc, json_spirit::Object& robj);
    
    /**************Ӣ�ۻ****************/
public:
    bool isHeroActionOpen(CharData* pc);
    void getHeroAction(CharData* pc, char_hero_data& e, json_spirit::Object& robj);
    char_hero_data* getCharHeroData(int cid);
    void updateHeroAction(int cid, int id);
private:
    hero_action m_hero_action;
    std::map<int, boost::shared_ptr<char_hero_data> > m_char_hero_actions;
    
    /**************��ػ****************/
public:
    bool isStrongholdActionOpen(CharData* pc);
    void getStrongholdAction(CharData* pc, char_stronghold_data& e, json_spirit::Object& robj);
    char_stronghold_data* getCharStrongholdData(int cid);
    void updateStrongholdAction(int cid, int id);
private:
    stronghold_action m_stronghold_action;
    std::map<int, boost::shared_ptr<char_stronghold_data> > m_char_stronghold_actions;
};


//��ѯǩ����Ϣ
int ProcessQuerySignInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ǩ��
int ProcessSign(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����ǩ���̵�
int ProcessQuerySignShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����ǩ���̵���Ʒ
int ProcessBuySignShopGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ���������Ϣ
int ProcessQueryOnlineLibaoInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�������״̬����
int ProcessOnlineLibaoUpdate(json_spirit::mObject& o);
//��ѯ��ʱ��б�
int ProcessQueryTimeLimitActionList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ���ջ��Ϣ
int ProcessQuerySevenInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯǿ�����Ϣ
int ProcessQueryEquiptLevelInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ���ǻ��Ϣ
int ProcessQueryHeroStarInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ˺�����Ϣ
int ProcessQueryHeroPackInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯӢ�ۻ��Ϣ
int ProcessQueryHeroActionInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ��ػ��Ϣ
int ProcessQueryStrongholdActionInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

