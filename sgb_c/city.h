#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "data.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

enum city_building_type_enum
{
    BUILDING_TYPE_BASE = 0,
    BUILDING_TYPE_CASTLE = 1,
    BUILDING_TYPE_METALLURGY = 2,
    BUILDING_TYPE_SMITHY = 3,
    BUILDING_TYPE_BARRACKS = 4,
    BUILDING_TYPE_COPY = 5,
    BUILDING_TYPE_RACE = 6,
    BUILDING_TYPE_SHENJIANG = 7,
    BUILDING_TYPE_SHENLING = 8,
    BUILDING_TYPE_AUCTION = 9,
    BUILDING_TYPE_MAX = 9,
};

struct base_city_building_info
{
    int type;
    int open_level;
    std::string name;
    int x;
    int y;
};

struct base_castle
{
    int m_level;//�Ǳ��ȼ�
    int m_need_level;//��������ȼ�
    int m_need_cost;//�������ĳ���
    int m_min_resident;//������Сֵ
    int m_max_resident;//�������ֵ
    int m_output_resident;//ÿ����ļ������
    int m_output_silver;//ÿ��˰�ղ�������
    int m_rob_defense;//�Ӷ����ֵ�����Ӷ���ʧ
    int m_attack_per;//�Ƿ��ӳ�
    int m_defense_per;//�Ƿ��ӳ�
    int m_magic_per;//�Ƿ��ӳ�
    int m_hp_per;//�Ƿ��ӳ�
};

struct char_castle
{
    int m_level;//�ȼ�
    int m_resident;//��������
    int m_recruit_cd;//��ļ��ȴʱ��
    int m_levy;//˰����ȡ���0δ��ȡ1����ȡ
    CharData& m_chardata;//�������
    boost::uuids::uuid _uuid;    //��ʱ��Ψһid
    int levelup(json_spirit::Object& robj);//�����Ǳ�����Ҳ�����
    int recruit(json_spirit::Object& robj);//��ļ������Ҳ�����
    void resident_away();//������ʧ��ϵͳ��ʱ��
    void cal_add(int& att_per, int& def_per, int& levy_per);// ���ݾ���ٷֱȻ�ù�������˰�ռӳ�ֵ
    int gerRobDefense();
    void getDefenseAdd(int& att_per, int& def_per, int& magic_per, int& hp_per);
    int levy_get();
    int levy_left_times();
    int levy(bool cost, json_spirit::Object& robj);//˰���Լ�ǿ��˰�գ���Ҳ�����
    void toObj(json_spirit::Object& robj);
    void save();

    char_castle(CharData& chardata)
    :m_chardata(chardata)
    {
    }
};

struct base_metallurgy
{
    int m_level;//���𷿵ȼ�
    int m_need_level;//��������ȼ�
    int m_need_cost;//�������ĳ���
    double m_compound_add;//�ϳɳɹ��� �ӳ�
    int m_compound_star;//�ɺϳ�Ӣ���Ǽ�
    int m_decompose_star;//�ɷֽ�Ӣ���Ǽ�
    int m_golden_star;//�ɵ��Ӣ���Ǽ�
    int m_smelt_cnt;//������������
};

struct char_metallurgy
{
    int m_level;//�ȼ�
    CharData& m_chardata;//�������
    boost::uuids::uuid _uuid;    //��ʱ��Ψһid
    int levelup(json_spirit::Object& robj);//����
    double getCompoundAdd();
    int getCompoundMaxStar();
    int getDecomposeMaxStar();
    int getGoldenMaxStar();
    int getSmeltCnt();
    void toObj(json_spirit::Object& robj);
    void save();

    char_metallurgy(CharData& chardata)
    :m_chardata(chardata)
    {
    }
};

struct base_smithy
{
    int m_level;//�����̵ȼ�
    int m_need_level;//��������ȼ�
    int m_need_cost;//�������ĳ���
    double m_compound_add;//�ϳɳɹ��� �ӳ�
};

struct char_smithy
{
    int m_level;//�ȼ�
    CharData& m_chardata;//�������
    int levelup(json_spirit::Object& robj);//����
    double getCompoundAdd();
    void toObj(json_spirit::Object& robj);
    void save();

    char_smithy(CharData& chardata)
    :m_chardata(chardata)
    {
    }
};

struct base_barracks
{
    int m_level;//��Ӫ�ȼ�
    int m_need_level;//��������ȼ�
    int m_need_cost;//�������ĳ���
    double m_add;//�ӳ�
};

struct char_barracks
{
    int m_level;//�ȼ�
    CharData& m_chardata;//�������
    int levelup(json_spirit::Object& robj);//����
    double getAdd();
    void toObj(json_spirit::Object& robj);
    void save();

    char_barracks(CharData& chardata)
    :m_chardata(chardata)
    {
    }
};

class cityMgr
{
public:
    cityMgr();
    void getButton(CharData* pc, json_spirit::Array& list);
    boost::shared_ptr<base_city_building_info> getBuildingInfo(int type);
    base_castle* getCastle(int level);
    char_castle* getCharCastle(int cid);
    void residentAway();//������ʧ��ϵͳ��ʱ��
    void resetLevy();//����˰�գ�ϵͳ��ʱ��
    boost::shared_ptr<base_metallurgy> getMetallurgy(int level);
    int getSmeltOpenLevel(int cnt);
    boost::shared_ptr<char_metallurgy> getCharMetallurgy(int cid);
    boost::shared_ptr<base_smithy> getSmithy(int level);
    boost::shared_ptr<char_smithy> getCharSmithy(int cid);
    boost::shared_ptr<base_barracks> getBarracks(int level);
    boost::shared_ptr<char_barracks> getCharBarracks(int cid);
private:
    int m_max_castle_level;
    int m_max_metallurgy_level;
    int m_max_smithy_level;
    int m_max_barracks_level;
    base_castle _base_castle[100];//�Ǳ��б�
    std::vector<boost::shared_ptr<base_metallurgy> > base_metallurgy_list;//�����б�
    std::vector<boost::shared_ptr<base_smithy> > base_smithy_list;//�������б�
    std::vector<boost::shared_ptr<base_barracks> > base_barracks_list;//��Ӫ�б�
    std::map<int, boost::shared_ptr<char_castle> > m_char_castles;
    std::map<int, boost::shared_ptr<char_metallurgy> > m_char_metallurgys;
    std::map<int, boost::shared_ptr<char_smithy> > m_char_smithys;
    std::map<int, boost::shared_ptr<char_barracks> > m_char_barracks;
    
    std::vector<boost::shared_ptr<base_city_building_info> > base_buildinginfo_list;//�ǳ�������Ϣ
};

//��ȡ���ڽ����б�
int ProcessQueryCityBuildingList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ���ڽ�����Ϣ
int ProcessQueryCityBuilding(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�������ڽ���
int ProcessLevelUpCityBuilding(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ļ����
int ProcessRecruit(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ļ��ť����
int ProcessCityRecruitUpdate(json_spirit::mObject& o);
//�Ǳ���˰
int ProcessLevy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����ˢ�°�ť����
int ProcessSmeltRefreshUpdate(json_spirit::mObject& o);

