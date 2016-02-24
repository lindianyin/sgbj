
#include "utils_all.h"

#include "buff.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "errcode_def.h"
#include "data.h"
#include "statistics.h"
#include "spls_timer.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
int InsertInternalActionWork(json_spirit::mObject& obj);

//玩家限时增益数据
int ProcessQueryCharBuffs(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    json_spirit::Array list;
    cdata->m_Buffs.getBuffInfo(list);
    robj.push_back( Pair("buffs", list) );
    return HC_SUCCESS;
}

//限时增益状态改变
int ProcessBuffChange(json_spirit::mObject& o)
{
    int cid = 0;
    READ_INT_FROM_MOBJ(cid,o,"cid");
    //通知客户端
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (cdata.get())
    {
        cdata->m_Buffs.refresh();
    }
    return HC_SUCCESS;
}

void CharBuff::start(int cid)
{
    //开始任务计时器
    json_spirit::mObject mobj;
    mobj["cmd"] = "buffChange";
    mobj["cid"] = cid;
    int left_time = m_end_time-time(NULL);
    if (left_time <= 0)
        left_time = 0;
    boost::shared_ptr<splsTimer> tmsg;
    tmsg.reset(new splsTimer(left_time, 1, mobj,1));
    m_invalid_timer = splsTimerMgr::getInstance()->addTimer(tmsg);
}

CharBuffs::CharBuffs(CharData& cd)
:cdata(cd)
{
    for(int i = 0; i < 4; ++i)
    {
        buffs[i].m_type = i+1;
        buffs[i].m_value = 0;
        buffs[i].m_end_time = 0;
    }
    reload = true;
}

int CharBuffs::load()
{
    time_t t_now = time(NULL);
    Query q(GetDb());
    q.get_result("select type,value,endtime from char_buffs where cid=" + LEX_CAST_STR(cdata.m_id));
    while (q.fetch_row())
    {
        int type = q.getval();
        buffs[type-1].m_value = q.getval();
        buffs[type-1].m_end_time = q.getval();
        if (buffs[type-1].m_end_time > t_now)
        {
            buffs[type-1].start(cdata.m_id);
        }
    }
    q.free_result();
    return 0;
}

int CharBuffs::addBuff(int type, int value, int invalid_time)
{
    if (type < 1 || type > 4 || value <= 0)
        return HC_ERROR;
    time_t t_now = time(NULL);
    buffs[type-1].m_value = value;
    if (buffs[type-1].m_end_time <= t_now)
    {
        buffs[type-1].m_end_time = t_now+invalid_time;
    }
    else
    {
        buffs[type-1].m_end_time = buffs[type-1].m_end_time+invalid_time;
    }
    buffs[type-1].start(cdata.m_id);
    InsertSaveDb("replace into char_buffs (cid,type,value,endtime) values ("+LEX_CAST_STR(cdata.m_id)+","+LEX_CAST_STR(type)+","+LEX_CAST_STR(buffs[type-1].m_value)+","+LEX_CAST_STR(buffs[type-1].m_end_time)+")");
    cdata.m_heros.updateAttribute();
    return HC_SUCCESS;
}

void CharBuffs::clearBuff()
{
    for(int i = 0; i < 5; ++i)
    {        
        if (buffs[i].m_end_time > 0 && buffs[i].m_value > 0)
        {
            buffs[i].m_end_time = time(NULL);
        }
    }

    json_spirit::mObject mobj;
    mobj["cmd"] = "buffChange";
    mobj["cid"] = cdata.m_id;
    InsertInternalActionWork(mobj);
}

//刷新增益效果
void CharBuffs::refresh()
{
    json_spirit::Array change_list;
    time_t t_now = time(NULL);
    bool buff_change = false;
    for(int i = 0; i < 4; ++i)
    {
        if (buffs[i].m_end_time <= t_now && buffs[i].m_value > 0)
        {
            json_spirit::Object obj;
            buffs[i].m_value = 0;
            buffs[i].m_end_time = 0;
            buff_change = true;
            InsertSaveDb("replace into char_buffs (cid,type,value,endtime) values ("+LEX_CAST_STR(cdata.m_id)+","+LEX_CAST_STR(i+1)+","+LEX_CAST_STR(buffs[i].m_value)+","+LEX_CAST_STR(buffs[i].m_end_time)+")");
        }
    }
    if (buff_change)
    {
        //通知消失buff具体列表
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "buff_notify") );
        obj.push_back( Pair("s", 200) );
        cdata.sendObj(obj);
        cdata.m_heros.updateAttribute();
    }
    return;
}

//增益信息
int CharBuffs::getBuffInfo(json_spirit::Array& slist)
{
    if (reload)
    {
        load();
        reload = true;
    }
    refresh();
    for (int i = 0; i < 4; ++i)
    {
        json_spirit::Object obj;
        int left_time = buffs[i].m_end_time-time(NULL);
        obj.push_back( Pair("type", buffs[i].m_type) );
        obj.push_back( Pair("effect", int2percent(buffs[i].m_value,100)) );
        obj.push_back( Pair("end_time", left_time > 0 ? left_time : 0) );
        slist.push_back(obj);
    }
    return HC_SUCCESS;
}

