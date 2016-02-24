
#include "utils_all.h"

#include "state.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "spls_errcode.h"
#include "data.h"
#include "statistics.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

int random_state_level(int vip_level = 0, int type = 1)
{
    if (vip_level <= 4 || 1 == type)
    {
        int rand = my_random(1,100);
        if (rand <= 50)
        {
            return 1;
        }
        else if (rand <= 80)
        {
            return 2;
        }
        else
        {
            return 3;
        }
    }
    else if (vip_level <= 8)
    {
        int rand = my_random(1,100);
        if (rand <= 80)
        {
            return 2;
        }
        else
        {
            return 3;
        }
    }
    else
    {
        return 3;
    }
}

baseStateMgr* baseStateMgr::m_handle = NULL;

baseStateMgr::baseStateMgr()
{
    load();
}

baseStateMgr::~baseStateMgr()
{
}

int baseStateMgr::load()
{
    Query q(GetDb());
    //基础状态
    q.get_result("SELECT id,type,effect_type,effect,gailv,name,memo FROM base_states WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseState> st = GetBaseState(id);
        if (!st.get())
        {
            st.reset(new baseState);
        }
        m_base_states[id] = st;
        st->id = id;
        st->type = q.getval();
        st->effect_type = q.getval();
        st->effect_value = q.getval();
        st->effect_gailv = q.getval() * 10;
        st->name = q.getstr();
        st->memo = q.getstr();
    }
    q.free_result();

    for (int i = 0; i < max_refresh_state_types; ++i)
    {
        boost::shared_ptr<baseState> st = GetBaseState(3*i+3);
        if (!st.get())
        {
            ERR();
            continue;
        }
        m_state_obj[i].push_back( Pair("id", i+1) );
        //m_state_obj[i].push_back( Pair("name", st->name) );
        //m_state_obj[i].push_back( Pair("memo", st->memo) );
        m_state_obj[i].push_back( Pair("type", st->type) );
        m_state_obj[i].push_back( Pair("spic", i+1) );
    }
    
    for (int i = 1; i <= 8; ++i)
    {
        m_state_sets[0].push_back(i);
    }
    for (int i = 1; i <= 16; ++i)
    {
        m_state_sets[1].push_back(i);
    }
    for (int i = 1; i <= 24; ++i)
    {
        m_state_sets[2].push_back(i);
    }
    return 0;
}

//获得基础状态
boost::shared_ptr<baseState> baseStateMgr::GetBaseState(int id)
{
    std::map<int, boost::shared_ptr<baseState> >::iterator it = m_base_states.find(id);
    if (it != m_base_states.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<baseState> gd;
        gd.reset();
        return gd;
    }
}

baseStateMgr* baseStateMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new baseStateMgr();
    }
    return m_handle;
}

static inline int getEffect(int id, int level)
{
    if (5 == id)
    {
        return 20 * level;
    }
    else if (7 == id)
    {
        return level;
    }
    return 5 * level;
}

static inline std::string getValue(int id, int v)
{
    if (7 == id)
    {
        return LEX_CAST_STR(v);
    }
    else
    {
        return int2percent(v,1000);
    }
}

newCharStates::newCharStates(CharData& cd)
:cdata(cd)
{
    _enable = false;
    _star_level = 0;    //星图等级
    memset(_stars, 0, sizeof(int)*8);        //破军，武曲，廉贞，文曲，禄存，巨门，贪狼，紫薇
    memset(_effects, 0, sizeof(int)*8);        //状态效果
    memset(_cur_state, 0, sizeof(int)*3);    //当前状态 1-8
}

#if 0
int newCharStates::load()            //从数据库加载
{
    if (cdata.m_level <iFirstStateLevel)
    {
        return 0;
    }
    Query q(GetDb());
    q.get_result("select star_map,star1,star2,star3,star4,star5,star6,star7,star8,state1,state2,state3 from char_new_states where cid=" + LEX_CAST_STR(cdata.m_id));
    if (q.fetch_row())
    {
        _star_level = q.getval();
        _stars[0] = q.getval();
        _stars[1] = q.getval();
        _stars[2] = q.getval();
        _stars[3] = q.getval();
        _stars[4] = q.getval();
        _stars[5] = q.getval();
        _stars[6] = q.getval();
        _stars[7] = q.getval();
        _cur_state[0] = q.getval();
        _cur_state[1] = q.getval();
        _cur_state[2] = q.getval();

        for (int i = 0; i < 8; ++i)
        {
            _effects[i] = getEffect(i+1, _star_level + _stars[i]);
        }
        q.free_result();
    }
    else
    {
        q.free_result();
        init();
        refresh();
    }
    _enable = true;
    return 0;
}

int newCharStates::init()        //首次开启初始化
{
    InsertSaveDb("replace into char_new_states (cid,star_map,star1,star2,star3,star4,star5,star6,star7,star8,state1,state2,state3) values ("
            + LEX_CAST_STR(cdata.m_id) + ",0,0,0,0,0,0,0,0,0,1,0,0)");
    _star_level = 0;
    _stars[0] = 0;
    _stars[1] = 0;
    _stars[2] = 0;
    _stars[3] = 0;
    _stars[4] = 0;
    _stars[5] = 0;
    _stars[6] = 0;
    _stars[7] = 0;
    _cur_state[0] = 1;
    _cur_state[1] = 0;
    _cur_state[2] = 0;
    _enable = true;
    return 0;
}

int newCharStates::levelup()        //升级或者斗转星移
{
    if (_star_level >= iMaxStateStarLevel)
    {
        return HC_ERROR;
    }
    //地图等级是否足够
    if (_star_level >= (cdata.m_area-1) * 5)
    {
        return HC_ERROR;
    }
    int levelup_pos = 0;
    for (int i = 0; i < 8; ++i)
    {
        if (_stars[i] <= _star_level)
        {
            levelup_pos = i + 1;
            break;
        }
    }
    //斗转星移
    if (levelup_pos == 0)
    {
        ++_star_level;

        for (int i = 0; i < 8; ++i)
        {
            _effects[i] = getEffect(i+1, _star_level + _stars[i]);
        }
        //保存到db
        InsertSaveDb("update char_new_states set star_map=" + LEX_CAST_STR(_star_level) + " where cid=" + LEX_CAST_STR(cdata.m_id));
    }
    else
    {
        int silver = iStarLevelupCost[_star_level];
        //银币消耗
        if (cdata.addSilver(-silver) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_SILVER;
        }
        //银币消耗统计
        add_statistics_of_silver_cost(cdata.m_id, cdata.m_ip_address, silver, silver_cost_for_upgrade_star);

        cdata.NotifyCharData();

        ++_stars[levelup_pos - 1];
        _effects[levelup_pos - 1] = getEffect(levelup_pos, _star_level + _stars[levelup_pos - 1]);
        //保存到db
        InsertSaveDb("update char_new_states set star" + LEX_CAST_STR(levelup_pos) + "=" + LEX_CAST_STR(_stars[levelup_pos-1]) + " where cid=" + LEX_CAST_STR(cdata.m_id));
    }
    return HC_SUCCESS;
}

int newCharStates::refresh(int type, int pos/* = 1*/)        //玩家刷新状态
{
    //状态未开放
    if (cdata.m_level < iFirstStateLevel)
    {
        return 0;
    }

    int max_state = 8;
    if (_star_level == 0)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (_stars[i] == 0)
            {
                max_state = i;
                break;
            }
        }
    }

    int state_nums = 1;
    //根据vip等级确定状态个数
    if (cdata.m_vip >= 4 || cdata.m_prestige >= 80000)
    {
        state_nums = 3;
    }
    else if (cdata.m_vip >= 2 || cdata.m_prestige >= 30000)
    {
        state_nums = 2;
    }

    if (pos > state_nums || pos < 1)
    {
        pos = 1;
    }

    int new_state = 0;
    if (state_nums >= max_state)
    {
        for (int i = 0; i < state_nums; ++i)
        {
            _cur_state[i] = i + 1;
        }
        return _stars[0];
    }
    if (type)
    {
        int silverCost = 0, goldCost = 0;
        getRefreshStateCost(cdata.m_area, silverCost, goldCost);
        switch (type)
        {
            case 1:
                if (cdata.addSilver(silverCost) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_SILVER;
                }
                //银币消耗统计
                add_statistics_of_silver_cost(cdata.m_id, cdata.m_ip_address,-silverCost,silver_cost_for_refresh_state);
                cdata.NotifyCharData();
                break;
            case 2:
                if (cdata.m_vip < iRefreshStateWithGold_VIP_level)
                {
                    return HC_ERROR_MORE_VIP_LEVEL;
                }
                if (cdata.addGold(goldCost)<0)
                {
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                }
                //金币消耗统计
                add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, -goldCost, gold_cost_for_refresh_state);
                cdata.NotifyCharData();
                break;
        }
    }
    //_cur_state[pos-1] = 0;
    new_state = my_random(1, max_state);
    //cout<<"1 rand state "<<new_state<<endl;
    int loop_times = 0;
    for (;;)
    {
        ++loop_times;
        if (loop_times > 50)
        {
            break;
        }
        bool success = true;
        for (int i = 0; i < state_nums; ++i)
        {
            if (new_state == _cur_state[i])
            {
                success = false;
                break;
            }
        }
        if (success)
        {
            break;
        }
        else
        {
            new_state = my_random(1, max_state);
            //cout<<"2 rand state "<<new_state<<endl;
        }
    }
    _cur_state[pos-1] = new_state;
    //cout<<pos<<",3 rand state "<<new_state<<endl;
    //保存到db
    InsertSaveDb("update char_new_states set state" + LEX_CAST_STR(pos) + "=" + LEX_CAST_STR(new_state) + " where cid=" + LEX_CAST_STR(cdata.m_id));
    return _stars[new_state-1];
}

//战斗后系统刷新状态，顶掉前面一个，最后一个随机
int newCharStates::refresh()
{
    //状态未开放
    if (cdata.m_level < iFirstStateLevel)
    {
        return 0;
    }

    int max_state = 8;
    if (_star_level == 0)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (_stars[i] == 0)
            {
                max_state = i;
                break;
            }
        }
    }    
    int state_nums = 1;
    int pos = 1;
    //根据vip等级确定状态个数
    if (cdata.m_vip >= 4 || cdata.m_prestige >= 80000)
    {
        state_nums = 3;
        int temp = _cur_state[0];
        _cur_state[0] = _cur_state[1];
        _cur_state[1] = _cur_state[2];
        _cur_state[2] = temp;
        pos = 3;
    }
    else if (cdata.m_vip >= 2 || cdata.m_prestige >= 30000)
    {
        state_nums = 2;
        int temp = _cur_state[0];
        _cur_state[0] = _cur_state[1];
        _cur_state[1] = temp;
        pos = 2;
    }
    else
    {
        pos = 1;
    }

    if (state_nums >= max_state)
    {
        for (int i = 0; i < state_nums; ++i)
        {
            _cur_state[i] = i + 1;
        }
        return _stars[0];
    }

    int new_state = my_random(1, max_state);
    {
        //_cur_state[pos-1] = 0;
        int loop_times = 0;
        for (;;)
        {
            ++loop_times;
            if (loop_times > 50)
            {
                break;
            }
            bool success = true;
            for (int i = 0; i < state_nums; ++i)
            {
                if (new_state == _cur_state[i])
                {
                    success = false;
                    break;
                }
            }
            if (success)
            {
                break;
            }
            else
            {
                new_state = my_random(1, max_state);
            }
        }
        _cur_state[pos-1] = new_state;

        for (int i = 0; i < state_nums; ++i)
        {
            if (_cur_state[i] == 0)
            {
                refresh(0, i + 1);
            }
        }
    }
    //保存到db
    InsertSaveDb("update char_new_states set state1=" + LEX_CAST_STR(_cur_state[0])
        + ",state2=" + LEX_CAST_STR(_cur_state[1])
        + ",state3=" + LEX_CAST_STR(_cur_state[2])
    + " where cid=" + LEX_CAST_STR(cdata.m_id));
    return _stars[new_state-1];    
}

//北斗7星信息
int newCharStates::getStarInfo(json_spirit::Object& robj)
{
    if (!_enable && cdata.m_level >= iFirstStateLevel)
    {
        init();
        refresh();
    }
    //星图等级
    json_spirit::Object info;
    info.push_back( Pair("level", _star_level) );
    json_spirit::Array slist;
    for (int i = 0; i < 8; ++i)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("id", i+1) );
        obj.push_back( Pair("level", _stars[i]) );
        obj.push_back( Pair("effect", getValue(i+1, getEffect(i+1,_stars[i] + _star_level))) );
        slist.push_back(obj);
    }
    robj.push_back( Pair("list", slist) );
    int canDo = 1;    // 1 能升级 2 能斗转星移 3 不能升级，银币不足 4到达当前地图上限等级 5无法再升级了
    int silver = 0;
    if (_star_level >= iMaxStateStarLevel)
    {
        canDo = 5;
    }
    else if (_star_level >= (cdata.m_area-1) * 5)
    {
        canDo = 4;
    }
    else
    {
        int levelup_pos = 0;
        for (int i = 0; i < 8; ++i)
        {
            if (_stars[i] <= _star_level)
            {
                levelup_pos = i + 1;
                break;
            }
        }
        //斗转星移
        if (levelup_pos == 0)
        {
            canDo = 2;
        }
        else
        {
            silver = iStarLevelupCost[_star_level];
            if (cdata.silver() >= silver)
            {
                canDo = 1;
            }
            else
            {
                canDo = 3;
            }
        }
    }
    //需要银币
    info.push_back( Pair("silver", silver) );
    info.push_back( Pair("canDo", canDo) );

    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

//状态刷新信息
int newCharStates::getStateInfo(json_spirit::Array& slist)
{
    if (!_enable && cdata.m_level >= iFirstStateLevel)
    {
        init();
        refresh();
    }
    else if (cdata.m_level >= iFirstStateLevel)
    {
        //根据vip等级确定状态个数
        if (cdata.m_vip >= 4 || cdata.m_prestige >= 80000)
        {
            if (_cur_state[1] == 0)
            {
                refresh(0, 2);
            }
            if (_cur_state[2] == 0)
            {
                refresh(0, 3);
            }
        }
        else if (cdata.m_vip >= 2 || cdata.m_prestige >= 30000)
        {
            if (_cur_state[1] == 0)
            {
                refresh(0, 2);
            }
        }
    }
    for (int i = 0; i < 3; ++i)
    {
        if (_cur_state[i] >=1 && _cur_state[i] <= 8)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", _cur_state[i]) );
            obj.push_back( Pair("level", _stars[_cur_state[i] - 1]) );
            obj.push_back( Pair("effect", getValue(_cur_state[i], _effects[_cur_state[i]-1])) );
            slist.push_back(obj);
        }
    }
    return HC_SUCCESS;
}

int newCharStates::getCostInfo(json_spirit::Object& info)
{
    int silverCost = 0, goldCost = 0;
    getRefreshStateCost(cdata.m_area, silverCost, goldCost);
    info.push_back( Pair("silver", -silverCost) );
    info.push_back( Pair("gold", -goldCost) );
    return HC_SUCCESS;
}
#endif

npcStrongholdStates::npcStrongholdStates(int cid, int stronghold, int level, int num)
:_cid(cid)
,_stronghold(stronghold)
,_level(level)
,_state_num(num)
{
    memset(_cur_state, 0, 3*sizeof(int));
    memset(_effects, 0, 3*sizeof(int));
    m_need_refresh = true;
}

int npcStrongholdStates::load()
{
    if (_state_num == 0)
    {
        return 0;
    }
    refresh();
    return _state_num;
#if 0    
    int num = 0;
    Query q(GetDb());
    q.get_result("SELECT state1,state2,state3 FROM char_stronghold_states WHERE cid=" + LEX_CAST_STR(_cid)
                                        + " AND strongholdid=" + LEX_CAST_STR(_stronghold));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        _cur_state[0] = q.getval();
        _cur_state[1] = q.getval();
        _cur_state[2] = q.getval();

        for (int i = 0; i < 3; ++i)
        {
            if (_cur_state[i])
            {
                _effects[i] = getEffect(_cur_state[i], 2*_level);
                ++num;
            }
            else
            {
                _effects[i] = 0;
            }
        }
    }
    q.free_result();
    return num;
#endif
}

int npcStrongholdStates::refresh()
{
    m_need_refresh = false;
    if (_state_num == 0)
    {
        return HC_SUCCESS;
    }
    else if (1 == _state_num)
    {
        _cur_state[0] = my_random(1, 8);
        _effects[0] = getEffect(_cur_state[0], 2*_level);
    }
    else if (2 == _state_num)
    {
        _cur_state[0] = my_random(1, 8);
        _effects[0] = getEffect(_cur_state[0], 2*_level);
        for (;;)
        {
            _cur_state[1] = my_random(1, 8);
            if (_cur_state[1] != _cur_state[0])
            {
                _effects[1] = getEffect(_cur_state[1], 2*_level);
                break;
            }
        }
    }
    else
    {
        _cur_state[0] = my_random(1, 8);
        _effects[0] = getEffect(_cur_state[0], 2*_level);
        for (;;)
        {
            _cur_state[1] = my_random(1, 8);
            if (_cur_state[1] != _cur_state[0])
            {
                _effects[1] = getEffect(_cur_state[1], 2*_level);
                break;
            }
        }
        for (;;)
        {
            _cur_state[2] = my_random(1, 8);
            if (_cur_state[2] != _cur_state[0] && _cur_state[2] != _cur_state[1])
            {
                _effects[2] = getEffect(_cur_state[2], 2*_level);
                break;
            }
        }
    }
    /*
    InsertSaveDb("replace into char_stronghold_states (cid,strongholdid,state1,state2,state3) values ("
                                        + LEX_CAST_STR(_cid) + "," + LEX_CAST_STR(_stronghold)
                                        + LEX_CAST_STR(_cur_state[0]) + ","
                                        + LEX_CAST_STR(_cur_state[1]) + ","
                                        + LEX_CAST_STR(_cur_state[2]) + ","
                                         + ")");*/
    return HC_SUCCESS;
}

//状态信息
int npcStrongholdStates::getStateInfo(json_spirit::Array& slist)
{
    if (m_need_refresh)
    {
        refresh();
    }
    for (int i = 0; i < _state_num; ++i)
    {
        if (_cur_state[i] >= 1 && _cur_state[i] <= 8)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", _cur_state[i]) );
            obj.push_back( Pair("level", _level) );
            obj.push_back( Pair("effect", getValue(_cur_state[i], _effects[i])) );
            slist.push_back(obj);
        }
    }
    return HC_SUCCESS;
}

