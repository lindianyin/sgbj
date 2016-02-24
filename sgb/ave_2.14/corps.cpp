
#include "corps.h"
#include "spls_errcode.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "utils_lang.h"
#include "net.h"
#include "statistics.h"
#include "text_filter.h"
#include "daily_task.h"
#include "spls_race.h"
#include "relation.h"
#include "singleton.h"

#include "corpsExplore.h"
#include "spls_timer.h"
#include "json_spirit_writer_template.h"
#include "rewards.h"
#include "qq_invite.h"
#include "corpsFighting.hpp"
#include "boss.h"

using namespace net;

extern void InsertSaveDb(const std::string& sql);
extern Database& GetDb();

extern int getSessionChar(session_ptr& psession, CharData* &pc);
extern int getSessionChar(session_ptr& psession, boost::shared_ptr<CharData>& cdata);

extern std::string strCorpsMsgJoin;
extern std::string strCorpsMsgFire;
extern std::string strCorpsMsgLeave;
extern std::string strCorpsMsgExplore;
extern std::string strCoprsYmsjRefresh;
extern std::string strCoprsYmsjMsg;
extern std::string strCoprsYmsjMsg2;
extern std::string strGongxun;

//const std::string strZhaoji = "军团宴会还需$N人<font color=\"#00ff00\"><a href=\"$E\"><u>[立即参与]</u></a>";
const std::string strZhaojiUrl = "event:{'cmd':'openParty'}";

//一个小时自动获胜
const int iCorpsYmsjTimeout = 3600;

//辕门射戟刷新时间
const int iCoprsYmsjRefresh = 1800;

//辕门射戟选择时间
const int iCorpsYmsjChooseTimer = 60;

void corpsRealReward(int& get);

//辕门射戟
//const std::string strJoinYmsj = "event:{'cmd':'openYmsj'}";

void broadCastCorpsMsg(int corps, const std::string& msg)
{
    boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(corps);
    if (gch)
    {
        std::string smsg = "{\"cmd\":\"chat\",\"ctype\":3,\"s\":200,\"m\":\"" + json_spirit::add_esc_chars<std::string>(msg, true) + "\"}";
        gch->BroadMsg(smsg);
    }    
}

bool compare_corps(boost::shared_ptr<splsCorps> a, boost::shared_ptr<splsCorps> b)
{
    if (!a.get())
    {
        return false;
    }
    if (!b.get())
    {
        return true;
    }
    if (a->_level > b->_level)
    {
        return true;
    }
    else if (a->_level < b->_level)
    {
        return false;
    }
    else
    {
        return (a->_exp > b->_exp);
    }
}

bool compare_corps_member(boost::shared_ptr<corps_member> a, boost::shared_ptr<corps_member> b)
{
    if (!a.get())
    {
        return false;
    }
    if (!b.get())
    {
        return true;
    }
    if (a->offical == 1)
    {
        return true;
    }
    else if (b->offical == 1)
    {
        return false;
    }
    else if (a->offical != b->offical)
    {
        return a->offical > b->offical;
    }
    else if (a->contribution != b->contribution)
    {
        return (a->contribution > b->contribution);
    }
    else
    {
        return (a->join_time < b->join_time);
    }
}

int corps_member::save()
{
    InsertSaveDb("update char_corps_members set corps=" + LEX_CAST_STR(corps)
            + ",offical=" + LEX_CAST_STR(offical)
            + ",contribution=" + LEX_CAST_STR(contribution)
            + ",contributionToday=" + LEX_CAST_STR(contribution_day)
            + ",joinTime=" + LEX_CAST_STR(join_time)
            + ",ymsj_gongxun=" + LEX_CAST_STR(ymsj_can_get)
            + " where cid=" + LEX_CAST_STR(cid));
    return 0;
}

corps_ymsj::corps_ymsj(splsCorps& c)
:m_corps(c)
{
    //m_id = m_corps._id;
    m_choice[0] = 0;
    m_choice[1] = 0;
    m_join_time[0] = 0;
    m_join_time[1] = 0;
    next_time = 0;
    m_choose_timer[0] = boost::uuids::nil_uuid();
    m_choose_timer[1] = boost::uuids::nil_uuid();

    m_done_timer = boost::uuids::nil_uuid();
}

void corps_ymsj::choose_timer(int pos)
{
    json_spirit::mObject mobj;
    mobj["cmd"] = "YmsjChoose";
    mobj["id"] = m_corps._id;
    mobj["cid"] = m_char[pos-1]->cid;
    mobj["time"] = m_join_time[pos-1];

    //60秒内作出选择
    boost::shared_ptr<splsTimer> tmsg;
       tmsg.reset(new splsTimer(iCorpsYmsjChooseTimer, 1, mobj,1));
       m_choose_timer[pos-1] = splsTimerMgr::getInstance()->addTimer(tmsg);
}

void corps_ymsj::win_timer(int pos)
{
    if (m_char[pos-1].get())
    {
        json_spirit::mObject mobj;
        mobj["cmd"] = "YmsjWin";
        mobj["id"] = m_corps._id;
        mobj["cid"] = m_char[pos-1]->cid;
        mobj["time"] = m_join_time[pos-1];

        int left = m_join_time[pos-1] + iCorpsYmsjTimeout - time(NULL);
        if (left < 0)
        {
            left = 0;
        }

        //cout<<"win timer "<<left<<endl;
        //加入后一个小时没人应战直接胜利
        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(left, 1, mobj,1));
        m_done_timer = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
}

int corps_ymsj::join(int pos, boost::shared_ptr<corps_member> pc, json_spirit::Object& robj)
{
    if (NULL == pc.get() || pc->cdata.get() == NULL)
    {
        return HC_ERROR;
    }
    if (pc->ymsj_can_get > 0)
    {
        return HC_ERROR_CORPS_YMSJ_GET_AWARD_FIRST;
    }
    if (time(NULL) < next_time)
    {
        return HC_ERROR;
    }
    if (m_char[0] == pc || m_char[1] == pc)
    {
        return HC_ERROR;
    }
    if (pos > 2 || pos < 1)
    {
        if (m_char[0].get() == NULL)
        {
            pos = 1;
        }
        else if (m_char[1].get() == NULL)
        {
            pos = 2;
        }
        else
        {
            return HC_ERROR;
        }
    }
    else if (m_char[pos-1].get())
    {
        if (m_char[2-pos].get() == NULL)
        {
            pos = 3 - pos;
        }
        else
        {
            return HC_ERROR;
        }
    }
    if (pc->cdata->queryExtraData(char_data_type_daily, char_data_daily_corps_ymsj) >= 3)
    {
        //没有次数了
        return HC_ERROR;
    }

    if (m_done_timer.is_nil() == false)
    {
        splsTimerMgr::getInstance()->delTimer(m_done_timer);
        m_done_timer = boost::uuids::nil_uuid();
    }

    m_char[pos-1] = pc;
    m_join_time[pos-1] = time(NULL);

    choose_timer(pos);

    Save();
    robj.push_back( Pair("pos", pos) );
    return HC_SUCCESS;
}

int corps_ymsj::leave(int cid)
{
    int idx = 0;
    if (m_char[0].get() && m_char[0]->cid == cid)
    {
        idx = 0;
    }
    else if (m_char[1].get() && cid == m_char[1]->cid)
    {
        idx = 1;
    }
    else
    {
        return HC_SUCCESS;
    }
    m_char[idx].reset();
    m_choice[idx] = 0;
    m_join_time[idx] = 0;
    splsTimerMgr::getInstance()->delTimer(m_choose_timer[idx]);
    m_choose_timer[idx] = boost::uuids::nil_uuid();

    //另一个位置有人的，倒计时直接胜利
    win_timer(2-idx);    

    Save();
}

int corps_ymsj::choose(int cid, int c)
{
    int idx = 0;
    if (m_char[0].get() && m_char[0]->cid == cid)
    {
        idx = 0;
    }
    else if (m_char[1].get() && m_char[1]->cid == cid)
    {
        idx = 1;
    }
    else
    {
        return HC_ERROR;
    }
    if (m_choice[idx])
    {
        //已经选择了
        return HC_ERROR;
    }
    if (c > 2 || c < 1)
    {
        c = my_random(1,2);
    }
    m_choice[idx] = c;

    if (m_choose_timer[idx].is_nil() == false)
    {
        splsTimerMgr::getInstance()->delTimer(m_choose_timer[idx]);
        m_choose_timer[idx] = boost::uuids::nil_uuid();
    }

    if (m_choice[1-idx] > 0)
    {
        if (0 == Done())
        {            
            Save();
        }
    }
    else
    {
        //选择的时候对方没人，开始胜利倒计时
        if (m_char[1-idx].get() == NULL)
        {
            //胜利倒计时
            win_timer(1+idx);
        }
        Save();
    }
    return HC_SUCCESS;
}

void corps_ymsj::load(const std::string& data)
{
    if (data == "")
    {
        return;
    }
    json_spirit::mValue value;
    json_spirit::read(data, value);
    if (value.type() == json_spirit::obj_type)
    {
        mObject& mobj = value.get_obj();
        //READ_INT_FROM_MOBJ(m_id,mobj,"id");
        READ_INT_FROM_MOBJ(next_time,mobj,"next");

        json_spirit::mArray list;
        READ_ARRAY_FROM_MOBJ(list,mobj,"list");

        int idx = 0;
        for (json_spirit::mArray::iterator it = list.begin(); it != list.end(); ++it)
        {
            json_spirit::mValue& v = *it;
            if (v.type() == json_spirit::obj_type)
            {
                json_spirit::mObject& o = v.get_obj();
                int cid = 0;
                READ_INT_FROM_MOBJ(cid,o,"id");
                if (cid)
                {
                    m_char[idx] = m_corps.getMember(cid);
                }
                if (m_char[idx].get() == NULL)
                {
                    ++idx;
                }
                else
                {
                    READ_INT_FROM_MOBJ(m_join_time[idx],o,"j");
                    READ_INT_FROM_MOBJ(m_choice[idx],o,"c");
                    ++idx;

                    if (idx > 1)
                    {
                        break;
                    }
                }
            }
        }

        if (m_char[0].get() && m_char[1].get() && m_choice[0] && m_choice[1])
        {
            Done();
        }
        else if (m_char[0].get() && m_char[1].get())
        {
            if (m_choice[0] == 0)
            {
                choose_timer(1);
            }
            if (m_choice[1] == 0)
            {
                choose_timer(2);
            }
        }
        else if (m_char[0].get() && m_char[1].get() == NULL)
        {
            if (m_choice[0])
            {
                win_timer(1);
            }
            else
            {
                choose_timer(1);
            }
        }
        else if (m_char[1].get() && m_char[0].get() == NULL)
        {
            if (m_choice[1])
            {
                win_timer(2);
            }
            else
            {
                choose_timer(2);
            }
        }
    }
}

void corps_ymsj::refresh()
{
    broadCastCorpsMsg(m_corps._id, strCoprsYmsjRefresh);
}

void corps_ymsj::_Done()
{
    //结束
    next_time = time(NULL) + iCoprsYmsjRefresh;

    //定时器通知，刷新
    json_spirit::mObject mobj;
    mobj["cmd"] = "YmsjRefresh";
    mobj["id"] = m_corps._id;
    mobj["time"] = next_time;

    boost::shared_ptr<splsTimer> tmsg;
    tmsg.reset(new splsTimer(iCoprsYmsjRefresh, 1, mobj,1));
    splsTimerMgr::getInstance()->addTimer(tmsg);

    //重置数据
    for (int i = 0; i < 2; ++i)
    {
        m_char[i].reset();
        m_choice[i] = 0;
        m_join_time[i] = 0;

        if (m_choose_timer[i].is_nil() == false)
        {
            splsTimerMgr::getInstance()->delTimer(m_choose_timer[i]);
            m_choose_timer[i] = boost::uuids::nil_uuid();
        }
    }
    Save();
}

int corps_ymsj::Done()
{
    if (m_choice[0] && m_choice[1])
    {
        int win_idx = 0;
        //相同，
        if (m_choice[0] == m_choice[1])
        {
            win_idx = 0;
        }
        else
        {
            win_idx = 1;
        }

        //cout<<"1:"<<m_char[0]->cid<<","<<m_choice[0];
        //cout<<" | 2:"<<m_char[1]->cid<<","<<m_choice[1]<<endl;
        
        //发奖励
        if (m_char[win_idx].get() && m_char[win_idx]->cdata.get()
            && m_char[1-win_idx].get() && m_char[1-win_idx]->cdata.get())
        {
            //支线任务
            m_char[win_idx]->cdata->m_trunk_tasks.updateTask(task_corps_ymsj, 1);
            m_char[1-win_idx]->cdata->m_trunk_tasks.updateTask(task_corps_ymsj, 1);

            m_char[win_idx]->ymsj_can_get = m_char[win_idx]->cdata->m_area > 0 ? (500 * (m_char[win_idx]->cdata->m_area - 1)) : 500;
            //军团实际收益
            corpsRealReward(m_char[win_idx]->ymsj_can_get);
            m_char[win_idx]->save();

            int nums = m_char[win_idx]->cdata->queryExtraData(char_data_type_daily, char_data_daily_corps_ymsj);
            m_char[win_idx]->cdata->setExtraData(char_data_type_daily, char_data_daily_corps_ymsj, nums + 1);
            //act统计
            act_to_tencent(m_char[win_idx]->cdata.get(),act_new_corps_ymsj);

            json_spirit::Object notify;
            json_spirit::Object you;            
            you.push_back( Pair("win", 1) );
            you.push_back( Pair("choice", m_choice[win_idx]) );
            you.push_back( Pair("pos", win_idx+1) );
            notify.push_back( Pair("you", you) );
            json_spirit::Object other;            
            other.push_back( Pair("choice", m_choice[1-win_idx]) );
            other.push_back( Pair("spic", m_char[1-win_idx]->cdata->m_spic) );
            other.push_back( Pair("name", m_char[1-win_idx]->cdata->m_name) );
            notify.push_back( Pair("otherside", other) );
            notify.push_back( Pair("cmd", "JtYmsjDone") );
            notify.push_back( Pair("s", 200) );

            m_char[win_idx]->cdata->sendObj(notify);
            //军团活动按钮闪动
            json_spirit::Object action;
            action.push_back( Pair("cmd", "updateAction") );
            action.push_back( Pair("type", top_level_event_corp) );
            action.push_back( Pair("active", 1) );
            action.push_back( Pair("s", 200) );
            m_char[win_idx]->cdata->sendObj(action);

            int nums2 = m_char[1-win_idx]->cdata->queryExtraData(char_data_type_daily, char_data_daily_corps_ymsj);
            m_char[1-win_idx]->cdata->setExtraData(char_data_type_daily, char_data_daily_corps_ymsj, nums2 + 1);
            //act统计
            act_to_tencent(m_char[1-win_idx]->cdata.get(),act_new_corps_ymsj);

            json_spirit::Object notify2;
            json_spirit::Object you2;        
            you2.push_back( Pair("win", 0) );
            you2.push_back( Pair("choice", m_choice[1-win_idx]) );
            you2.push_back( Pair("pos", 2-win_idx) );
            notify2.push_back( Pair("you", you2) );
            json_spirit::Object other2;            
            other2.push_back( Pair("choice", m_choice[win_idx]) );
            other2.push_back( Pair("spic", m_char[win_idx]->cdata->m_spic) );
            other2.push_back( Pair("name", m_char[win_idx]->cdata->m_name) );
            notify2.push_back( Pair("otherside", other2) );
            notify2.push_back( Pair("cmd", "JtYmsjDone") );
            notify2.push_back( Pair("s", 200) );

            m_char[1-win_idx]->cdata->sendObj(notify2);

            //通知结果
            std::string msg = strCoprsYmsjMsg;
            str_replace(msg, "$A", MakeCharNameLink(m_char[win_idx]->cdata->m_name));
            str_replace(msg, "$B", MakeCharNameLink(m_char[1-win_idx]->cdata->m_name));

            std::string get = strGongxun + "+" + LEX_CAST_STR(m_char[win_idx]->ymsj_can_get);
            str_replace(msg, "$G", get);

            broadCastCorpsMsg(m_corps._id, msg);
            //日常任务
            dailyTaskMgr::getInstance()->updateDailyTask(*(m_char[win_idx]->cdata),daily_task_corp_ymsj);
            dailyTaskMgr::getInstance()->updateDailyTask(*(m_char[1-win_idx]->cdata),daily_task_corp_ymsj);
        }

        _Done();

        return 1;
    }
    else
    {
        return 0;
    }
}

int corps_ymsj::timeout()
{
    int win_idx = 0;
    if (m_char[0].get() && m_char[0]->cdata.get() && m_char[1].get() == NULL
        && (m_join_time[0] + iCorpsYmsjTimeout) <= time(NULL))
    {
        win_idx = 0;
    }
    else if (m_char[1].get() && m_char[1]->cdata.get() && m_char[0].get() == NULL
        && (m_join_time[1] + iCorpsYmsjTimeout) <= time(NULL))
    {
        win_idx = 1;
    }
    else
    {
        ERR();
        return HC_ERROR;
    }

    m_char[win_idx]->ymsj_can_get = m_char[win_idx]->cdata->m_area > 1 ? (500 * (m_char[win_idx]->cdata->m_area - 1)) : 500;
    //军团实际收益
    corpsRealReward(m_char[win_idx]->ymsj_can_get);
    m_char[win_idx]->save();

    int nums = m_char[win_idx]->cdata->queryExtraData(char_data_type_daily, char_data_daily_corps_ymsj);
    m_char[win_idx]->cdata->setExtraData(char_data_type_daily, char_data_daily_corps_ymsj, nums + 1);
    //act统计
    act_to_tencent(m_char[win_idx]->cdata.get(),act_new_corps_ymsj);

    json_spirit::Object notify;
    notify.push_back( Pair("cmd", "JtYmsjDone") );
    notify.push_back( Pair("s", 200) );
    json_spirit::Object you;            
    you.push_back( Pair("win", 1) );
    you.push_back( Pair("choice", m_choice[win_idx]) );
    you.push_back( Pair("pos", win_idx+1) );
    notify.push_back( Pair("you", you) );

    m_char[win_idx]->cdata->sendObj(notify);
    
    //军团活动按钮闪动
    json_spirit::Object action;
    action.push_back( Pair("cmd", "updateAction") );
    action.push_back( Pair("type", top_level_event_corp) );
    action.push_back( Pair("active", 1) );
    action.push_back( Pair("s", 200) );
    m_char[win_idx]->cdata->sendObj(action);

    std::string msg = strCoprsYmsjMsg2;
    str_replace(msg, "$A", MakeCharNameLink(m_char[win_idx]->cdata->m_name));

    std::string get = strGongxun + "+" + LEX_CAST_STR(m_char[win_idx]->ymsj_can_get);
    str_replace(msg, "$G", get);
    
    broadCastCorpsMsg(m_corps._id, msg);
    
    //日常任务
    dailyTaskMgr::getInstance()->updateDailyTask(*(m_char[win_idx]->cdata),daily_task_corp_ymsj);

    //支线任务
    m_char[win_idx]->cdata->m_trunk_tasks.updateTask(task_corps_ymsj, 1);

    _Done();

    return HC_SUCCESS;
}

void corps_ymsj::Save()
{
    json_spirit::Object obj;
    //obj.push_back( Pair("id", m_id) );
    obj.push_back( Pair("next", next_time) );

    json_spirit::Array list;

    for (int i = 0; i < 2; ++i)
    {
        json_spirit::Object o;
        if (m_char[i].get())
        {        
            o.push_back( Pair("id", m_char[i]->cid) );
            o.push_back( Pair("j", m_join_time[i]) );
            o.push_back( Pair("c", m_choice[i]) );
        }
        list.push_back(o);
    }
    obj.push_back( Pair("list", list) );
    InsertSaveDb("update char_corps set ymsj_data='" + json_spirit::write(obj) + "' where id=" + LEX_CAST_STR(m_corps._id));
}

splsCorps::splsCorps(int id, int camp, int level, int exp, int expWeek, int memberLimit, int assistantLimit, const std::string& name, const std::string& flag,
            const std::string& bulletin, const std::string& introduction, int createrId, const std::string& qqGroup, const std::string& ymsj_data)
:_id(id)
,m_ymsj(*this)
{
    //_id = id;
    _camp = camp;
    _level = level;
    _exp = exp;
    _expWeek = expWeek;
    int temp1, temp2;
    corpsMgr::getInstance()->getBaseCorps(level, _maxExp, temp1, temp2);
    _memberLimit = memberLimit;
    _assistantLimit = assistantLimit;
    _name = name;
    _flag = flag;
    _strBulletin = bulletin;
    _strIntroduction = introduction;
    _creater = createrId;
    _qqGroup = qqGroup;
    m_ymsj_data = ymsj_data;
    for (int i = 0; i < iCorpsYanhuiMax; ++i)
    {
        _corps_yanhui[i]._id = 0;
    }
    _createTime = 0;
    m_auto_recruit_msg = 0;
    m_auto_recruit_msg_time = 0;
}

int splsCorps::load()
{
    //cout<<"splsCorps::load()"<<_id<<endl;
    {
        Query q(GetDb());
        q.get_result("select cid,offical,contribution,contributionToday,joinTime,ymsj_gongxun from char_corps_members where corps=" + LEX_CAST_STR(_id));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            boost::shared_ptr<corps_member> cm(new corps_member);
            cm->cid = q.getval();
            //cout<<"::load:"<<cm->cid<<endl;
            cm->offical = q.getval();
            cm->contribution = q.getval();
            cm->contribution_day = q.getval();
            cm->join_time = q.getval();
            cm->ymsj_can_get = q.getval();
            cm->recruit_time = 0;
            cm->corps = _id;
            cm->cdata = GeneralDataMgr::getInstance()->GetCharData(cm->cid);

            if (1 == cm->offical)
            {
                _leader = cm;
            }
            else if (2 == cm->offical)
            {
                _assistant.push_back(cm);
            }
            if (cm->cdata.get())
            {
                //cout<<"add..."<<endl;
                cm->cdata->m_corps_member.reset();
                cm->cdata->m_corps_member = cm;
            }
            else
            {
                continue;
            }
            _members[cm->cid] = cm;
            _members_list.push_back(cm);
        }
        q.free_result();
    }

    {
        Query q(GetDb());
        //加载申请
        q.get_result("select cid,message,inputTime from char_corps_applications where corps=" + LEX_CAST_STR(_id));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            boost::shared_ptr<corps_application> app(new corps_application);
            app->cid = q.getval();
            app->message = q.getstr();
            app->app_time = q.getval();
            app->corps = _id;
            app->cdata = GeneralDataMgr::getInstance()->GetCharData(app->cid);
            if (app->cdata.get())
            {
                app->cdata->m_corps_applications.push_back(app);
            }
            _applications[app->cid] = app;
        }
        q.free_result();
    }

    time_t last_input = 0;
    {
        Query q(GetDb());
        //加载事件
        q.get_result("select cid,msg,input,name from char_corps_event where type='1' and corps=" + LEX_CAST_STR(_id) + " order by input desc limit 20");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            boost::shared_ptr<corpsEvent> e(new corpsEvent);
            e->cid = q.getval();
            e->msg = q.getstr();
            e->inputTime = q.getval();
            e->name = q.getstr();
            e->corps = _id;
            _event_list.push_back(e);
            last_input = e->inputTime;
        }
        q.free_result();
    }

    {
        Query q(GetDb());
        q.execute("delete from char_corps_event where type=1 and corps=" + LEX_CAST_STR(_id) + " and input<" + LEX_CAST_STR(last_input));
        CHECK_DB_ERR(q);
    }

    last_input = 0;
    {
        Query q(GetDb());
        q.get_result("select cid,msg,input,name from char_corps_event where type='2' and corps=" + LEX_CAST_STR(_id) + " order by input desc limit 10");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            boost::shared_ptr<corpsEvent> e(new corpsEvent);
            e->cid = q.getval();
            e->msg = q.getstr();
            e->inputTime = q.getval();
            e->name = q.getstr();
            e->corps = _id;
            _js_event_list.push_back(e);
            last_input = e->inputTime;
        }
        q.free_result();
    }

    {
        Query q(GetDb());
        q.execute("delete from char_corps_event where type=2 and corps=" + LEX_CAST_STR(_id) + " and input<" + LEX_CAST_STR(last_input));
        CHECK_DB_ERR(q);
    }

    sort();
    for (int i = 0; i < iCorpsYanhuiMax; ++i)
    {
        _corps_yanhui[i]._id = 0;
        _corps_yanhui[i]._name= "";
        _corps_yanhui[i]._type = 0;
        _corps_yanhui[i]._spic = 0;
    }
    m_ymsj.load(m_ymsj_data);
    return 0;
}

boost::shared_ptr<corps_member> splsCorps::getMember(int cid)
{
    std::map<int, boost::shared_ptr<corps_member> >::iterator it = _members.find(cid);
    if (it != _members.end())
    {
        return it->second;
    }
    boost::shared_ptr<corps_member> cm;
    return cm;
}

corps_member* splsCorps::addMember(int cid)
{
    std::map<int, boost::shared_ptr<corps_member> >::iterator it = _members.find(cid);
    if (it != _members.end())
    {
        return it->second.get();
    }
    boost::shared_ptr<corps_member> cm(new corps_member);
    cm->cid = cid;
    cm->cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    cm->contribution = 0;
    cm->contribution_day = 0;
    cm->join_time = time(NULL);
    cm->offical = 0;
    cm->ymsj_can_get = 0;
    cm->corps = _id;

    InsertSaveDb("replace into char_corps_members (corps,cid,offical,contribution,contributionToday,joinTime,ymsj_gongxun) values ("
            + LEX_CAST_STR(cm->corps) + "," + LEX_CAST_STR(cid) + ","
            + LEX_CAST_STR(cm->offical) + ","
            + LEX_CAST_STR(cm->contribution) + ","
            + LEX_CAST_STR(cm->contribution_day) + ","
            + LEX_CAST_STR(cm->join_time) + ","
            + LEX_CAST_STR(cm->ymsj_can_get) + ")");
    
    if (cm->cdata.get())
    {
        cm->cdata->m_corps_member.reset();
        cm->cdata->m_corps_member = cm;
        _members[cid] = cm;
        _members_list.push_back(cm);
        //通知军团id
        cm->cdata->NotifyCharData();
        //加入军团频道
        if (cm->cdata->GetGuildId() > 0)
        {
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cm->cdata->m_name);
            if (account.get())
            {
                boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(cm->cdata->GetGuildId());
                if (gch.get())
                {
                    gch->Add(account);
                }
            }
        }
        addEvent(cid, cm->cdata->m_name, "", corps_event_join);
        //支线任务
        cm->cdata->m_trunk_tasks.updateTask(task_join_corps);
        return cm.get();
    }
    else
    {
        ERR();
        return NULL;
    }    
}

int splsCorps::setOffical(int cid, int offical)
{
    boost::shared_ptr<corps_member> spcm = getMember(cid);
    corps_member* cm = spcm.get();
    if (NULL == cm)
    {
        ERR();
        return HC_ERROR;
    }
    if (cm->offical == offical)
    {
        return HC_SUCCESS;
    }
    if (cm->offical == 0)
    {
        if (offical == 2)
        {
            if ((int)_assistant.size() >= _assistantLimit)
            {
                return HC_ERROR_CORPS_MAX_ASSISTANT;
            }
            cm->offical = 2;
            _assistant.push_back(spcm);
            cm->save();
            sort();
            return HC_SUCCESS;
        }
        else if (_members_list.size() == 1 && cm->offical == 0)
        {
            cm->offical = 1;            
            if (_leader.get())
            {
                _leader->offical = 0;
                _leader->save();
            }
            _leader.reset();
            _leader = spcm;
            cm->save();
            sort();
            return HC_SUCCESS;
        }
        else
        {
            ERR();
            return HC_ERROR;
        }
    }
    else if (cm->offical == 2)
    {
        //转让军团长
        if (offical == 1)
        {
            cm->offical = 1;
            std::list<boost::shared_ptr<corps_member> >::iterator it = _assistant.begin();
            while (it != _assistant.end())
            {
                if ((*it)->cid == cm->cid)
                {
                    _assistant.erase(it);
                    break;
                }
                ++it;
            }
            if (_leader.get())
            {
                //军团事件
                corpsMgr::getInstance()->addEvent(_leader->cdata.get(), corps_event_new_leader, cm->cid, 0);
                _leader->offical = 2;
                _assistant.push_back(_leader);
                _leader->save();
            }
            _leader.reset();
            _leader = spcm;
            cm->save();
            sort();
            return HC_SUCCESS;
        }
        //降职
        else if (offical == 0)
        {
            cm->offical = 0;
            cm->save();
            std::list<boost::shared_ptr<corps_member> >::iterator it = _assistant.begin();
            while (it != _assistant.end())
            {
                if ((*it)->cid == cm->cid)
                {
                    _assistant.erase(it);
                    break;
                }
                ++it;
            }
            sort();
        }
        return HC_SUCCESS;
    }
    ERR();
    return HC_ERROR;
}

int splsCorps::setNewLeader(int cid)
{
    boost::shared_ptr<corps_member> spcm = getMember(cid);
    corps_member* cm = spcm.get();
    if (NULL == cm)
    {
        ERR();
        return HC_ERROR;
    }
    if (cm->offical == 1)
    {
        return HC_SUCCESS;
    }
    //转让军团长
    cm->offical = 1;
    std::list<boost::shared_ptr<corps_member> >::iterator it = _assistant.begin();
    while (it != _assistant.end())
    {
        if ((*it)->cid == cm->cid)
        {
            _assistant.erase(it);
            break;
        }
        ++it;
    }
    if (_leader.get())
    {
        //军团事件
        corpsMgr::getInstance()->addEvent(_leader->cdata.get(), corps_event_new_leader, cm->cid, 0);
        _leader->offical = 0;
        _leader->save();
    }
    _leader.reset();
    _leader = spcm;
    cm->save();
    sort();
    return HC_SUCCESS;
}

int splsCorps::removeMember(int cid, int type, const std::string& who)
{
    boost::shared_ptr<corps_member> spCm = getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm)
    {
        return HC_SUCCESS;
    }
    //军团长不能退出，只能解散
    if (1 == cm->offical)
    {
        return HC_ERROR;
    }

    if (cm->offical == 2)
    {
        std::list<boost::shared_ptr<corps_member> >::iterator it = _assistant.begin();
        while (it != _assistant.end())
        {
            if ((*it)->cid == cm->cid)
            {
                _assistant.erase(it);
                break;
            }
            ++it;
        }        
    }

    //离开军团频道
    if (cm->cdata->GetGuildId() > 0)
    {
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cm->cdata->m_name);
        if (account.get())
        {
            boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(cm->cdata->GetGuildId());
            if (gch.get())
            {
                gch->Remove(account);
            }
        }
    }

    switch (type)
    {
        case corps_event_fire:
            addEvent(cid, cm->cdata->m_name, who, type);
            break;
        case corps_event_leave:
            addEvent(cid, cm->cdata->m_name, "", type);
            break;
    }

    //删除成员
    _members.erase(cid);    //成员
    std::list<boost::shared_ptr<corps_member> >::iterator it = _members_list.begin();    //成员
    while (it != _members_list.end())
    {
        if ((*it).get() && (*it)->cid == cid)
        {
            _members_list.erase(it);
            break;
        }
        ++it;
    }
    if (cm->cdata.get())
    {
        cm->cdata->m_corps_member.reset();
        //通知军团id
        cm->cdata->NotifyCharData();
    }
    
    //删除军团成员
    InsertSaveDb("delete from char_corps_members where cid=" + LEX_CAST_STR(cm->cid));

    //取消军团战报名
    Singleton<corpsFightingMgr>::Instance().cancelSignUp(_id, cid);
    return HC_SUCCESS;
}

bool splsCorps::haveApplication(int cid)
{
    return (_applications.find(cid) != _applications.end());
}

int splsCorps::cancelApplication(int cid)
{
    std::map<int, boost::shared_ptr<corps_application> >::iterator it = _applications.find(cid);
    if (it == _applications.end())
    {
        return HC_SUCCESS;
    }
    boost::shared_ptr<corps_application> capp = it->second;
    corps_application* app = capp.get();
    _applications.erase(it);
    if (NULL == app || app->cdata.get() == NULL)
    {
        return HC_SUCCESS;
    }
    CharData* pc = app->cdata.get();
    std::list<boost::shared_ptr<corps_application> >::iterator it2 = pc->m_corps_applications.begin();
    while (it2 != pc->m_corps_applications.end())
    {
        if (it2->get() && it2->get()->corps == _id)
        {
            pc->m_corps_applications.erase(it2);
            break;
        }
        ++it2;
    }
    return HC_SUCCESS;
}

void splsCorps::save()
{
    InsertSaveDb("update char_corps set name='" + GetDb().safestr(_name) + 
            "',flag='" + GetDb().safestr(_flag) +
            "',level='"  + LEX_CAST_STR(_level) +
            "',member_limit='" + LEX_CAST_STR(_memberLimit) +
            "',creater='" + LEX_CAST_STR(_creater) +
            "',exp='" + LEX_CAST_STR(_exp) +
            "',expWeek='" + LEX_CAST_STR(_expWeek) +
            "',assistantLimit='" + LEX_CAST_STR(_assistantLimit) +
            "',qqGroup='" + GetDb().safestr(_qqGroup) +
            "',bulletin='" + GetDb().safestr(_strBulletin) +
            "',introduction='" + GetDb().safestr(_strIntroduction) + "' where id=" + LEX_CAST_STR(_id));
}

void splsCorps::sort()
{
    _members_list.sort(compare_corps_member);
}

int splsCorps::jiSi(CharData* pc, int type)
{    
    if (type < 1 || type > 3)
    {
        type = 1;
    }
    if (pc->m_vip < iCorpsJisi[type-1][3])
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    int fac = corpsMgr::getInstance()->getJisiFac();
    int add_prestige = iCorpsJisi[type-1][2] * fac / 100;
    if (add_prestige <= 0)
        return HC_ERROR;
    if (iCorpsJisi[type-1][0])
    {
        if (pc->addGold(-iCorpsJisi[type-1][0]) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        {
            if (type == 2)
            {
                add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, iCorpsJisi[type-1][0], gold_cost_for_corps_jisi_type2, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
                gold_cost_tencent(pc,iCorpsJisi[type-1][0],gold_cost_for_corps_jisi_type2);
#endif
            }
            else if(type == 3)
            {
                add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, iCorpsJisi[type-1][0], gold_cost_for_corps_jisi_type3, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
                gold_cost_tencent(pc,iCorpsJisi[type-1][0],gold_cost_for_corps_jisi_type3);
#endif
            }
        }
    }
    if (iCorpsJisi[type-1][1])
    {
        if (pc->addGongxun(-iCorpsJisi[type-1][1]) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GONGXUN;
        }
    }
    ++pc->m_temp_jisi_times;
    pc->saveCharDailyVar();
    //军团实际收益
    corpsRealReward(add_prestige);

    pc->addPrestige(add_prestige);
    add_statistics_of_prestige_get(pc->m_id,pc->m_ip_address,add_prestige,prestige_jisi,pc->m_union_id,pc->m_server_id);
    {
        if (type == 1)
        {
            add_statistics_of_prestige_get(pc->m_id,pc->m_ip_address,add_prestige,prestige_jisi_type1,pc->m_union_id,pc->m_server_id);
        }
        else if (type == 2)
        {
            add_statistics_of_prestige_get(pc->m_id,pc->m_ip_address,add_prestige,prestige_jisi_type2,pc->m_union_id,pc->m_server_id);
        }
        else if(type == 3)
        {
            add_statistics_of_prestige_get(pc->m_id,pc->m_ip_address,add_prestige,prestige_jisi_type3,pc->m_union_id,pc->m_server_id);
        }
    }
    pc->NotifyCharData();
    //增加事件
    corpsMgr::getInstance()->addEvent(pc, corps_event_add_exp, add_prestige, 0);

    std::string msg = "";

    switch (type)
    {
        default:
        case 1:
            msg = strCorpsJisiEventType1;
            break;
        case 2:
            msg = strCorpsJisiEventType2;
            break;
        case 3:
            msg = strCorpsJisiEventType3;
            break;
    }
    str_replace_all(msg, "$D", LEX_CAST_STR(add_prestige));

    corpsEvent* ce = new corpsEvent;
    ce->cid = pc->m_id;
    ce->corps = _id;
    ce->inputTime = time(NULL);
    ce->name = pc->m_name;
    ce->msg = msg;
    InsertSaveDb("insert into char_corps_event (corps,cid,name,msg,input,type) values (" + LEX_CAST_STR(ce->corps)
                        + "," + LEX_CAST_STR(ce->cid) + ",'" + GetDb().safestr(ce->name)
                        + "','" + GetDb().safestr(ce->msg) + "',unix_timestamp(),2)");
    boost::shared_ptr<corpsEvent> sp_ce(ce);
    _js_event_list.insert(_js_event_list.begin(), sp_ce);
    while (_js_event_list.size() >= 10)
    {
        boost::shared_ptr<corpsEvent> e = _js_event_list.back();
        if (e.get())
        {
            InsertSaveDb("delete from char_corps_event where type='2' and corps=" + LEX_CAST_STR(e->corps)
                        + " and input<" + LEX_CAST_STR(e->inputTime));
        }
        _js_event_list.pop_back();
    }
    //日常任务
    dailyTaskMgr::getInstance()->updateDailyTask(*pc, daily_task_corp_jisi);

    //支线任务
    pc->m_trunk_tasks.updateTask(task_corps_jisi, 1);
    return HC_SUCCESS;
}

int splsCorps::JoinYanhui(CharData * pc, int pos)
{
    if (pos < 1 || pos > iCorpsYanhuiMax)
    {
        return HC_ERROR;
    }
    if (_corps_yanhui[pos-1]._id != 0)
    {
        int new_pos = 0;
        for (int i = 0; i < iCorpsYanhuiMax; ++i)
        {
            if (0 == _corps_yanhui[i]._id)
            {
                new_pos = i + 1;
                break;
            }
        }
        if (!new_pos)
        {
            return HC_ERROR;
        }
        else
        {
            pos = new_pos;
        }
    }

    _corps_yanhui[pos-1]._id = pc->m_id;
    _corps_yanhui[pos-1]._name = pc->m_name;
    _corps_yanhui[pos-1]._spic = pc->m_spic;
    _corps_yanhui[pos-1]._type = pc->m_temp_corps_yanhui > 0 ? 2 : 1;

    checkYanhuiSuccess();
    return HC_SUCCESS;
}

int splsCorps::inviteSomeone(CharData* pc, int pos)    //邀请路人
{
    if (pos < 1 || pos > iCorpsYanhuiMax)
    {
        return HC_ERROR;
    }
    if (_corps_yanhui[pos-1]._id != 0)
    {
        int new_pos = 0;
        for (int i = 0; i < iCorpsYanhuiMax; ++i)
        {
            if (0 == _corps_yanhui[i]._id)
            {
                new_pos = i + 1;
                break;
            }
        }
        if (!new_pos)
        {
            return HC_ERROR;
        }
        else
        {
            pos = new_pos;
        }
    }
    if (pc->addGold(-5) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    //金币消耗统计
    add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, 5, gold_cost_for_corps_invite, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(pc,5,gold_cost_for_corps_invite);
#endif

    _corps_yanhui[pos-1]._id = -1;
    _corps_yanhui[pos-1]._name = "Lu ren";
    _corps_yanhui[pos-1]._spic = pc->m_spic;
    _corps_yanhui[pos-1]._type = 2;

    pc->NotifyCharData();

    checkYanhuiSuccess();
    return HC_SUCCESS;
}

int splsCorps::checkYanhuiSuccess()
{
    bool bOpenSuccess = true;
    for (int i = 0; i < iCorpsYanhuiMax; ++i)
    {
        if (0 == _corps_yanhui[i]._id)
        {
            bOpenSuccess = false;
            break;
        }
    }
    if (bOpenSuccess)
    {
        //军团广播下成功开宴会
        for (int i = 0; i < iCorpsYanhuiMax; ++i)
        {
            if (-1 == _corps_yanhui[i]._id)
            {
                _corps_yanhui[i]._id = 0;
                continue;
            }
            //不是协助的给予奖励
            if (1 == _corps_yanhui[i]._type)
            {
                //INFO("************NotifyCharData()"<<m_name);
                std::string notify_msg = strCorpsYanhuiNotify1;
                str_replace(notify_msg, "$N", LEX_CAST_STR(iCorpsYanhuiLing));
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(_corps_yanhui[i]._name);
                if (account.get())
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "partyEnd"));
                    obj.push_back( Pair("s",200) );
                    obj.push_back( Pair("msg", notify_msg) );
                    account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
                }
                boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(_corps_yanhui[i]._id);
                if (cdata.get())
                {
                    ++cdata->m_temp_corps_yanhui;
                    cdata->saveCharDailyVar();
                    //奖励暂存
                    std::list<Item> tmp_list;
                    {
                        Item item_p;
                        item_p.type = item_type_ling;
                        item_p.nums = iCorpsYanhuiLing;
                        //军团实际收益
                        corpsRealReward(item_p.nums);
                        tmp_list.push_back(item_p);
                    }
                    Singleton<char_rewards_mgr>::Instance().updateCharRewards(cdata->m_id,rewards_type_yanhui,0,tmp_list);
                    //cdata->addLing(iCorpsYanhuiLing);
                    //军令统计
                    //add_statistics_of_ling_cost(cdata->m_id,cdata->m_ip_address,iCorpsYanhuiLing,ling_corps,1, cdata->m_union_id, cdata->m_server_id);
                    cdata->NotifyCharData();
                }
                //获得军令的发送系统邮件
                sendSystemMail(_corps_yanhui[i]._name, _corps_yanhui[i]._id, strCorpsYanhuiMailTitle, notify_msg);
                //日常任务
                dailyTaskMgr::getInstance()->updateDailyTask(*cdata,daily_task_corp_yanhui);
            }
            else
            {
                //INFO("************NotifyCharData()"<<m_name);
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(_corps_yanhui[i]._name);
                if (account.get())
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "partyEnd"));
                    obj.push_back( Pair("s",200) );
                    obj.push_back( Pair("msg", strCorpsYanhuiNotify2) );
                    account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
                }
            }
            _corps_yanhui[i]._id = 0;
        }
    }
    return HC_SUCCESS;
}

int splsCorps::queryYanhuiPerson()                //查询宴会人数
{
    int counts = 0;
    for (int i = 0; i < iCorpsYanhuiMax; ++i)
    {
        if (_corps_yanhui[i]._id)
        {
            ++counts;
        }
    }
    return counts;
}

void splsCorps::dailyReset()
{
    std::map<int, boost::shared_ptr<corps_member> >::iterator it = _members.begin();
    while (it != _members.end())
    {
        corps_member* cm = it->second.get();
        if (cm)
        {
            cm->contribution_day = 0;
        }
        ++it;
    }
    InsertSaveDb("update char_corps_members set contributionToday=0 where corps="    + LEX_CAST_STR(_id));
}

void splsCorps::addEvent(int cid, const std::string& name, const std::string& name2, int type)
{
    corpsEvent* ce = new corpsEvent;
    ce->cid = cid;
    ce->corps = _id;
    ce->inputTime = time(NULL);
    ce->name = name;

    switch (type)
    {
        case corps_event_fire:
            ce->msg = strCorpsMsgFire;
            str_replace(ce->msg, "$W", name2);
            break;

        case corps_event_leave:
            ce->msg = strCorpsMsgLeave;
            break;
        case corps_event_join:
        default:
            ce->msg = strCorpsMsgJoin;
            break;
    }
    InsertSaveDb("insert into char_corps_event (corps,cid,name,msg,input,type) values (" + LEX_CAST_STR(ce->corps)
                        + "," + LEX_CAST_STR(ce->cid) + ",'" + GetDb().safestr(ce->name)
                        + "','" + GetDb().safestr(ce->msg) + "',unix_timestamp(),1)");
    boost::shared_ptr<corpsEvent> sp_ce(ce);
    _event_list.insert(_event_list.begin(), sp_ce);
    while (_event_list.size() >= 10)
    {
        boost::shared_ptr<corpsEvent> e = _event_list.back();
        if (e.get())
        {
            InsertSaveDb("delete from char_corps_event where type='1' and corps=" + LEX_CAST_STR(ce->corps)
                        + " and input<" + LEX_CAST_STR(ce->inputTime));
        }
        _event_list.pop_back();
    }
}

corpsMgr* corpsMgr::m_handle = NULL;

corpsMgr* corpsMgr::getInstance()
{
    if (m_handle == NULL)
    {
        time_t time_start = time(NULL);
        cout<<"corpsMgr::getInstance()..."<<endl;
        m_handle = new corpsMgr();
        m_handle->init();
        cout<<"corpsMgr::getInstance() finish,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    }
    return m_handle;
}

//初始化
int corpsMgr::init()
{
    cout<<"int corpsMgr::init()..."<<endl;
    Query q(GetDb());

    m_open_robot_corps = GeneralDataMgr::getInstance()->getInt("robot_corps", 1) ? true : false;
    m_total_robot_corps = 0;
    max_corps = 0;

    q.get_result("select name from base_robot_corps_name where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        m_robot_corps_name.push_back(q.getstr());
    }
    q.free_result();

    q.get_result("select memo from base_robot_corps_memo where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        m_robot_corps_memo.push_back(q.getstr());
    }
    q.free_result();
    
    q.get_result("select exp,memberLimit,assistantLimit from base_corps_level where 1 order by level");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        //int level = q.getval();
        baseCorps xxx;
        xxx._maxExp = q.getval();
        xxx._memberLimit = q.getval();
        xxx._assistantLimit = q.getval();
        m_base_corps.push_back(xxx);
    }
    q.free_result();
    max_corps_action = 0;
    q.get_result("select id,need_level,name,award_memo,memo from base_corps_action where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        base_corps_action action;
        action._id = q.getval();
        action._needlevel = q.getval();
        action._name = q.getstr();
        action._award_memo = q.getstr();
        action._memo = q.getstr();
        m_base_corps_action.push_back(action);
        max_corps_action = action._id;
    }
    q.free_result();
    
    q.get_result("select id,camp,name,flag,level,member_limit,assistantLimit,creater,exp,expWeek,qqGroup,bulletin,introduction,ymsj_data,createTime from char_corps where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int camp = q.getval();
        std::string name = q.getstr();
        std::string flag = q.getstr();
        int level = q.getval();

        int memberLimit = q.getval();
        int assistantLimit = q.getval();
        int creater = q.getval();
        int exp = q.getval();
        int exp_week = q.getval();
        std::string qqGroup = q.getstr();
        std::string bulletin = q.getstr();
        std::string introduction = q.getstr();
        std::string ymsj_data = q.getstr();
        time_t createTime = q.getval();
        boost::shared_ptr<splsCorps> spCorps(new splsCorps(id, camp, level, exp, exp_week, memberLimit, assistantLimit, name, flag, bulletin, introduction, creater, qqGroup, ymsj_data));
        m_corps_maps[id] = spCorps;
        m_corps_maps2[name] = spCorps;
        spCorps->_createTime = createTime;

        //spCorps->m_ymsj.load(ymsj_data);

        m_corps_list.push_back(spCorps);

        max_corps = id;
    }
    q.free_result();

    std::map<int, boost::shared_ptr<splsCorps> >::iterator it = m_corps_maps.begin();
    while (it != m_corps_maps.end())
    {
        if (it->second.get())
        {
            it->second->load();
            if (it->second->_leader.get() == NULL)
            {
                ++m_total_robot_corps;
            }
        }
        ++it;
    }
    //更新排名
    updateRank();

    m_strZhaoMsg = strCorpsYanhuiZhaojiMsg;
    str_replace(m_strZhaoMsg, "$E", strZhaojiUrl);
    //祭祀活动
    m_jisi_event_fac = GeneralDataMgr::getInstance()->getInt("jisi_fac", 100);
    m_jisi_event_endtime = GeneralDataMgr::getInstance()->getInt("jisi_event_end", 0);
    if (m_jisi_event_endtime <= time(NULL))
    {
        m_jisi_event_fac = 100;
    }
    return HC_SUCCESS;
}

void corpsMgr::getAction(CharData* pc, json_spirit::Array& blist)
{
    int state = 0;
    if (pc->m_corpsOpen && pc->m_corps_member.get())
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_corp) );
        if (pc->m_corps_member->ymsj_can_get)
        {
            state = 1;
        }
        charCorpsExplore* c = Singleton<corpsExplore>::Instance().getChar(pc->m_id).get();
        int nums = pc->queryExtraData(char_data_type_daily, char_data_daily_corps_explore);
        int corps_explore_times = 0;
        splsCorps* cp = corpsMgr::getInstance()->findCorps(pc->m_corps_member.get()->corps);
        if (cp)
        {
            corps_explore_times = iCorpsExploreTimesOneday[cp->_level];
        }
        if (c && c->isDone() && corps_explore_times > nums)
        {
            state = 1;
        }
        obj.push_back( Pair("active", state) );
        blist.push_back(obj);
    }
}

//创建军团
int corpsMgr::createCorps(int cid, const std::string& name, json_spirit::Object& robj)
{
    std::map<const std::string, boost::shared_ptr<splsCorps> >::iterator it = m_corps_maps2.find(name);
    if (it != m_corps_maps2.end())
    {
        return HC_ERROR_CORPS_NAME_EXIST;
    }
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (NULL == pc)
    {
        return HC_ERROR;
    }
    if (name.length() > 18)
    {
        return HC_ERROR_NAME_TOO_LONG;
    }
    if (name.length() < 3 || !Forbid_word_replace::getInstance()->isLegal(name))
    {
        return HC_ERROR_NAME_ILLEGAL;
    }
    //等级限制，及是否选择了阵营
    if (!pc->m_corpsOpen)
    {
        return HC_ERROR_CORPS_NEED_MORE_LEV;
    }
#ifndef JP_SERVER
    #if 0
    //金币消耗
    if (pc->addGold(-50) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    //金币消耗统计
    add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, 50, gold_cost_for_corps_create, pc->m_union_id, pc->m_server_id);
    #else
    if (pc->m_vip < iCorpsCreateVip)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    if (pc->addSilver(-10000) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_SILVER;
    }
    //消耗统计
    add_statistics_of_silver_cost(pc->m_id, pc->m_ip_address, 10000, silver_cost_for_corps_create, pc->m_union_id, pc->m_server_id);
    #endif
#endif
    boost::shared_ptr<splsCorps> spCorps(new splsCorps(++max_corps, pc->m_camp, 1, 0, 0, 20, 2, name, "", "", "", cid, "", ""));
    m_corps_maps[max_corps] = spCorps;
    m_corps_maps2[name] = spCorps;
    spCorps->_createTime = time(NULL);
    m_corps_list.push_back(spCorps);
    spCorps->_rank = m_corps_list.size();

    InsertSaveDb("insert into char_corps (id,camp,name,flag,level,member_limit,assistantLimit,creater,exp,expWeek,qqGroup,bulletin,introduction,createTime) values ("
        + LEX_CAST_STR(max_corps) + "," + LEX_CAST_STR(pc->m_camp) + ",'" + GetDb().safestr(name) + "','',1,20,2," + LEX_CAST_STR(cid) + ",0,0,'','',''," + LEX_CAST_STR(spCorps->_createTime) +")");
    robj.push_back( Pair("id", max_corps) );

    corps_member* cm = spCorps->addMember(cid);
    //设置军团长
    spCorps->setOffical(cid, 1);

    if (cm->cdata.get())
    {
        cm->cdata->m_corps_applications.clear();
#ifdef QQ_PLAT
        //加入军团分享
        Singleton<inviteMgr>::Instance().update_event(cid, SHARE_EVENT_JOIN_CORPS, 0);
#endif
    }
    //取消该角色其他的申请
    std::map<int, boost::shared_ptr<splsCorps> >::iterator itx = m_corps_maps.begin();
    while (itx != m_corps_maps.end())
    {
        if (itx->second.get())
        {
            itx->second->cancelApplication(cid);
        }
        ++itx;
    }
    pc->NotifyCharData();
    return HC_SUCCESS;
}

//查看审核信息
int corpsMgr::getApplications(int cid, int corps, int page, int nums_per_page, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    if (cm->offical != 1 && cm->offical != 2)
    {
        return HC_ERROR_CORPS_OFFICAL_LIMIT;
    }
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page == 0)
    {
        nums_per_page = 10;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;

    json_spirit::Array app_list;
    std::map<int, boost::shared_ptr<corps_application> >::iterator it_app = cp->_applications.begin();
    while (it_app != cp->_applications.end())
    {
        if (it_app->second.get() && it_app->second->cdata.get())
        {
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                corps_application* app = it_app->second.get();
                CharData* pc = it_app->second->cdata.get();
                json_spirit::Object obj;
                obj.push_back( Pair("id", pc->m_id) );
                obj.push_back( Pair("name", pc->m_name) );
                obj.push_back( Pair("level", pc->m_level) );
                obj.push_back( Pair("time", time(NULL)-app->app_time) );
                app_list.push_back(obj);
            }
        }
        ++it_app;
    }
    robj.push_back( Pair("list", app_list) );

    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );

    return HC_SUCCESS;
}

//通过审核
int corpsMgr::acceptApplication(int cid, int corps, int tcid, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    //是否有权限接受成员
    if (cm->offical != 1 && cm->offical != 2)
    {
        return HC_ERROR_CORPS_OFFICAL_LIMIT;
    }
    //人员数量是否达到上限了
    if ((int)cp->_members_list.size() >= cp->_memberLimit)
    {
        return HC_ERROR_CORPS_MAX_MEMBERS;
    }
    //是否有该申请
    std::map<int, boost::shared_ptr<corps_application> >::iterator it_app = cp->_applications.find(tcid);
    if (it_app == cp->_applications.end())
    {
        return HC_ERROR;
    }
    cp->_applications.erase(it_app);
    cp->addMember(tcid);
    cp->sort();
    //被审核通过的玩家刷新开放信息
    boost::shared_ptr<corps_member> spCm_t = cp->getMember(tcid);
    corps_member* cm_t = spCm_t.get();
    if (cm_t)
    {
        CharData* cdata_t = cm_t->cdata.get();
        if (cdata_t)
        {
            cdata_t->NotifyCharOpenInfo();
            //好友动态
            Singleton<relationMgr>::Instance().postFriendInfos(tcid, cdata_t->m_name, FRIEND_NEWS_JOIN_CORPS, 0, cp->_name);
#ifdef QQ_PLAT
            //加入军团分享
            Singleton<inviteMgr>::Instance().update_event(tcid, SHARE_EVENT_JOIN_CORPS, 0);
#endif
        }
    }

    //取消该角色其他的申请
    std::map<int, boost::shared_ptr<splsCorps> >::iterator it = m_corps_maps.begin();
    while (it != m_corps_maps.end())
    {
        if (it->second.get())
        {
            it->second->cancelApplication(tcid);
        }
        ++it;
    }
    if (cm->cdata.get())
    {
        cm->cdata->m_corps_applications.clear();
    }

    //超过两个军团满员，并且没机器人军团，创建机器人军团
    if (m_open_robot_corps && m_total_robot_corps == 0 && (int)cp->_members_list.size() >= cp->_memberLimit)
    {
        std::list<boost::shared_ptr<splsCorps> >::iterator it = m_corps_list.begin();
        while (it != m_corps_list.end())
        {
            if (it->get() && it->get() != cp && it->get()->_members_list.size() >= it->get()->_memberLimit)
            {
                //超过两个军团满员了
                createRobotCorps();
                break;
            }
            else
            {
                ++it;
            }
        }
    }
    return HC_SUCCESS;
}

//拒绝审核
int corpsMgr::rejectApplication(int cid, int corps, int tcid, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm)
    {
        ERR();
        return HC_ERROR;
    }
    if (cm->offical != 1 && cm->offical != 2)
    {
        ERR();
        return HC_ERROR;
    }
    //是否有该申请
    std::map<int, boost::shared_ptr<corps_application> >::iterator it_app = cp->_applications.find(tcid);
    if (it_app == cp->_applications.end() || !it_app->second.get())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<corps_application> capp = it_app->second;
    CharData* cdata = capp->cdata.get();
    cp->_applications.erase(it_app);

    if (cdata)
    {
        std::list<boost::shared_ptr<corps_application> >::iterator it = cdata->m_corps_applications.begin();
        while (it != cdata->m_corps_applications.end())
        {
            if (it->get() && (*it)->corps == corps)
            {
                cdata->m_corps_applications.erase(it);
                break;
            }
            ++it;
        }
    }
    return HC_SUCCESS;
}

//拒绝所有审核
int corpsMgr::rejectAllApplication(int cid, int corps, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    if (cm->offical != 1 && cm->offical != 2)
    {
        return HC_ERROR_CORPS_OFFICAL_LIMIT;
    }
    //是否有该申请
    std::map<int, boost::shared_ptr<corps_application> >::iterator it_app = cp->_applications.begin();
    while (it_app != cp->_applications.end())
    {
        if (it_app->second.get() && it_app->second->cdata.get())
        {
            CharData* cdata = it_app->second->cdata.get();
            if (cdata)
            {
                std::list<boost::shared_ptr<corps_application> >::iterator it_c = cdata->m_corps_applications.begin();
                while (it_c != cdata->m_corps_applications.end())
                {
                    if (it_c->get() && (*it_c)->corps == corps)
                    {
                        cdata->m_corps_applications.erase(it_c);
                        break;
                    }
                    ++it_c;
                }
            }
        }
        ++it_app;
    }
    cp->_applications.clear();
    return HC_SUCCESS;
}

//查询军团名字
std::string corpsMgr::getCorpsName(int corps)
{
    std::string name = "";
    //找军团
    std::map<int, boost::shared_ptr<splsCorps> >::iterator it = m_corps_maps.find(corps);
    if (it != m_corps_maps.end() && it->second.get())
    {
        name = it->second->_name;
    }
    return name;
}

//获取军团等级
int corpsMgr::getCorpsLevel(int corps)
{
    int level = 0;
    //找军团
    std::map<int, boost::shared_ptr<splsCorps> >::iterator it = m_corps_maps.find(corps);
    if (it != m_corps_maps.end() && it->second.get())
    {
        level = it->second->_level;
    }
    return level;
}

//查询军团信息(申请页面查看)
int corpsMgr::getCorpsInfo(int cid, int corps, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    json_spirit::Object obj;
    obj.push_back( Pair("name", cp->_name) );
    obj.push_back( Pair("level", cp->_level) );
    obj.push_back( Pair("rank", cp->_rank) );
    obj.push_back( Pair("memberNums", cp->_members.size()) );
    obj.push_back( Pair("memberLimit", cp->_memberLimit) );
    obj.push_back( Pair("intro", cp->_strIntroduction) );

    if (cp->_leader.get() && cp->_leader->cdata.get())
    {
        obj.push_back( Pair("leader", cp->_leader->cdata->m_name) );
    }

    //json_spirit::Array member_list;
    
    return HC_SUCCESS;
}

//查询本军团信息
int corpsMgr::getCorpsDetail(int cid, int corps, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    json_spirit::Object obj;
    obj.push_back( Pair("name", cp->_name) );
    obj.push_back( Pair("level", cp->_level) );
    obj.push_back( Pair("nums", cp->_members.size()) );
    obj.push_back( Pair("qq", cp->_qqGroup) );
    obj.push_back( Pair("rank", cp->_rank) );

    obj.push_back( Pair("exp", cp->_exp) );
    obj.push_back( Pair("expLimit", cp->_maxExp) );
    obj.push_back( Pair("expWeek", cp->_expWeek) );

    obj.push_back( Pair("numsLimit", cp->_memberLimit) );
    obj.push_back( Pair("memo", cp->_strBulletin) );
    obj.push_back( Pair("intro", cp->_strIntroduction) );

    if (cp->_leader.get() && cp->_leader->cdata.get())
    {
        obj.push_back( Pair("leader", cp->_leader->cdata->m_name) );
        if (cp->_leader->cdata->m_id != cid)
        {
            obj.push_back( Pair("change_leader", (time(NULL) - cp->_leader->cdata->m_login_time)>72*3600) );
        }
    }
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (cm != NULL)
    {
        obj.push_back( Pair("recruit_member", (cm->recruit_time - time(NULL))) );
    }
    robj.push_back( Pair("info", obj) );
    return HC_SUCCESS;
}

//查询军团成员
int corpsMgr::getCorpsMembers(int cid, int corps, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    bool bDetail = true;
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm)
    {
        bDetail = false;
    }
    json_spirit::Array alist;
    std::list<boost::shared_ptr<corps_member> >::iterator it_l = cp->_members_list.begin();    //副军团长
    while (it_l != cp->_members_list.end())
    {
        if (it_l->get() && it_l->get()->cdata.get())
        {
            corps_member* cm = it_l->get();
            CharData* cdata = cm->cdata.get();
            json_spirit::Object obj;
            obj.push_back( Pair("id", cm->cid) );
            obj.push_back( Pair("name", cdata->m_name) );
            obj.push_back( Pair("level", cdata->m_level) );
            obj.push_back( Pair("post", cm->offical) );
            if (bDetail)
            {
                obj.push_back( Pair("devoteTotal", cm->contribution) );
                obj.push_back( Pair("devoteToday", cm->contribution_day) );
                obj.push_back( Pair("login", time(NULL) - cdata->m_login_time) );
            }
            boost::shared_ptr<CharRaceData> rd = RaceMgr::getInstance()->getRaceData(cdata->m_id);
            if (rd.get() && rd->getChar())
            {
                obj.push_back( Pair("rank", rd->m_rank) );
            }
            obj.push_back( Pair("gender", cdata->m_gender) );
            if (cdata->getChangeSpic())
            {
                obj.push_back( Pair("change", cdata->getChangeSpic()) );
            }
            obj.push_back( Pair("online", cdata->m_is_online) );
            alist.push_back(obj);            
        }
        ++it_l;
    }
    robj.push_back( Pair("list", alist) );
    return HC_SUCCESS;
}

//查询军团日志
int corpsMgr::getCorpsEvents(int cid, int corps, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    json_spirit::Array elist;
    std::list<boost::shared_ptr<corpsEvent> >::iterator it_e = cp->_event_list.begin();
    while (it_e != cp->_event_list.end())
    {
        if (it_e->get())
        {
            corpsEvent* pe = it_e->get();
            json_spirit::Object obj;
            obj.push_back( Pair("id", pe->cid) );
            obj.push_back( Pair("name", pe->name) );
            obj.push_back( Pair("memo", pe->msg) );
            obj.push_back( Pair("time", time(NULL)-pe->inputTime) );
            elist.push_back(obj);
        }
        ++it_e;
    }
    robj.push_back( Pair("list", elist) );
    return HC_SUCCESS;
}

//退出军团
int corpsMgr::quitCorps(int cid, int corps, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }

    return cp->removeMember(cid, corps_event_leave, "");
}

//提交申请
int corpsMgr::submitApplication(int cid, int corps, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    if (cp->_memberLimit <= (int)cp->_members_list.size())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    CharData* pc = cdata.get();
    if (NULL == pc)
    {
        return HC_ERROR;
    }
    //已经有军团了不能申请
    if (pc->m_corps_member.get())
    {
        return HC_ERROR_ALREADY_IN_A_CORPS;
    }
    //已经申请三个军团了
    if (pc->m_corps_applications.size() >= 3)
    {
        return HC_ERROR_CORPS_MAX_APPLY;
    }
    //等级限制
    if (!pc->m_corpsOpen)
    {
        return HC_ERROR_CORPS_NEED_MORE_LEV;
    }
    //同一阵营才能加入
    //if (pc->m_camp != cp->_camp)
    //{
    //    return HC_ERROR_CORPS_NOT_SAME_CAMP;
    //}
    //是否提交过申请了
    if (cp->haveApplication(cid))
    {
        return HC_ERROR_CORPS_ALREADY_APPLY;
    }

    //机器人军团，自动加入
    if (NULL == cp->_leader.get() || NULL == cp->_leader->cdata.get())
    {
        corps_member* cm_t = cp->addMember(cid);
        cp->sort();
        //被审核通过的玩家刷新开放信息
        if (cm_t)
        {
            CharData* cdata_t = cm_t->cdata.get();
            if (cdata_t)
            {
                cdata_t->NotifyCharOpenInfo();
                //好友动态
                Singleton<relationMgr>::Instance().postFriendInfos(cid, cdata_t->m_name, FRIEND_NEWS_JOIN_CORPS, 0, cp->_name);
#ifdef QQ_PLAT
                //加入军团分享
                Singleton<inviteMgr>::Instance().update_event(cid, SHARE_EVENT_JOIN_CORPS, 0);
#endif
            }
        }
        //取消该角色其他的申请
        std::map<int, boost::shared_ptr<splsCorps> >::iterator it = m_corps_maps.begin();
        while (it != m_corps_maps.end())
        {
            if (it->second.get())
            {
                it->second->cancelApplication(cid);
            }
            ++it;
        }
        pc->m_corps_applications.clear();

        //军团是否满了，是否需要再建一个机器人军团
        if (m_open_robot_corps
            && cp->_memberLimit <= (int)cp->_members_list.size()
            && m_total_robot_corps < 50)
        {
            bool bFull = true;
            std::list<boost::shared_ptr<splsCorps> >::iterator it = m_corps_list.begin();    
            while (it != m_corps_list.end())
            {
                if (it->get())
                {
                    splsCorps* ccp = it->get();
                    //是机器人军团
                    if (ccp->_leader.get() == NULL && ccp->_memberLimit <= (int)ccp->_members_list.size())
                    {
                        ;
                    }
                    else
                    {
                        bFull = false;
                        break;
                    }
                }
                ++it;
            }
            if (bFull)
            {
                createRobotCorps();
            }
        }
        return HC_SUCCESS;
    }
    else
    {
        corps_application* capp = new corps_application;
        capp->cid = cid;
        capp->cdata = cdata;
        capp->corps = corps;
        capp->app_time = time(NULL);
        boost::shared_ptr<corps_application> spApp(capp);
        pc->m_corps_applications.push_back(spApp);
        cp->_applications[cid] = spApp;
        json_spirit::Object msg_obj;
        msg_obj.push_back( Pair("cmd", "corp_sub") );
        msg_obj.push_back( Pair("s", 200) );
        cp->_leader->cdata->sendObj(msg_obj);

        std::list<boost::shared_ptr<corps_member> >::iterator it = cp->_assistant.begin();
        while (it != cp->_assistant.end())
        {
            if ((*it).get() && (*it)->cdata.get())
            {
                (*it)->cdata->sendObj(msg_obj);
            }
            ++it;
        }
    }
    return HC_SUCCESS;
}

//取消申请
int corpsMgr::cancelApplication(int cid, int corps, json_spirit::Object& robj)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (NULL == pc)
    {
        return HC_ERROR;
    }
    std::list<boost::shared_ptr<corps_application> >::iterator it = pc->m_corps_applications.begin();
    while (it != pc->m_corps_applications.end())
    {
        if (it->get() && (*it)->corps == corps)
        {
            pc->m_corps_applications.erase(it);
            //找军团
            splsCorps* cp = findCorps(corps);
            if (!cp)
            {
                return HC_ERROR_NOT_JOIN_JT;
            }
            cp->cancelApplication(cid);
            return HC_SUCCESS;
        }
        ++it;
    }
    return HC_ERROR;
}

//任命
int corpsMgr::appointment(int cid, int corps, int tcid, int offical, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm)
    {
        ERR();
        return HC_ERROR;
    }
    if (cm->offical != 1)
    {
        return HC_ERROR_CORPS_OFFICAL_LIMIT;
    }
    return cp->setOffical(tcid, offical);
}

//申请改变军团长
int corpsMgr::changeLeader(int cid, int corps, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm)
    {
        ERR();
        return HC_ERROR;
    }
    //需要自身不是军团长
    if (cm->offical == 1)
    {
        return HC_ERROR;
    }

    if (cp->_leader.get() == NULL || cp->_leader->cdata.get() == NULL)
    {
        return HC_ERROR;
    }
    //需要军团长离线超过三天
    if ((time(NULL) - cp->_leader->cdata->m_login_time)<72*3600)
    {
        return HC_ERROR;
    }

    if (cp->_leader->contribution > 0)
    {
        //申请人贡献需要大于军团长贡献
        if (cm->contribution < cp->_leader->contribution
            || (cm->contribution - cp->_leader->contribution)*10/cp->_leader->contribution < 1)
        {
            return HC_ERROR_CORPS_CON;
        }
    }
    return cp->setNewLeader(cid);
}

//发布招募团员
int corpsMgr::recruitMember(int cid, int corps, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm || !cm->cdata.get())
    {
        ERR();
        return HC_ERROR;
    }
    //时间限制
    if (cm->recruit_time > time(NULL))
    {
        return HC_ERROR;
    }
    if (cm->cdata->addGold(-1) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    //金币消耗统计
    add_statistics_of_gold_cost(cm->cdata->m_id, cm->cdata->m_ip_address, 1, gold_cost_for_corps_recruit, cm->cdata->m_union_id, cm->cdata->m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(cm->cdata.get(),1,gold_cost_for_corps_recruit);
#endif
    std::string msg = strCorpsRecruitMsg;
    str_replace(msg, "$N", cp->_name);
    str_replace(msg, "$n", LEX_CAST_STR(cp->_id));
    GeneralDataMgr::getInstance()->broadCastSysMsg(msg,-1);
    cm->recruit_time = time(NULL) + 60;
    return HC_SUCCESS;
}

//开除成员
int corpsMgr::fireMember(int cid, int corps, int tcid, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm || NULL == cm->cdata.get())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<corps_member> target_spCm = cp->getMember(tcid);
    if (NULL == target_spCm.get())
    {
        return HC_ERROR;
    }
    //军团长或副军团长可以踢成员，只能踢比自己级别低的成员
    if ((cm->offical != 1 && cm->offical != 2))
    {
        return HC_ERROR_CORPS_OFFICAL_LIMIT;
    }
    if (target_spCm->offical > 0 && cm->offical >= target_spCm->offical)
    {
        return HC_ERROR_CORPS_OFFICAL_LIMIT;
    }

    return cp->removeMember(tcid, corps_event_fire, cm->cdata->m_name);
}

//解散军团
int corpsMgr::dissolve(int cid, int corps, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm || !cm->cdata.get())
    {
        return HC_ERROR;
    }
    if (cm->offical != 1)
    {
        return HC_ERROR;
    }

    //解散
    //删除军团
    InsertSaveDb("delete from char_corps where id=" + LEX_CAST_STR(cp->_id));
    //删除军团成员
    InsertSaveDb("delete from char_corps_members where corps=" + LEX_CAST_STR(cp->_id));
    //申请
    InsertSaveDb("delete from char_corps_applications where corps=" + LEX_CAST_STR(cp->_id));
    //历史记录
    InsertSaveDb("delete from char_corps_applications where corps=" + LEX_CAST_STR(cp->_id));

    boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(cm->cdata->GetGuildId());

    //删除角色的军团数据
    std::list<boost::shared_ptr<corps_member> >::iterator it_m = cp->_members_list.begin();
    while (it_m != cp->_members_list.end())
    {
        if (it_m->get() && it_m->get()->cdata.get())
        {
            if (gch.get())
            {
                //离开军团频道
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(it_m->get()->cdata->m_name);
                if (account.get())
                {
                    gch->Remove(account);
                }
            }
            it_m->get()->cdata->m_corps_member.reset();
            it_m->get()->cdata->NotifyCharData();
        }
        ++it_m;
    }
    //删除角色的申请数据
    std::map<int, boost::shared_ptr<corps_application> >::iterator it_a = cp->_applications.begin();
    while (it_a != cp->_applications.end())
    {
        if (it_a->second.get() && it_a->second->cdata.get())
        {
            std::list<boost::shared_ptr<corps_application> >::iterator it_cc = it_a->second->cdata->m_corps_applications.begin();
            while (it_cc != it_a->second->cdata->m_corps_applications.end())
            {
                if (it_cc->get() && (*it_cc)->corps == corps)
                {
                    it_a->second->cdata->m_corps_applications.erase(it_cc);
                    break;
                }
                ++it_cc;
            }
        }
        ++it_a;
    }
    //删除军团
    m_corps_maps.erase(cp->_id);
    m_corps_maps2.erase(cp->_name);
    std::list<boost::shared_ptr<splsCorps> >::iterator itx = m_corps_list.begin();
    while (itx != m_corps_list.end())
    {
        if (itx->get() && itx->get()->_id == corps)
        {
            m_corps_list.erase(itx);
            break;
        }
        ++itx;
    }
    updateRank();
    return HC_SUCCESS;
}

//查询副军团长信息
int corpsMgr::getAssistants(int cid, int corps, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    json_spirit::Array alist;
    std::list<boost::shared_ptr<corps_member> >::iterator it2 = cp->_assistant.begin();    //副军团长
    while (it2 != cp->_assistant.end())
    {
        if (it2->get() && it2->get()->cdata.get())
        {
            corps_member* cm = it2->get();
            CharData* cdata = cm->cdata.get();
            json_spirit::Object obj;
            obj.push_back( Pair("id", cm->cid) );
            obj.push_back( Pair("name", cdata->m_name) );
            obj.push_back( Pair("level", cdata->m_level) );
            obj.push_back( Pair("post", cm->offical) );
            alist.push_back(obj);            
        }
        ++it2;
    }
    robj.push_back( Pair("list", alist) );
    return HC_SUCCESS;
}

//查询军团列表
int corpsMgr::getCorpsList(int cid, int page, int nums_per_page, json_spirit::Object& robj)
{
    //m_corps_list.sort(compare_corps);

    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page == 0)
    {
        nums_per_page = 10;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;

    json_spirit::Array clist;
    std::list<boost::shared_ptr<splsCorps> >::iterator it = m_corps_list.begin();
    while (it != m_corps_list.end())
    {
        if (it->get())
        {
            splsCorps* cp = it->get();
            
            {
                ++cur_nums;
                if (cur_nums >= first_nums && cur_nums <= last_nums)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("id", cp->_id) );
                    obj.push_back( Pair("rank", cp->_rank) );
                    obj.push_back( Pair("name", cp->_name) );
                    //obj.push_back( Pair("camp", cp->_camp) );
                    obj.push_back( Pair("level", cp->_level) );
                    if (cp->_leader.get() && cp->_leader->cdata.get())
                    {
                        obj.push_back( Pair("leader", cp->_leader->cdata->m_name) );
                    }
                    obj.push_back( Pair("nums", cp->_members_list.size()) );
                    obj.push_back( Pair("numsLimit", cp->_memberLimit) );
                    obj.push_back( Pair("isApply", cp->haveApplication(cid)?1:0) );
                    clist.push_back(obj);
                }
            }
        }
        ++it;
    }
    robj.push_back( Pair("list", clist) );

    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//更新排名
int corpsMgr::updateRank()
{
    m_corps_list.sort(compare_corps);
    int rank = 0;
    std::list<boost::shared_ptr<splsCorps> >::iterator it = m_corps_list.begin();    
    while (it != m_corps_list.end())
    {
        if (it->get())
        {
            it->get()->_rank = ++rank;
        }
        ++it;
    }
    return HC_SUCCESS;
}

//搜索军团
int corpsMgr::searchCorps(int cid, const std::string& leader, const std::string& corpsName, json_spirit::Object& robj)
{
    splsCorps* cp = NULL;
    json_spirit::Array clist;
    if (leader != "")
    {
        std::list<boost::shared_ptr<splsCorps> >::iterator it = m_corps_list.begin();
        while (it != m_corps_list.end())
        {
            if (it->get() && it->get()->_leader.get()
                && it->get()->_leader->cdata.get()
                && it->get()->_leader->cdata.get()->m_name.find(leader) != std::string::npos)
            {
                cp = it->get();
                if (cp != NULL)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("id", cp->_id) );
                    obj.push_back( Pair("rank", cp->_rank) );
                    //obj.push_back( Pair("camp", cp->_camp) );
                    obj.push_back( Pair("name", cp->_name) );
                    obj.push_back( Pair("level", cp->_level) );
                    obj.push_back( Pair("leader", cp->_leader->cdata->m_name) );
                    obj.push_back( Pair("nums", cp->_members_list.size()) );
                    obj.push_back( Pair("numsLimit", cp->_memberLimit) );
                    obj.push_back( Pair("isApply", cp->haveApplication(cid)?1:0) );
                    clist.push_back(obj);
                }
                
            }
            ++it;
        }
    }
    else if ("" != corpsName)
    {
        std::list<boost::shared_ptr<splsCorps> >::iterator it = m_corps_list.begin();
        while (it != m_corps_list.end())
        {
            if (it->get()
                && it->get()->_name.find(corpsName) != std::string::npos)
            {
                cp = it->get();
                if (cp != NULL)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("id", cp->_id) );
                    obj.push_back( Pair("rank", cp->_rank) );
                    obj.push_back( Pair("name", cp->_name) );
                    obj.push_back( Pair("level", cp->_level) );
                    if (it->get()->_leader.get() && it->get()->_leader->cdata.get())
                    {
                        obj.push_back( Pair("leader", cp->_leader->cdata->m_name) );
                    }
                    obj.push_back( Pair("nums", cp->_members_list.size()) );
                    obj.push_back( Pair("numsLimit", cp->_memberLimit) );
                    obj.push_back( Pair("isApply", cp->haveApplication(cid)?1:0) );
                    clist.push_back(obj);
                }
            }
            ++it;
        }
    }
    robj.push_back( Pair("list", clist) );
    return HC_SUCCESS;
}

//查询已申请军团
int corpsMgr::getApplicationCorps(int cid, json_spirit::Object& robj)
{
    json_spirit::Array clist;
    std::list<boost::shared_ptr<splsCorps> >::iterator it = m_corps_list.begin();
    while (it != m_corps_list.end())
    {
        if (it->get())
        {
            splsCorps* cp = it->get();
            if (cp->_leader.get() && cp->_leader->cdata.get() && cp->haveApplication(cid))
            {
                json_spirit::Object obj;
                obj.push_back( Pair("id", cp->_id) );
                obj.push_back( Pair("rank", cp->_rank) );
                obj.push_back( Pair("name", cp->_name) );
                obj.push_back( Pair("level", cp->_level) );
                obj.push_back( Pair("leader", cp->_leader->cdata->m_name) );
                obj.push_back( Pair("nums", cp->_members_list.size()) );
                obj.push_back( Pair("numsLimit", cp->_memberLimit) );
                obj.push_back( Pair("isApply", 1) );

                clist.push_back(obj);
            }
        }
        ++it;
    }
    robj.push_back( Pair("list", clist) );

    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", 1) );
    pageobj.push_back( Pair("page", 1) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//增加军团事件
int corpsMgr::addEvent(CharData* pc,int type,int param1,int param2)
{
    if (!pc->m_corps_member.get())
    {
        return HC_ERROR;
    }
    int corps = pc->m_corps_member->corps;
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }

    corpsEvent* ce = NULL;
    switch (type)
    {
        case corps_event_add_exp:
            {
                pc->m_corps_member->contribution += param1;
                pc->m_corps_member->contribution_day += param1;
                pc->m_corps_member->save();
                std::string msg = strCorpsMsg1;
                str_replace_all(msg, "$D", LEX_CAST_STR(param1));
                ce = new corpsEvent;
                ce->etype = type;
                ce->cid = pc->m_id;
                ce->corps = corps;
                ce->inputTime = time(NULL);
                ce->name = pc->m_name;
                ce->msg = msg;
                
                if (cp->_level < 10)
                {
                    cp->_exp += param1;
                    cp->_expWeek += param1;
                    //判断是否能够升级了
                    if (cp->_exp >= cp->_maxExp)
                    {
                        ++cp->_level;
                        for (int i = 1; i <= max_corps_action; ++i)
                        {
                            if (cp->_level == getCorpsActionLevel(i))
                            {
                                std::list<boost::shared_ptr<corps_member> >::iterator it_l = cp->_members_list.begin();
                                while (it_l != cp->_members_list.end())
                                {
                                    if (it_l->get() && it_l->get()->cdata.get())
                                    {
                                        corps_member* cm = it_l->get();
                                        CharData* cdata = cm->cdata.get();
                                        cdata->NotifyCharOpenInfo();
                                    }
                                    ++it_l;
                                }
                                break;
                            }
                        }
                        if (cp->_level == iCorpsLevelForParty)
                        {
                            std::list<boost::shared_ptr<corps_member> >::iterator it_l = cp->_members_list.begin();
                            while (it_l != cp->_members_list.end())
                            {
                                if (it_l->get() && it_l->get()->cdata.get())
                                {
                                    corps_member* cm = it_l->get();
                                    CharData* cdata = cm->cdata.get();
                                    cdata->NotifyCharOpenInfo();
                                }
                                ++it_l;
                            }
                        }
                        cp->_exp -= cp->_maxExp;
                        getBaseCorps(cp->_level, cp->_maxExp, cp->_memberLimit, cp->_assistantLimit);
                    }
                }
                cp->save();
                cp->sort();
            }
            break;
        case corps_event_new_leader:
            {
                boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(param1);
                if (cdata.get())
                {
                    std::string msg =strCorpsMsg2;
                    str_replace(msg, "$W", "");//pc->m_name);
                    str_replace(msg, "$T", cdata->m_name);
                    ce = new corpsEvent;
                    ce->etype = type;
                    ce->cid = pc->m_id;
                    ce->corps = corps;
                    ce->inputTime = time(NULL);
                    ce->name = pc->m_name;
                    ce->msg = msg;
                }
            }
            break;
        case corps_event_appointment:
            break;
        case corps_event_donate_gold:
            {
                int add_exp = 20*param1;
                pc->m_corps_member->contribution += add_exp;
                pc->m_corps_member->contribution_day += add_exp;
                std::string msg =strCorpsMsgDonate;
                str_replace_all(msg, "$G", LEX_CAST_STR(param1));
                str_replace_all(msg, "$E", LEX_CAST_STR(add_exp));
                pc->m_corps_member->save();
                ce = new corpsEvent;
                ce->etype = type;
                ce->cid = pc->m_id;
                ce->corps = corps;
                ce->inputTime = time(NULL);
                ce->name = pc->m_name;
                ce->msg = msg;
                
                cp->_exp += add_exp;
                cp->_expWeek += add_exp;

                if (cp->_level < 10)
                {
                    //判断是否能够升级了
                    if (cp->_exp >= cp->_maxExp)
                    {
                        ++cp->_level;
                        for (int i = 1; i <= max_corps_action; ++i)
                        {
                            if (cp->_level == getCorpsActionLevel(i))
                            {
                                std::list<boost::shared_ptr<corps_member> >::iterator it_l = cp->_members_list.begin();
                                while (it_l != cp->_members_list.end())
                                {
                                    if (it_l->get() && it_l->get()->cdata.get())
                                    {
                                        corps_member* cm = it_l->get();
                                        CharData* cdata = cm->cdata.get();
                                        cdata->NotifyCharOpenInfo();
                                    }
                                    ++it_l;
                                }
                                break;
                            }
                        }
                        cp->_exp -= cp->_maxExp;
                        getBaseCorps(cp->_level, cp->_maxExp, cp->_memberLimit, cp->_assistantLimit);
                    }
                }
                cp->save();
                cp->sort();
            }
            break;
        case corps_event_explore:    //军团探索
        {
            //类别
            std::string name = Singleton<corpsExplore>::Instance().getName(param1%10);
            //品质
            addColor(name, param1/10);

            //增加exp
            int add_exp = param2;
            pc->m_corps_member->contribution += add_exp;
            pc->m_corps_member->contribution_day += add_exp;
            std::string msg =strCorpsMsgExplore;
            str_replace_all(msg, "$N", name);
            str_replace_all(msg, "$D", LEX_CAST_STR(add_exp));
            pc->m_corps_member->save();
            ce = new corpsEvent;
            ce->etype = type;
            ce->cid = pc->m_id;
            ce->corps = corps;
            ce->inputTime = time(NULL);
            ce->name = pc->m_name;
            ce->msg = msg;
            
            cp->_exp += add_exp;
            cp->_expWeek += add_exp;

            if (cp->_level < 10)
            {
                //判断是否能够升级了
                if (cp->_exp >= cp->_maxExp)
                {
                    ++cp->_level;
                    for (int i = 1; i <= max_corps_action; ++i)
                    {
                        if (cp->_level == getCorpsActionLevel(i))
                        {
                            std::list<boost::shared_ptr<corps_member> >::iterator it_l = cp->_members_list.begin();
                            while (it_l != cp->_members_list.end())
                            {
                                if (it_l->get() && it_l->get()->cdata.get())
                                {
                                    corps_member* cm = it_l->get();
                                    CharData* cdata = cm->cdata.get();
                                    cdata->NotifyCharOpenInfo();
                                }
                                ++it_l;
                            }
                            break;
                        }
                    }
                    cp->_exp -= cp->_maxExp;
                    getBaseCorps(cp->_level, cp->_maxExp, cp->_memberLimit, cp->_assistantLimit);
                }
            }
            cp->save();
            cp->sort();
            break;
        }
    }
    if (NULL != ce)
    {
        InsertSaveDb("insert into char_corps_event (corps,cid,name,msg,input,type) values (" + LEX_CAST_STR(ce->corps)
                        + "," + LEX_CAST_STR(ce->cid) + ",'" + GetDb().safestr(ce->name)
                        + "','" + GetDb().safestr(ce->msg) + "',unix_timestamp(),type)");
        boost::shared_ptr<corpsEvent> sp_ce(ce);
        cp->_event_list.insert(cp->_event_list.begin(), sp_ce);
        while (cp->_event_list.size() >= 20)
        {
            boost::shared_ptr<corpsEvent> e = cp->_event_list.back();
            if (e.get())
            {
                InsertSaveDb("delete from char_corps_event where type='1' and corps=" + LEX_CAST_STR(e->corps)
                            + " and input<" + LEX_CAST_STR(e->inputTime));
            }
            cp->_event_list.pop_back();
        }
    }
    return HC_SUCCESS;
}

//军团等级对应的经验/人数上限
int corpsMgr::getBaseCorps(int level, int& exp, int& numsLimit, int& assistLimit)
{
    if (level >= 1 && level <= (int)m_base_corps.size())
    {
        exp = m_base_corps[level-1]._maxExp;
        numsLimit = m_base_corps[level-1]._memberLimit;
        assistLimit = m_base_corps[level-1]._assistantLimit;
        return HC_SUCCESS;
    }
    else
    {
        exp = 0;
        //numsLimit = 0;
        //assistLimit = 0;
        return HC_SUCCESS;
    }
}

//设置军团信息
int corpsMgr::setCorpsInfo(int cid, int corps, const std::string& memo,
            const std::string& intro, const std::string& qq, json_spirit::Object& robj)
{
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //找到军团看是否有权限看
    boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
    corps_member* cm = spCm.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    if (cm->offical != 1)
    {
        return HC_ERROR_CORPS_OFFICAL_LIMIT;
    }

    cp->_qqGroup = qq;
    cp->_strBulletin = memo;
    cp->_strIntroduction = intro;
    cp->save();
    return HC_SUCCESS;
}

//删除角色
int corpsMgr::deleteChar(int cid)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (NULL != pc)
    {
        //推出军团
        if (pc->m_corps_member.get())
        {            
            //找军团
            splsCorps* cp = findCorps(pc->m_corps_member.get()->corps);
            if (!cp)
            {
                return HC_ERROR_NOT_JOIN_JT;
            }
            {
                //找到军团看是否有权限看
                boost::shared_ptr<corps_member> spCm = cp->getMember(cid);
                corps_member* cm = spCm.get();
                if (cm)
                {
                    if (cm->offical == 1)
                    {
                        if (cp->_assistant.size() > 0 && cp->_assistant.begin()->get())
                        {
                            cp->setOffical(cp->_assistant.begin()->get()->cid, 1);
                            cp->removeMember(cid);
                        }
                        else
                        {
                            int tocid = 0;
                            int gongxian = -1;
                            std::list<boost::shared_ptr<corps_member> >::iterator it = cp->_members_list.begin();
                            while (it != cp->_members_list.end())
                            {
                                if (it->get())
                                {
                                    corps_member* cmm = it->get();
                                    if (cmm->contribution > gongxian)
                                    {
                                        tocid = cmm->cid;
                                        gongxian = cmm->contribution;
                                    }
                                }
                                ++it;
                            }
                            if (tocid > 0)
                            {
                                cp->setOffical(tocid, 2);
                                cp->setOffical(tocid, 1);
                                cp->removeMember(cid);
                            }
                            else
                            {
                                json_spirit::Object robj;
                                dissolve(cid, pc->m_corps_member.get()->corps, robj);
                            }
                        }
                    }
                    else
                    {
                        cp->removeMember(cid);
                    }
                }
            }
        }
        if (pc->m_corps_applications.size())
        {
            //取消该角色其他的申请
            std::map<int, boost::shared_ptr<splsCorps> >::iterator it = m_corps_maps.begin();
            while (it != m_corps_maps.end())
            {
                if (it->second.get())
                {
                    it->second->cancelApplication(cid);
                }
                ++it;
            }
            pc->m_corps_applications.clear();
        }
    }
    return HC_SUCCESS;
}

//查询祭祀信息 getTroopsFeteList
int corpsMgr::getJisiInfo(CharData* pc, json_spirit::Object& robj)
{
    if (!pc->m_corps_member.get())
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int corps = pc->m_corps_member->corps;
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    robj.push_back( Pair("fete", pc->m_temp_jisi_times == 0 ? 1 : 0) );

    json_spirit::Array elist;
    std::list<boost::shared_ptr<corpsEvent> >::iterator it_e = cp->_js_event_list.begin();
    while (it_e != cp->_js_event_list.end())
    {
        if (it_e->get())
        {
            corpsEvent* pe = it_e->get();
            json_spirit::Object obj;
            obj.push_back( Pair("id", pe->cid) );
            obj.push_back( Pair("name", pe->name) );
            obj.push_back( Pair("memo", pe->msg) );
            obj.push_back( Pair("time", time(NULL)-pe->inputTime) );
            elist.push_back(obj);
        }
        ++it_e;
    }
    robj.push_back( Pair("list", elist) );
    return HC_SUCCESS;
}

//祭祀 troopsFete
int corpsMgr::jiSi(CharData* pc, int type)
{
    if (pc->m_temp_jisi_times > 0)
    {
        return HC_ERROR_CORPS_ALREADY_JISI;
    }
    if (!pc->m_corps_member.get())
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int corps = pc->m_corps_member->corps;
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    return cp->jiSi(pc, type);
}

//查看宴会信息
int corpsMgr::getYanhui(CharData* pc, json_spirit::Object& robj)
{
    if (!pc->m_corps_member.get())
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int corps = pc->m_corps_member->corps;
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    json_spirit::Array ylist;
    for (int i = 0; i < iCorpsYanhuiMax; ++i)
    {
        if (cp->_corps_yanhui[i]._id)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("position", i+1) );
            obj.push_back( Pair("id", cp->_corps_yanhui[i]._id) );
            obj.push_back( Pair("name", cp->_corps_yanhui[i]._name) );            
            obj.push_back( Pair("spic", cp->_corps_yanhui[i]._spic) );
            obj.push_back( Pair("type", cp->_corps_yanhui[i]._type) );
            ylist.push_back(obj);
        }
    }
    robj.push_back( Pair("list", ylist) );
    return HC_SUCCESS;
}

//是否加入宴会，返回位置
int corpsMgr::inYanhui(CharData* pc)
{
    if (!pc->m_corps_member.get())
    {
        return 0;
    }
    int corps = pc->m_corps_member->corps;
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    json_spirit::Array ylist;
    for (int i = 0; i < iCorpsYanhuiMax; ++i)
    {
        if (cp->_corps_yanhui[i]._id == pc->m_id)
        {
            return i+1;
        }
    }
    return 0;
}

//加入宴会
int corpsMgr::JoinYanhui(CharData* pc, int pos)
{
    if (!pc->m_corps_member.get())
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    if (time(NULL) - pc->m_corps_member->join_time < 43200)
    {
        return HC_ERROR_CORPS_NEED_12H;
    }
    int corps = pc->m_corps_member->corps;
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    if (inYanhui(pc))
    {
        return HC_ERROR_CORPS_ALREADY_IN_PARTY;
    }
    if (cp->_level < getCorpsActionLevel(corps_action_yanhui))
    {
        return HC_ERROR_CORPS_PARYTY_LEVEL;
    }
    return cp->JoinYanhui(pc, pos);
}

//邀请路人加入宴会
int corpsMgr::inviteSomeone(CharData* pc, int pos)
{
    if (!pc->m_corps_member.get())
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int corps = pc->m_corps_member->corps;
    //找军团
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    //if (!inYanhui(pc))
    //{
    //    return HC_ERROR_CORPS_ALREADY_IN_PARTY;
    //}
    if (cp->_level < getCorpsActionLevel(corps_action_yanhui))
    {
        return HC_ERROR_CORPS_PARYTY_LEVEL;
    }
    return cp->inviteSomeone(pc, pos);
}

//查找军团
splsCorps* corpsMgr::findCorps(int corps)
{
    //找军团
    std::map<int, boost::shared_ptr<splsCorps> >::iterator it = m_corps_maps.find(corps);
    if (it == m_corps_maps.end() || !it->second.get())
    {
        return NULL;
    }
    return it->second.get();
}

//每周重置本周获得经验
void corpsMgr::weekReset()
{
    std::list<boost::shared_ptr<splsCorps> >::iterator it = m_corps_list.begin();    
    while (it != m_corps_list.end())
    {
        if (it->get())
        {
            it->get()->_expWeek = 0;
        }
        ++it;
    }
}

//每日重置本日貢獻
void corpsMgr::dailyReset()
{
    std::list<boost::shared_ptr<splsCorps> >::iterator it = m_corps_list.begin();    
    while (it != m_corps_list.end())
    {
        if (it->get())
        {
            it->get()->dailyReset();
        }
        ++it;
    }
}

void corpsMgr::setCorpsExp(int corps, int level, int exp, int weekExp)
{
    splsCorps* cp = findCorps(corps);
    if (!cp)
    {
        return;
    }
    cp->_level = level;
    cp->_exp = exp;
    cp->_expWeek = weekExp;
    cp->save();
}

void corpsMgr::setCharContribution(CharData& cdata, int contribute)
{
    if (cdata.m_corps_member.get())
    {
        cdata.m_corps_member->contribution = contribute;
        cdata.m_corps_member->contribution_day = 0;
        cdata.m_corps_member->save();
    }
}

//查询军团活动所需等级
int corpsMgr::getCorpsActionLevel(int id)
{
    int needlevel = 0;
    for (std::vector<base_corps_action>::iterator it = m_base_corps_action.begin(); it != m_base_corps_action.end(); ++it)
    {
        if (it->_id == id)
        {
            needlevel = it->_needlevel;
            break;
        }
    }
    return needlevel;
}

//查询军团活动页面
int corpsMgr::getCorpsActionList(CharData* pc, json_spirit::Object& robj)
{
    json_spirit::Array list;
    for (std::vector<base_corps_action>::iterator it = m_base_corps_action.begin(); it != m_base_corps_action.end(); ++it)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("id", it->_id) );
        obj.push_back( Pair("spic", it->_id) );
        obj.push_back( Pair("needlevel", it->_needlevel) );
        obj.push_back( Pair("name", it->_name) );
        obj.push_back( Pair("award_memo", it->_award_memo) );
        obj.push_back( Pair("memo", it->_memo) );
        int state = 0;
        if (it->_id == corps_action_jisi)
        {
            state = pc->m_temp_jisi_times == 0 ? 0 : 2;
        }
        else if (it->_id == corps_action_sheji)
        {
            int nums = pc->queryExtraData(char_data_type_daily, char_data_daily_corps_ymsj);
            if (pc->m_corps_member->ymsj_can_get)
            {
                state = 1;
            }
            else if(nums >= 3)
            {
                state = 2;
            }
            else
            {
                //辕门射戟倒计时
                splsCorps* cp = corpsMgr::getInstance()->findCorps(pc->m_corps_member->corps);
                if (cp)
                {
                    if (cp->m_ymsj.next_time > time(NULL))
                    {
                        obj.push_back( Pair("leftTime", cp->m_ymsj.next_time - time(NULL)) );
                    }
                }
            }
        }
        else if (it->_id == corps_action_tansuo)
        {
            int nums = pc->queryExtraData(char_data_type_daily, char_data_daily_corps_explore);
            charCorpsExplore* c = Singleton<corpsExplore>::Instance().getChar(pc->m_id).get();
            int corps_explore_times = 0;
            splsCorps* cp = corpsMgr::getInstance()->findCorps(pc->m_corps_member.get()->corps);
            if (cp)
            {
                corps_explore_times = iCorpsExploreTimesOneday[cp->_level];
            }
            if (c && c->isDone() && nums < corps_explore_times)
            {
                state = 1;
            }
            else if (nums >= corps_explore_times)
            {
                state = 2;
            }
            else if(c && c->getDoneTime())
            {
                obj.push_back( Pair("leftTime", c->getDoneTime()) );
            }
        }
        else if (corps_action_fighting == it->_id)
        {
            state = Singleton<corpsFightingMgr>::Instance().getSignupState(pc->m_corps_member->corps, pc->m_id);
        }
        else if (corps_action_jt_boss == it->_id)
        {
            if (pc->m_corps_member.get())
            {
                state = bossMgr::getInstance()->getJtBossState(pc->m_corps_member->corps);
            }
        }
        obj.push_back( Pair("state", state) );
        list.push_back(obj);
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//为了修改老数据，更新军团人数上限
void corpsMgr::updateCorpsMemberLimit()
{
    for (std::map<int, boost::shared_ptr<splsCorps> >::iterator it = m_corps_maps.begin(); it != m_corps_maps.end(); ++it)
    {
        if (it->second.get())
        {
            splsCorps* cp = it->second.get();
            getBaseCorps(cp->_level, cp->_maxExp, cp->_memberLimit, cp->_assistantLimit);
            cp->save();
        }
    }
}

void corpsMgr::openJisiEvent(int fac, int last_mins)
{
    getJisiFac();
    if (fac > 100)
    {
        m_jisi_event_fac = fac;
        m_jisi_event_endtime = time(NULL) + 60 * last_mins;

        GeneralDataMgr::getInstance()->setInt("jisi_fac", m_jisi_event_fac);
        GeneralDataMgr::getInstance()->setInt("jisi_event_end", m_jisi_event_endtime);
    }
    else if (fac <= 100 && m_jisi_event_fac > 100)
    {
        m_jisi_event_fac = 100;
        m_jisi_event_endtime = 0;
        GeneralDataMgr::getInstance()->setInt("jisi_fac", m_jisi_event_fac);
        GeneralDataMgr::getInstance()->setInt("jisi_event_end", m_jisi_event_endtime);
    }
}

int corpsMgr::getJisiFac()
{
    if (time(NULL) < m_jisi_event_endtime)
    {
        return m_jisi_event_fac;
    }
    else
    {
        if (m_jisi_event_fac > 100)
        {
            m_jisi_event_fac = 100;
            GeneralDataMgr::getInstance()->setInt("jisi_fac", m_jisi_event_fac);
        }
        return 100;
    }
}

std::string corpsMgr::randomCorpsName()
{
    return m_robot_corps_name[my_random(0, m_robot_corps_name.size()-1)];
}

std::string corpsMgr::randomCorpsMemo()
{
    return m_robot_corps_memo[my_random(0, m_robot_corps_memo.size()-1)];
}

void corpsMgr::createRobotCorps()
{
    int loop_time = 0;
    std::string name = randomCorpsName();
    while (m_corps_maps2.find(name) != m_corps_maps2.end())
    {
        ++loop_time;
        name = randomCorpsName();

        if (loop_time > 100)
        {
            name = name + LEX_CAST_STR(time(NULL));
            break;
        }
    }

    std::string memo = randomCorpsMemo();
    boost::shared_ptr<splsCorps> spCorps(new splsCorps(++max_corps, 1, 1, 0, 0, 20, 2, name, "", memo, "", 0, "", ""));
    m_corps_maps[max_corps] = spCorps;
    m_corps_maps2[name] = spCorps;
    spCorps->_createTime = time(NULL);
    m_corps_list.push_back(spCorps);
    spCorps->_rank = m_corps_list.size();
    spCorps->m_auto_recruit_msg = 2;

    InsertSaveDb("insert into char_corps (id,camp,name,flag,level,member_limit,assistantLimit,creater,exp,expWeek,qqGroup,bulletin,introduction,createTime) values ("
        + LEX_CAST_STR(max_corps) + "," + LEX_CAST_STR(1) + ",'" + GetDb().safestr(name) + "','',1,20,2," + LEX_CAST_STR(0) + ",0,0,'','" + GetDb().safestr(memo) + "',''," + LEX_CAST_STR(spCorps->_createTime) +")");
    ++m_total_robot_corps;
}

//开启关闭机器人军团 1开启，0关闭
void corpsMgr::openRobotCorps(int state)
{
    if (state)
    {
        m_open_robot_corps = true;
    }
    else
    {
        m_open_robot_corps = false;
    }
    GeneralDataMgr::getInstance()->setInt("robot_corps", state);
}

//设置机器军团团长
void corpsMgr::setRobotCorpsLeader(int id, int cid)
{
    splsCorps* cp = findCorps(id);
    if (cp && cp->_leader.get() == NULL)
    {
        std::list<boost::shared_ptr<corps_member> >::iterator it_l = cp->_members_list.begin();
        while (it_l != cp->_members_list.end())
        {
            if (it_l->get() && it_l->get()->cdata.get() && it_l->get()->cid == cid)
            {
                if (it_l->get()->offical == 2)
                {
                    std::list<boost::shared_ptr<corps_member> >::iterator it = cp->_assistant.begin();
                    while (it != cp->_assistant.end())
                    {
                        if ((*it)->cid == cid)
                        {
                            cp->_assistant.erase(it);
                            break;
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
                it_l->get()->offical = 1;
                cp->_leader = *it_l;
                it_l->get()->save();
                cp->sort();
                --m_total_robot_corps;
                return;
            }
        }
    }
}

//检查机器人军团
void corpsMgr::checkRobotCorps()
{
    if (m_total_robot_corps <= 0)
    {
        return;
    }
    time_t time_now = time(NULL);
    std::list<boost::shared_ptr<splsCorps> >::iterator it = m_corps_list.begin();    
    while (it != m_corps_list.end())
    {
        if (it->get())
        {
            splsCorps* cp = it->get();
            //是机器人军团
            if (cp->_leader.get() == NULL)
            {
                //发布招募公告
                if (cp->m_auto_recruit_msg && (cp->m_auto_recruit_msg_time+600) < time(NULL))
                {
                    cp->m_auto_recruit_msg_time = time(NULL);
                    std::string msg = strCorpsRecruitMsg;
                    str_replace(msg, "$N", cp->_name);
                    str_replace(msg, "$n", LEX_CAST_STR(cp->_id));
                    GeneralDataMgr::getInstance()->broadCastSysMsg(msg,-1);
                    --cp->m_auto_recruit_msg;
                }
                //72小时后，转让军团长
                if (cp->_createTime + iONE_DAY_SECS*3 < time_now)
                {                    
                    std::list<boost::shared_ptr<corps_member> >::iterator it = cp->_assistant.begin();
                    while (it != cp->_assistant.end())
                    {
                        if ((*it).get())
                        {
                            (*it)->offical = 0;
                        }
                        ++it;
                    }
                    cp->_assistant.clear();

                    std::list<boost::shared_ptr<corps_member> >::iterator it_l = cp->_members_list.begin();
                    while (it_l != cp->_members_list.end())
                    {
                        if (it_l->get() && it_l->get()->cdata.get())
                        {
                            corps_member* cm = it_l->get();
                            CharData* cdata = cm->cdata.get();
                            if (cp->_leader.get() == NULL)
                            {
                                cp->_leader = *it_l;
                                cm->offical = 1;
                                cm->save();
                            }
                            else if ((int)cp->_assistant.size() < cp->_assistantLimit)
                            {
                                cp->_assistant.push_back(*it_l);
                                cm->offical = 2;
                                cm->save();
                            }
                            else
                            {
                                break;
                            }
                        }
                        ++it_l;
                    }    
                    cp->sort();
                    --m_total_robot_corps;
                }
                //48小时候，如果没副团，自动设置副团
                else if (cp->_assistant.size() == 0 && (cp->_createTime + iONE_DAY_SECS*2) < time_now)
                {
                    std::list<boost::shared_ptr<corps_member> >::iterator it_l = cp->_members_list.begin();
                    while (it_l != cp->_members_list.end())
                    {
                        if (it_l->get() && it_l->get()->cdata.get())
                        {
                            corps_member* cm = it_l->get();
                            CharData* cdata = cm->cdata.get();
                            if ((int)cp->_assistant.size() < cp->_assistantLimit)
                            {
                                cp->_assistant.push_back(*it_l);
                                cm->offical = 2;
                                cm->save();
                            }
                            else
                            {
                                break;
                            }
                        }
                        ++it_l;
                    }
                }
                else
                {
                    //军团满员后，自动踢人
                    if (cp->_memberLimit <= (int)cp->_members_list.size())
                    {
                        std::list<int> remove_cids;
                        std::list<boost::shared_ptr<corps_member> >::iterator it_l = cp->_members_list.begin();
                        while (it_l != cp->_members_list.end())
                        {
                            if (it_l->get() && it_l->get()->cdata.get())
                            {
                                corps_member* cm = it_l->get();
                                CharData* cdata = cm->cdata.get();
                                if (cdata->m_is_online == 0)
                                {
                                    int offline_time = (time_now - cdata->m_login_time)/60;
                                    if ((cdata->m_level < 29 && offline_time >= 120)
                                        || (cdata->m_level >= 29 && cdata->m_level <= 32 && offline_time >= 720)
                                        || (offline_time >= 1440))
                                    {
                                        /*  29级以下且非在线状态玩家，离线时间超过2小时，踢出军团；
                                            29-32级之间，离线时间超过12个小时，踢出军团；
                                            33级以上，离线时间超过24个小时，踢出军团    */
                                        remove_cids.push_back(cdata->m_id);
                                    }
                                }                                           
                            }
                            ++it_l;
                        }
                        for (std::list<int>::iterator itc = remove_cids.begin(); itc != remove_cids.end(); ++itc)
                        {
                            //让他们自动退团
                            cp->removeMember(*itc, corps_event_leave, "");
                        }
                    }
                }
            }            
        }
        ++it;
    }
}

//查询祭祀信息
int ProcessGetFeteList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return corpsMgr::getInstance()->getJisiInfo(cdata.get(), robj);
}

//祭祀
int ProcessFete(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    return corpsMgr::getInstance()->jiSi(cdata.get(), type);
}

//查询宴会
int ProcessQueryYanhui(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return corpsMgr::getInstance()->getYanhui(cdata.get(), robj);
}

//召集宴会
int ProcessYanhuiZhaoji(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    boost::shared_ptr<OnlineUser> account = psession->user();
    if (!account.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (!account->m_onlineCharactor.get() || !account->m_onlineCharactor->m_charactor.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (!corpsMgr::getInstance()->inYanhui(account->m_onlineCharactor->m_charactor.get()))
    {
        return HC_ERROR_CORPS_NOT_IN_PARTY;
    }
    CharData* pc = account->m_onlineCharactor->m_charactor.get();
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int corps = pc->m_corps_member->corps;
    //找军团    
    splsCorps* cp = corpsMgr::getInstance()->findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    
    std::string inviteMsg = corpsMgr::getInstance()->zhaojiMsg();
    int counts = cp->queryYanhuiPerson();
    str_replace(inviteMsg, "$N", LEX_CAST_STR(7-counts));
    account->m_onlineCharactor->GuildChat(inviteMsg);
    return HC_SUCCESS;
}

//加入宴会
int ProcessJoinYanhui(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int pos = 1;
    READ_INT_FROM_MOBJ(pos,o,"position");
    return corpsMgr::getInstance()->JoinYanhui(cdata.get(), pos);
}

//邀请路人
int ProcessInviteSomeone(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int pos = 1;
    READ_INT_FROM_MOBJ(pos,o,"position");
    return corpsMgr::getInstance()->inviteSomeone(cdata.get(), pos);
}

//辕门射戟加入
int ProcessCorpsYmsjJoin(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    boost::shared_ptr<OnlineUser> account = psession->user();
    if (!account.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (!account->m_onlineCharactor.get() || !account->m_onlineCharactor->m_charactor.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    CharData* pc = account->m_onlineCharactor->m_charactor.get();
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int corps = pc->m_corps_member->corps;
    //找军团    
    splsCorps* cp = corpsMgr::getInstance()->findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int pos = 0;
    READ_INT_FROM_MOBJ(pos,o, "pos");
    return cp->m_ymsj.join(pos,  pc->m_corps_member, robj);
}

//辕门射戟选择
int ProcessCorpsYmsjChoose(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    boost::shared_ptr<OnlineUser> account = psession->user();
    if (!account.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (!account->m_onlineCharactor.get() || !account->m_onlineCharactor->m_charactor.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    CharData* pc = account->m_onlineCharactor->m_charactor.get();
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int corps = pc->m_corps_member->corps;
    //找军团    
    splsCorps* cp = corpsMgr::getInstance()->findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int c = 0;
    READ_INT_FROM_MOBJ(c,o,"c");
    return cp->m_ymsj.choose(pc->m_id, c);
}

//辕门射戟选择
int ProcessCorpsYmsjChoose(json_spirit::mObject& o )
{     
    int corps = 0, cid = 0;
    READ_INT_FROM_MOBJ(corps,o,"id");
    READ_INT_FROM_MOBJ(cid,o,"cid");
    //找军团    
    splsCorps* cp = corpsMgr::getInstance()->findCorps(corps);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    return cp->m_ymsj.choose(cid, 0);
}

//辕门射戟信息
int ProcessCorpsYmsjInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    boost::shared_ptr<OnlineUser> account = psession->user();
    if (!account.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (!account->m_onlineCharactor.get() || !account->m_onlineCharactor->m_charactor.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    CharData* pc = account->m_onlineCharactor->m_charactor.get();
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int corps = pc->m_corps_member->corps;
    //找军团    
    splsCorps* cp = corpsMgr::getInstance()->findCorps(corps);
    if (!cp || pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int nums = pc->queryExtraData(char_data_type_daily, char_data_daily_corps_ymsj);
    int left = 3 - nums;
    if (left > 0)
    {
        robj.push_back( Pair("left", left) );
    }
    if (pc->m_corps_member->ymsj_can_get)
    {
        robj.push_back( Pair("canGet", pc->m_corps_member->ymsj_can_get) );
    }
    int refresh = cp->m_ymsj.next_time - time(NULL);
    if (refresh > 0)
    {
        robj.push_back( Pair("refresh", refresh) );
    }
    else
    {
        json_spirit::Array list;
        int my_pos = 0;
        for (int i = 0; i < 2; ++i)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("pos", i+1) );
            if (cp->m_ymsj.m_char[i].get() && cp->m_ymsj.m_char[i]->cdata.get())
            {
                obj.push_back( Pair("spic", cp->m_ymsj.m_char[i]->cdata->m_spic) );
                obj.push_back( Pair("name", cp->m_ymsj.m_char[i]->cdata->m_name) );
                if (pc->m_id == cp->m_ymsj.m_char[i]->cid)
                {
                    my_pos = i + 1;
                }
            }
            list.push_back(obj);
        }

        robj.push_back( Pair("list", list) );
        if (my_pos)
        {
            robj.push_back( Pair("pos", my_pos) );
            //选择倒计时
            if (cp->m_ymsj.m_choice[my_pos-1] == 0)
            {
                int left_t = cp->m_ymsj.m_join_time[my_pos-1] + 60 - time(NULL);
                if (left_t > 0)
                {
                    robj.push_back( Pair("choose", left_t) );
                }
            }
            else
            {
                robj.push_back( Pair("choice", cp->m_ymsj.m_choice[my_pos-1]) );
            }

            if (cp->m_ymsj.m_char[2-my_pos].get() == NULL)
            {
                int left_t = cp->m_ymsj.m_join_time[my_pos-1] + iCorpsYmsjTimeout - time(NULL);
                if (left_t > 0)
                {
                    robj.push_back( Pair("winTimer", left_t) );
                }
            }
        }
    }
    return HC_SUCCESS;
}

//辕门射戟领取奖励
int ProcessCorpsYmsjAward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    boost::shared_ptr<OnlineUser> account = psession->user();
    if (!account.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (!account->m_onlineCharactor.get() || !account->m_onlineCharactor->m_charactor.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    CharData* pc = account->m_onlineCharactor->m_charactor.get();
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    if (pc->m_corps_member->ymsj_can_get > 0)
    {
        int err_code = 0;
        pc->addTreasure(treasure_type_gongxun, pc->m_corps_member->ymsj_can_get, err_code);
        add_statistics_of_treasure_cost(pc->m_id,pc->m_ip_address,treasure_type_gongxun,pc->m_corps_member->ymsj_can_get,treasure_corps_ymsj,1,pc->m_union_id,pc->m_server_id);
        robj.push_back( Pair("get", pc->m_corps_member->ymsj_can_get) );
        pc->m_corps_member->ymsj_can_get = 0;
        pc->m_corps_member->save();
    }
    
    //军团活动按钮
    json_spirit::Object action;
    action.push_back( Pair("cmd", "updateAction") );
    action.push_back( Pair("type", top_level_event_corp) );
    int state = 0;
    if (pc->m_corps_member->ymsj_can_get)
    {
        state = 1;
    }
    charCorpsExplore* c = Singleton<corpsExplore>::Instance().getChar(pc->m_id).get();
    if (c && c->isDone())
    {
        state = 1;
    }
    action.push_back( Pair("active", state) );
    action.push_back( Pair("s", 200) );
    pc->sendObj(action);

    return HC_SUCCESS;
}

//辕门射戟重置
int ProcessCorpsYmsjRefresh(json_spirit::mObject& o)
{
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    //找军团
    splsCorps* cp = corpsMgr::getInstance()->findCorps(id);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    
    cp->m_ymsj.refresh();
    return HC_SUCCESS;
}

//辕门射戟超时胜利
int ProcessCorpsYmsjWin(json_spirit::mObject& o)
{
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    //cout<<"ProcessCorpsYmsjWin->"<<id<<endl;
    //找军团
    splsCorps* cp = corpsMgr::getInstance()->findCorps(id);
    if (!cp)
    {
        ERR();
        return HC_ERROR_NOT_JOIN_JT;
    }
    cp->m_ymsj.timeout();
    return HC_SUCCESS;
}

//检查机器人军团
int ProcessRobotCorps(json_spirit::mObject& o)
{
    (void)o;
    corpsMgr::getInstance()->checkRobotCorps();
    return HC_SUCCESS;
}

