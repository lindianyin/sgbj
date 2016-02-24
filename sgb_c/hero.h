#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "net.h"
#include "item.h"

#include "boost/smart_ptr/shared_ptr.hpp"
#include <boost/enable_shared_from_this.hpp>

struct CharTotalHeros;
struct CharData;

#define HERO_DEFAULT_SIZE 18
#define MAX_HERO_SIZE 180


enum HERO_STATE
{
    HERO_STATE_INIT = 0,
    HERO_STATE_DEFAULT,
    HERO_STATE_CITY,
};

//基础英雄材料数据
struct material_data
{
    int m_star;
    //英雄的分解熔炼数据
    int m_special_material_id;  //英雄对应特殊材料
    int m_decompose_material[3];  //分解可得基础材料
    int m_decompose_special_num;  //分解可得特殊材料
    int m_smelt_material[3];  //熔炼需要基础材料
    int m_smelt_special_num;  //熔炼需要特殊材料
};

//神将英雄数据
struct epic_hero_data
{
    int m_fragment_id;//神将对应碎片
    int m_scroll_id;//神将对应卷轴
    int m_get_cost;//招募消耗
    int m_get_level;//招募所需等级
    int m_get_vip;//招募所需vip
    double m_add[4];//四属性对应成长率
    int m_up_cost;//升级消耗
    int m_up_level;//升级所需等级
    int m_up_vip;//升级所需vip
};

//基础英雄数据
struct baseHeroData
{
    int m_hid;         //英雄id
    int m_spic;        //图片
    int m_race;       //花色类型
    int m_base_attack;    //基础攻击
    int m_base_defense;   //基础防御
    int m_base_magic;     //基础魔力
    int m_base_hp;        //基础血量
    int m_quality;        //品质
    std::string m_name; //名字
    std::string m_memo;     //介绍
    material_data m_material[iMaxHeroStar];//英雄的分解熔炼数据
    void toObj(json_spirit::Object& obj);
    void loadMeterial();

    //神将特有属性
    int m_epic;//是否神将
    int m_next_hid;//进阶英雄
    epic_hero_data m_epic_data;
    void toEpicObj(json_spirit::Object& obj);
    void loadEpicData();
};

//熔炼英雄数据
struct smeltHeroData
{
    int m_cid;
    int m_pos;
    int m_hid;         //英雄基础id
    int m_star;        //英雄星级
    boost::shared_ptr<baseHeroData> m_baseHero;
    smeltHeroData()
    {
        m_hid = 0;
        m_star = 0;
    }
    void refresh();
};

//玩家英雄数据
struct CharHeroData: public boost::enable_shared_from_this<CharHeroData>
{
    int m_id;       //唯一id
    int m_cid;      //所属角色id
    int m_hid;      //英雄id
    int m_spic;     //头像类别
    int m_race;    //花色类型
    int m_star;     //星级
    double m_add[4];   //成长率
    int m_level;    //等级
    int m_exp;      //经验
    int m_quality;    //品质
    //总攻防
    int m_attack;   //攻击
    int m_defense;  //防御
    int m_magic;   //魔力
    int m_hp;       //生命
    //英雄自身攻防(m_star,m_level)
    int m_hero_attack;   //攻击
    int m_hero_defense;  //防御
    int m_hero_magic;   //魔力
    int m_hero_hp;       //生命
    int m_state;        // 0 正常  1 默认英雄2城守英雄
    int m_city;        //守城英雄对应城池

    int m_attribute;//战斗属性值

    CharTotalHeros& m_belong_to;

    boost::shared_ptr<baseHeroData> m_baseHero;

    HeroBag m_bag;

    CharHeroData(CharData& c, CharTotalHeros& bl)
    :m_belong_to(bl)
    ,m_bag(*this, EQUIP_SLOT_MAX)
    {
        m_star = 1;
        for (int i = 0; i < 4; ++i)
        {
            m_add[i] = 0.0;
        }
        m_level = 1;
        m_exp = 0;
        m_attack = 0;
        m_defense = 0;
        m_magic = 0;
        m_hp = 0;
        m_hero_attack = 0;
        m_hero_defense = 0;
        m_hero_magic = 0;
        m_hero_hp = 0;
		m_attribute = 0;
        m_state = HERO_STATE_INIT;
        m_city = 0;
        m_changed = false;
        m_init_attr = false;
    };
    bool m_init_attr;//是否初始化过属性
    bool m_changed;  //有改动
    int addExp(int exp, int statistics_type = 0);
    int levelup(int level);
    int equipt(int slot, int eid);//穿上
    int unequipt(int slot);//卸下
    int useGem(int tid, int nums);//使用道具
    void updateAttribute(bool real_update = true);
    int Save();//保存
    void toObj(json_spirit::Object& obj, int star = 0);
    void updateStar(int up_star);
    bool checkEquiptLevel(int level);
    bool isDefault() {return m_state == HERO_STATE_DEFAULT;}
    bool isWork() {return m_state > HERO_STATE_INIT;}
};

//玩家英雄数据
struct CharTotalHeros
{
    std::map<int, boost::shared_ptr<CharHeroData> > m_heros;
    CharData& m_charData;
    int m_cid;              //角色id
    int m_default_hero;     //出战英雄
    size_t m_hero_max;//最大英雄数
    boost::shared_ptr<smeltHeroData> m_smeltHeros[iSmeltMaxCnt];  //当前可熔炼英雄

    CharTotalHeros(int cid, CharData& cdata)
    :m_charData(cdata)
    {
        m_cid = cid;
        m_default_hero = 0;
        m_changed = false;
        m_hero_max = HERO_DEFAULT_SIZE;
    };
    int Load();
    size_t addSize(size_t a);
    int buyHeroSize(int num, json_spirit::Object& robj);
    bool isFull() {return m_heros.size() >= m_hero_max;}
    int Add(int id, int level = 1, int star = 1, bool set_add = false, double add1 = 0, double add2 = 0, double add3 = 0, double add4 = 0);
    int Sub(int id);
    boost::shared_ptr<CharHeroData> GetHero(int id);
    int GetHeroByType(int htype);
    Equipment* getEquipById(int id);
    int SetDefault(int hid, json_spirit::Object& obj);//设置出战英雄
    void getList(json_spirit::Array& hlist);
    //合成
    int CompoundHeroInfo(json_spirit::Object& obj, json_spirit::mObject& o);//合成英雄界面信息
    int CompoundHero(json_spirit::Object& obj, json_spirit::mObject& o);//合成英雄
    //分解
    int DecomposeHeroInfo(json_spirit::Object& obj, json_spirit::mObject& o);//分解英雄界面信息
    int DecomposeHero(json_spirit::mObject& o);//分解英雄
    //熔炼
    int SmeltHeroInfo(json_spirit::Object& obj);//熔炼英雄界面信息
    int SmeltHeroRefresh();//熔炼英雄刷新
    int SmeltHero(json_spirit::mObject& o, json_spirit::Object& robj);//熔炼英雄刷新
    //点金
    int GoldenHeroInfo(json_spirit::Object& obj, json_spirit::mObject& o);//点金英雄界面
    int GoldenHero(json_spirit::Object& obj, json_spirit::mObject& o);//点金
    //提升神将
    int UpEpicHeroInfo(json_spirit::Object& obj, json_spirit::mObject& o);
    int UpEpicHero(json_spirit::Object& obj, json_spirit::mObject& o);
    void updateAttribute();
    int Save(); //保存
    bool m_changed; //有改动
};

struct base_hero_pack
{
    int m_id;
    std::string m_name;//名字
    int m_quality;//品质
    int m_level;//需要等级
    int m_vip;//需要VIP
    int m_silver_cost;//筹码花费
    int m_gold_cost;//金币花费
    int m_hero_cnt;//包含几个英雄
    int m_race_pack;//是否种族包(0不是，1-4表示对应种族)
    std::vector<int> m_gailvs;//各星级英雄概率
    double m_total_gailv;
    int randomStar();
    void toObj(json_spirit::Object& obj);
};

class HeroMgr
{
public:
    HeroMgr();
    boost::shared_ptr<baseHeroData> GetBaseHero(int hid);
    boost::shared_ptr<base_hero_pack> GetBaseHeroPack(int id);
    int RandomHero(int race = 0);
    int OpenHeroPack(json_spirit::Object& obj, json_spirit::mObject& o, boost::shared_ptr<CharData> cdata);
    int GetEpicHero(json_spirit::Object& obj, json_spirit::mObject& o, boost::shared_ptr<CharData> cdata);
    std::vector<int>& GetEpicList() {return m_epic_heros;}
private:
    std::vector<int> m_pack_heros; //可以开包获得英雄
    std::map<int, std::vector<int> > m_race_heros; //可以开包获得的种族英雄
    std::map<int, boost::shared_ptr<baseHeroData> > m_base_heros; //基础英雄数据
    std::vector<int> m_epic_heros; //神将英雄
    std::map<int, boost::shared_ptr<base_hero_pack> > m_base_heropacks; //英雄包
};

//查询英雄信息
int ProcessGetHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//显示角色英雄列表
int ProcessCharHeros(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//设置出战英雄
int ProcessSetDefaultHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取英雄包信息
int ProcessQueryHeroPack(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//开启英雄包
int ProcessOpenHeroPack(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//购买英雄位
int ProcessBuyHeroSize(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//合成概率信息
int ProcessCompoundHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//合成英雄
int ProcessCompoundHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//分解英雄信息
int ProcessDecomposeHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//分解英雄
int ProcessDecomposeHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//熔炼英雄信息
int ProcessSmeltHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//熔炼英雄刷新
int ProcessSmeltHeroRefresh(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//熔炼英雄
int ProcessSmeltHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//点金英雄信息
int ProcessGoldenHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//点金英雄
int ProcessGoldenHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//显示神将英雄
int ProcessEpicHeros(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//招募神将
int ProcessGetEpicHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//提升神将
int ProcessUpEpicHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessUpEpicHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

