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

//����Ӣ�۲�������
struct material_data
{
    int m_star;
    //Ӣ�۵ķֽ���������
    int m_special_material_id;  //Ӣ�۶�Ӧ�������
    int m_decompose_material[3];  //�ֽ�ɵû�������
    int m_decompose_special_num;  //�ֽ�ɵ��������
    int m_smelt_material[3];  //������Ҫ��������
    int m_smelt_special_num;  //������Ҫ�������
};

//��Ӣ������
struct epic_hero_data
{
    int m_fragment_id;//�񽫶�Ӧ��Ƭ
    int m_scroll_id;//�񽫶�Ӧ����
    int m_get_cost;//��ļ����
    int m_get_level;//��ļ����ȼ�
    int m_get_vip;//��ļ����vip
    double m_add[4];//�����Զ�Ӧ�ɳ���
    int m_up_cost;//��������
    int m_up_level;//��������ȼ�
    int m_up_vip;//��������vip
};

//����Ӣ������
struct baseHeroData
{
    int m_hid;         //Ӣ��id
    int m_spic;        //ͼƬ
    int m_race;       //��ɫ����
    int m_base_attack;    //��������
    int m_base_defense;   //��������
    int m_base_magic;     //����ħ��
    int m_base_hp;        //����Ѫ��
    int m_quality;        //Ʒ��
    std::string m_name; //����
    std::string m_memo;     //����
    material_data m_material[iMaxHeroStar];//Ӣ�۵ķֽ���������
    void toObj(json_spirit::Object& obj);
    void loadMeterial();

    //����������
    int m_epic;//�Ƿ���
    int m_next_hid;//����Ӣ��
    epic_hero_data m_epic_data;
    void toEpicObj(json_spirit::Object& obj);
    void loadEpicData();
};

//����Ӣ������
struct smeltHeroData
{
    int m_cid;
    int m_pos;
    int m_hid;         //Ӣ�ۻ���id
    int m_star;        //Ӣ���Ǽ�
    boost::shared_ptr<baseHeroData> m_baseHero;
    smeltHeroData()
    {
        m_hid = 0;
        m_star = 0;
    }
    void refresh();
};

//���Ӣ������
struct CharHeroData: public boost::enable_shared_from_this<CharHeroData>
{
    int m_id;       //Ψһid
    int m_cid;      //������ɫid
    int m_hid;      //Ӣ��id
    int m_spic;     //ͷ�����
    int m_race;    //��ɫ����
    int m_star;     //�Ǽ�
    double m_add[4];   //�ɳ���
    int m_level;    //�ȼ�
    int m_exp;      //����
    int m_quality;    //Ʒ��
    //�ܹ���
    int m_attack;   //����
    int m_defense;  //����
    int m_magic;   //ħ��
    int m_hp;       //����
    //Ӣ��������(m_star,m_level)
    int m_hero_attack;   //����
    int m_hero_defense;  //����
    int m_hero_magic;   //ħ��
    int m_hero_hp;       //����
    int m_state;        // 0 ����  1 Ĭ��Ӣ��2����Ӣ��
    int m_city;        //�س�Ӣ�۶�Ӧ�ǳ�

    int m_attribute;//ս������ֵ

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
    bool m_init_attr;//�Ƿ��ʼ��������
    bool m_changed;  //�иĶ�
    int addExp(int exp, int statistics_type = 0);
    int levelup(int level);
    int equipt(int slot, int eid);//����
    int unequipt(int slot);//ж��
    int useGem(int tid, int nums);//ʹ�õ���
    void updateAttribute(bool real_update = true);
    int Save();//����
    void toObj(json_spirit::Object& obj, int star = 0);
    void updateStar(int up_star);
    bool checkEquiptLevel(int level);
    bool isDefault() {return m_state == HERO_STATE_DEFAULT;}
    bool isWork() {return m_state > HERO_STATE_INIT;}
};

//���Ӣ������
struct CharTotalHeros
{
    std::map<int, boost::shared_ptr<CharHeroData> > m_heros;
    CharData& m_charData;
    int m_cid;              //��ɫid
    int m_default_hero;     //��սӢ��
    size_t m_hero_max;//���Ӣ����
    boost::shared_ptr<smeltHeroData> m_smeltHeros[iSmeltMaxCnt];  //��ǰ������Ӣ��

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
    int SetDefault(int hid, json_spirit::Object& obj);//���ó�սӢ��
    void getList(json_spirit::Array& hlist);
    //�ϳ�
    int CompoundHeroInfo(json_spirit::Object& obj, json_spirit::mObject& o);//�ϳ�Ӣ�۽�����Ϣ
    int CompoundHero(json_spirit::Object& obj, json_spirit::mObject& o);//�ϳ�Ӣ��
    //�ֽ�
    int DecomposeHeroInfo(json_spirit::Object& obj, json_spirit::mObject& o);//�ֽ�Ӣ�۽�����Ϣ
    int DecomposeHero(json_spirit::mObject& o);//�ֽ�Ӣ��
    //����
    int SmeltHeroInfo(json_spirit::Object& obj);//����Ӣ�۽�����Ϣ
    int SmeltHeroRefresh();//����Ӣ��ˢ��
    int SmeltHero(json_spirit::mObject& o, json_spirit::Object& robj);//����Ӣ��ˢ��
    //���
    int GoldenHeroInfo(json_spirit::Object& obj, json_spirit::mObject& o);//���Ӣ�۽���
    int GoldenHero(json_spirit::Object& obj, json_spirit::mObject& o);//���
    //������
    int UpEpicHeroInfo(json_spirit::Object& obj, json_spirit::mObject& o);
    int UpEpicHero(json_spirit::Object& obj, json_spirit::mObject& o);
    void updateAttribute();
    int Save(); //����
    bool m_changed; //�иĶ�
};

struct base_hero_pack
{
    int m_id;
    std::string m_name;//����
    int m_quality;//Ʒ��
    int m_level;//��Ҫ�ȼ�
    int m_vip;//��ҪVIP
    int m_silver_cost;//���뻨��
    int m_gold_cost;//��һ���
    int m_hero_cnt;//��������Ӣ��
    int m_race_pack;//�Ƿ������(0���ǣ�1-4��ʾ��Ӧ����)
    std::vector<int> m_gailvs;//���Ǽ�Ӣ�۸���
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
    std::vector<int> m_pack_heros; //���Կ������Ӣ��
    std::map<int, std::vector<int> > m_race_heros; //���Կ�����õ�����Ӣ��
    std::map<int, boost::shared_ptr<baseHeroData> > m_base_heros; //����Ӣ������
    std::vector<int> m_epic_heros; //��Ӣ��
    std::map<int, boost::shared_ptr<base_hero_pack> > m_base_heropacks; //Ӣ�۰�
};

//��ѯӢ����Ϣ
int ProcessGetHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʾ��ɫӢ���б�
int ProcessCharHeros(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//���ó�սӢ��
int ProcessSetDefaultHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡӢ�۰���Ϣ
int ProcessQueryHeroPack(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����Ӣ�۰�
int ProcessOpenHeroPack(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����Ӣ��λ
int ProcessBuyHeroSize(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ϳɸ�����Ϣ
int ProcessCompoundHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ϳ�Ӣ��
int ProcessCompoundHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ֽ�Ӣ����Ϣ
int ProcessDecomposeHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ֽ�Ӣ��
int ProcessDecomposeHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����Ӣ����Ϣ
int ProcessSmeltHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����Ӣ��ˢ��
int ProcessSmeltHeroRefresh(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����Ӣ��
int ProcessSmeltHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//���Ӣ����Ϣ
int ProcessGoldenHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//���Ӣ��
int ProcessGoldenHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʾ��Ӣ��
int ProcessEpicHeros(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ļ��
int ProcessGetEpicHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������
int ProcessUpEpicHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessUpEpicHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

