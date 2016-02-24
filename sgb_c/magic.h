
#pragma once

#include <string>
#include <map>
#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "const_def.h"
#include "json_spirit.h"
#include "net.h"

const int iMagicHeal[4][11] =
{
    {80, 70, 55, 50, 45, 40, 35, 30, 25, 20, 15},
    {85, 75, 60, 55, 50, 45, 40, 35, 30, 25, 20},
    {82, 72, 57, 52, 47, 42, 37, 32, 27, 22, 17},
    {100, 95, 75, 70, 65, 60, 55, 50, 45, 40, 35},
};

const int iMagicDamage[4][11] =
{
    {80, 70, 55, 50, 45, 40, 35, 30, 25, 20, 15},
    {85, 75, 60, 55, 50, 45, 40, 35, 30, 25, 20},
    {82, 72, 57, 52, 47, 42, 37, 32, 27, 22, 17},
    {100, 95, 75, 70, 65, 60, 55, 50, 45, 40, 35},
};

enum MAGIC_NORMAL_TYPE
{
    MAGIC_REFRESH = 1,
    MAGIC_SPADE = 2,
    MAGIC_JOKE = 3,
};

enum MAGIC_RACE_TYPE
{
    MAGIC_NOVA = 1,
    MAGIC_HEALTH = 2,
    MAGIC_DAMAGE = 3,
};

struct CharTotalMagics;
struct CharData;

//基础魔法
struct baseMagic
{
    int m_id;
    int m_spic;
    int m_race;
    int m_type;
    int m_combat_level;//出战等级
    std::string m_name;
    std::string m_memo;
    void toObj(json_spirit::Object& obj);
};

struct CharTotalMagics
{
    CharData& m_charData;
    int m_cid;
    int m_combat_magics[3];

    CharTotalMagics(int cid, CharData& cdata)
    :m_charData(cdata)
    {
        m_cid = cid;
        m_changed = false;
        m_combat_magics[0] = 0;
        m_combat_magics[1] = 0;
        m_combat_magics[2] = 0;
    };
    int Load();
    bool inCombat(int id);
    int SetCombatMagic(json_spirit::mObject& o, json_spirit::Object& obj);//设置出战技能
    int CancelCombatMagic(json_spirit::mObject& o, json_spirit::Object& obj);//取消出战技能
    int SwapCombatMagic(json_spirit::mObject& o, json_spirit::Object& obj);//交换出战技能
    int getCombatMagics(json_spirit::Array& list);
    int upgradeMagicInfo(int magic_id, json_spirit::Object& obj);
    int Save(); //保存
    bool m_changed; //有改动
};

class MagicMgr
{
public:
	MagicMgr();
	//获得基础状态
	boost::shared_ptr<baseMagic> getBaseMagic(int id);
	int RandomMagic();
    int getMagicList(json_spirit::Array& list, int race);
private:
    std::vector<int> m_magics;
    std::map<int, boost::shared_ptr<baseMagic> > m_base_magics;		//基础技能数据
};

//查询技能信息
int ProcessGetMagicInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//显示角色技能列表
int ProcessMagicList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//设置出战技能
int ProcessSetCombatMagic(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//取消出战技能
int ProcessCancelCombatMagic(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//交换出战技能
int ProcessSwapCombatMagic(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//显示角色出战技能列表
int ProcessCharCombatMagics(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

