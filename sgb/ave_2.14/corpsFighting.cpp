
#include "corpsFighting.hpp"
#include "singleton.h"
#include "utils_all.h"
#include "spls_timer.h"
#include "igeneral.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "rewards.h"
#include "corps.h"

static const int iRoundDelay = 30;
static const int iRoundDelay2 = 30;

extern std::string strCorpsFightingStartMsg;
extern std::string strCorpsFightingRankMsg;

extern void InsertSaveDb(const std::string& sql);
Database& GetDb();
void InsertCombat(Combat* pcombat);
void InsertSaveCombat(Combat* pcombat);
//军团实际收益
void corpsRealReward(int& get);

bool compare_corps_fighting(boost::shared_ptr<corpsFighting> a, boost::shared_ptr<corpsFighting> b)
{
    if (!a.get())
    {
        return false;
    }
    if (!b.get())
    {
        return true;
    }
    else
    {
        return (a->lastround_alive < b->lastround_alive);
    }
}

bool compare_corpsfighting_member(boost::shared_ptr<corpsFihtingMember>& c1, boost::shared_ptr<corpsFihtingMember>& c2)
{
    if (c1.get() == NULL)
    {
        return false;
    }
    else if (c2.get() == NULL)
    {
        return true;
    }
    return c1->m_hp_percent >= c2->m_hp_percent;
}

corpsFihtingMember::corpsFihtingMember(corpsFighting& c)
:_corps(c)
{
    m_hp_percent = 100;
    for (int i = 0; i < 9; ++i)
    {
        m_generals_hp[i] = 0;
    }
    _cid = 0;    //角色id
    _wins = 0;   //胜利次数
    _skips = 0;  //跳过次数
    _lose_round = 0;   //在拿轮被击败
}

void corpsFihtingMember::setDie(int round)
{
    for (int i = 0; i < 9; ++i)
    {
        m_generals_hp[i] = -1;
    }
    m_hp_percent = 0;
    _lose_round = round;
}

void corpsFightingCorpsRound::add(boost::shared_ptr<corpsFightingResult> a)
{
    m_reports.push_back(a);
}

void corpsFihtingMember::setHp(Army& amy)
{
    int total_left = 0, total_max = 0;
    //更新剩余血量
    for (int i = 1; i <= 9; ++i)
    {
        iGeneral* g = amy.GetGeneral(i);
        if (g)
        {
            total_max += g->MaxHp();
            int cur_hp = g->Hp();
            if (cur_hp)
            {
                m_generals_hp[i-1] = g->MaxHp() - g->Hp();
                total_left += cur_hp;
            }
            else
            {
                m_generals_hp[i-1] = -1;
            }
        }
        else
        {
            m_generals_hp[i-1] = -1;
        }
    }

    if (total_max != 0)
    {
        m_hp_percent = total_left * 100 / total_max;
        if (total_left > 0 && m_hp_percent == 0)
        {
            m_hp_percent = 1;
        }
    }
}

void corpsFihtingMember::addRecord(int type, const std::string& name, int wins)
{
    json_spirit::Object rd;
    rd.push_back( Pair("type", type) );
    rd.push_back( Pair("name", name) );
    rd.push_back( Pair("wins", wins) );
    m_records.push_back(rd);
}

corpsFighting::corpsFighting()
{
    rank = 0;
    lose_round = 0;
    id = 0;
    lastround_alive = 0;
    name = "";
}

boost::shared_ptr<corpsFihtingMember> corpsFighting::getMatch()
{
    if (unmatch.size() > 0)
    {
        int r = my_random(0, unmatch.size()-1);
        boost::shared_ptr<corpsFihtingMember> ret = *(unmatch.begin() + r);
        unmatch.erase(unmatch.begin() + r);
        return ret;
    }
    boost::shared_ptr<corpsFihtingMember> nil;
    return nil;
}

void corpsFighting::addUnMatch(boost::shared_ptr<corpsFihtingMember> m)
{
    unmatch.push_back(m);
}

int corpsFighting::getUnMatchCount()
{
    return unmatch.size();
}

void corpsFighting::processSkips()
{
    //未匹配的队列轮空了
    for (std::vector<boost::shared_ptr<corpsFihtingMember> >::iterator it = unmatch.begin(); it != unmatch.end(); ++it)
    {
        ++(*it)->_skips;
        //加轮空战报
        boost::shared_ptr<corpsFightingResult> result(new corpsFightingResult);
        result->attacker = *it;
        result->result = 0;
        //result->defender.reset();
        m_cur_reports->add(result);

        result->result = 0;
        army_data ad;
        ad.LoadCorpsFighting((*it).get());        
        json_spirit::Object army_obj;
        army_obj.push_back( Pair("name", ad.m_name) );
        if (ad.m_type == 0)
        {
            army_obj.push_back( Pair("id", ad.m_charactor) );
        }
        else
        {
            army_obj.push_back( Pair("id", 0) );
        }
        army_obj.push_back( Pair("level", ad.m_level) );
        army_obj.push_back( Pair("spic", ad.m_spic) );
        army_obj.push_back( Pair("type", ad.m_type+1) );
        army_obj.push_back( Pair("corps", result->attacker->_corps.id) );
        army_obj.push_back( Pair("cname", result->attacker->_corps.name) );
        json_spirit::Array garray;
        for (size_t i = 0; i < 9; ++i)
        {
            if (ad.m_generals[i])
            {
                garray.push_back(ad.m_generals[i]->GetOrgObj());
            }
        }
        army_obj.push_back( Pair("generals", garray) );

        //生成 report字段
        result->report = "\"attacker\":" + json_spirit::write(army_obj);
        result->report += ",\"result\":0";

        //std::string msg = "jtz : " + name + "@" + ad.m_name + " bye";
        //GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        (*it)->addRecord(0, "", (*it)->_wins);
    }
}

void corpsFighting::removeAlive(int id)
{
    std::vector<boost::shared_ptr<corpsFihtingMember> >::iterator it = alives.begin();
    while (it != alives.end())
    {
        if ((*it)->_cid == id)
        {
            alives.erase(it);
            return;
        }
        ++it;
    }
}

corpsFightingMgr::corpsFightingMgr()
{
    m_state = 0;
    m_round_start = false;
    m_round = 0;
    m_next_time = 0;
    m_next_rank = 0;

    Query q(GetDb());
    q.get_result("select cid,corps,signupTime from char_corps_fighting where 1 order by corps,signupTime");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int cid = q.getval();
        int corps = q.getval();
        time_t stime = q.getval();
        signUp(corps, cid, stime);
    }
    q.free_result();

    q.get_result("select rank,itemType,itemId,count from base_jtz_awards where 1 order by rank,itemType");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int rank = q.getval();
        if (rank == (m_awards.size()+1))
        {
            JtzAwards awards;
            awards.rank = rank;
            m_awards.push_back(awards);
        }
        else if (rank != m_awards.size())
        {
            ERR();
            continue;
        }
        JtzAwards& awards = m_awards[rank-1];
        Item x;
        x.type = q.getval();
        x.id = q.getval();
        x.nums = q.getval();
        awards.awards.push_back(x);
    }
    q.free_result();

    _channel.reset(new ChatChannel("corpsFighting", 1, "{\"cmd\":\"chat\",\"ctype\":11,\"s\":200,"));
    _channel->start();
}

//开放报名
void corpsFightingMgr::openSignup()
{
    //cout<<"corpsFightingMgr::openSignup()"<<endl;
    m_state = 0;
    m_signup_data.clear();
    InsertSaveDb("truncate table char_corps_fighting");
}

void corpsFightingMgr::start()
{
    if (m_state != 0)
    {
        return;
    }
    
    _channel->Clear();
    m_char_map.clear();
    m_cur_round.reset();
    m_rounds_data.clear();

    m_round_state = 0;
    m_state = 1;
    m_round_start = false;
    m_corps_map.clear();
    m_alive_corps.clear();
    //哪些人报名了
    for (std::map<int, corpsFightingSignup>::iterator it = m_signup_data.begin();
         it != m_signup_data.end();
         ++it)
    {
        corpsFightingSignup& cs = it->second;
        corpsFighting* cf = new corpsFighting;
        cf->id = it->first;
        cf->name = corpsMgr::getInstance()->getCorpsName(it->first);
        for (std::map<int, time_t>::iterator its = cs.signups.begin(); its != cs.signups.end(); ++its)
        {
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(its->first);
            CharData* pc = cdata.get();
            //开战时还在本军团的才开打
            if (pc && pc->m_corps_member.get() && pc->m_corps_member->corps == it->first)
            {
                corpsFihtingMember* cfm = new corpsFihtingMember(*cf);
                cfm->_cid = its->first;
                cfm->_cdata = cdata;
                boost::shared_ptr<corpsFihtingMember> cfm_sp(cfm);
                CharZhens& zhens = cfm->_cdata->GetZhens();
                boost::shared_ptr<ZhenData> zdata = zhens.GetZhen(zhens.GetDefault());
                if (zdata.get())
                {
                    CharTotalGenerals& cg = cfm->_cdata->GetGenerals();
                    for (size_t i = 0; i < 9; ++i)
                    {
                        cfm->m_generals[i].reset();
                        if (zdata->m_generals[i] > 0)
                        {
                            cfm->m_generals[i] = cg.GetGenral(zdata->m_generals[i]);
                        }
                    }
                }
                cf->members.push_back(cfm_sp);
            }
        }
        if (cf->members.size())
        {
            boost::shared_ptr<corpsFighting> cf_sp(cf);
            m_corps_map[it->first] = cf_sp;
            m_alive_corps.push_back(cf_sp);
        }
        else
        {
            delete cf;
        }
    }

    for (std::list<boost::shared_ptr<corpsFighting> >::iterator it = m_alive_corps.begin(); it != m_alive_corps.end(); ++it)
    {
        //(*it).get()->alives = (*it).get()->members;
        for (std::list<boost::shared_ptr<corpsFihtingMember> >::iterator it2 = (*it).get()->members.begin();
             it2 != (*it).get()->members.end(); ++it2)
        {
            (*it).get()->alives.push_back(*it2);
        }
    }

    //是否还有下一回合
    if (m_alive_corps.size() == 1)
    {
        m_round = 1;
        onRoundEnd();
    }
    else if (m_alive_corps.size() == 0)
    {
        m_state = 2;
    }
    else
    {
        m_round = 0;
        m_next_rank = m_alive_corps.size();

        roundMatch();
    }
}

void corpsFightingMgr::roundMatch()
{
    if (m_state != 1)
    {
        return;
    }
    if (m_cur_round.get() != NULL)
    {
        m_round_start = true;
        return;
    }
    m_round_state = 0;
    m_round_start = false;
    m_cur_round.reset(new corpsFihtingRound);
    corpsFihtingRound& thisRound = *(m_cur_round.get());
    thisRound.round = ++m_round;
    thisRound.m_recv_results = 0;

    //std::string msg = "jtz : round " + LEX_CAST_STR(m_round);
    //GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);

    std::vector<boost::shared_ptr<corpsFihtingMember> > matchList;

    //记录下本回合活着的人数
    for (std::list<boost::shared_ptr<corpsFighting> >::iterator it = m_alive_corps.begin(); it != m_alive_corps.end(); ++it)
    {
        (*it).get()->lastround_alive = (*it).get()->alives.size();
        (*it).get()->unmatch = (*it).get()->alives;
        (*it).get()->m_cur_reports.reset(new corpsFightingCorpsRound);
        (*it).get()->m_cur_reports->round = m_round;
    }

    std::list<boost::shared_ptr<corpsFighting> > matchCorps = m_alive_corps;

    boost::shared_ptr<corpsFihtingMember> skipM;
    while (matchCorps.size() > 1)
    {
        matchList.clear();
        std::list<boost::shared_ptr<corpsFighting> >::iterator it = matchCorps.begin();
        while (it != matchCorps.end())
        {
            if (skipM.get() && skipM->_corps.id == (*it)->id)
            {
                ++it;
                continue;
            }
            boost::shared_ptr<corpsFihtingMember> m = (*it)->getMatch();
            if (m.get())
            {
                matchList.push_back(m);
                ++it;
            }
            else
            {
                it = matchCorps.erase(it);
            }
        }

        if (matchList.size() > 1 || (matchList.size() == 1 && skipM.get()))
        {
            std::random_shuffle(matchList.begin(), matchList.end());

            if (skipM.get())
            {
                matchList.insert(matchList.begin() + my_random(0,1), skipM);
                skipM.reset();
            }
            for (int i = 0; i < matchList.size()/2; ++i)
            {
                Combat* pb = createCombat(matchList[2*i].get(), matchList[2*i+1].get());
                if (pb)
                {
                    boost::shared_ptr<corpsFightingResult> result(new corpsFightingResult);
                    result->attacker = matchList[2*i];
                    result->defender = matchList[2*i+1];
                    result->result = 0;

                    thisRound.m_results[pb->combat_id()] = result;

                    InsertCombat(pb);
                }
            }
            if (matchList.size() % 2 == 1)
            {
                skipM = matchList[matchList.size()-1];
            }
        }
    }
    if (skipM.get())
    {
        skipM->_corps.addUnMatch(skipM);
        skipM.reset();
    }
    m_rounds_data.push_back(m_cur_round);

    //定时开启下一场
    json_spirit::mObject mobj;
    mobj["cmd"] = "JtzMatch";
    boost::shared_ptr<splsTimer> tmsg;
    if (m_round < 10)
    {
        tmsg.reset(new splsTimer(iRoundDelay, 1,mobj,1));
        m_next_time = time(NULL) + iRoundDelay;
    }
    else
    {
        tmsg.reset(new splsTimer(iRoundDelay2, 1,mobj,1));
        m_next_time = time(NULL) + iRoundDelay2;
    }
    splsTimerMgr::getInstance()->addTimer(tmsg);
}

int corpsFightingMgr::combatResult(Combat * pCombat)
{
    if (pCombat && m_cur_round.get())
    {
        //cout<<"corpsFightingMgr::combatResult, id:"<<pCombat->m_combat_id<<","<<pCombat->m_attacker->getCharId()<<" vs "<<pCombat->m_defender->getCharId()<<endl;
        corpsFihtingRound& thisRound = *(m_cur_round.get());
        std::map<int, boost::shared_ptr<corpsFightingResult> >::iterator it = thisRound.m_results.find(pCombat->combat_id());
        if (it != thisRound.m_results.end())
        {
            corpsFightingResult& result = *(it->second.get());
            if (result.result == 0)
            {
                ++thisRound.m_recv_results;
                result.result = pCombat->state();

                //生成 report字段
                json_spirit::Object aobj;
                pCombat->m_attacker->GetObj(aobj, true);
                aobj.push_back( Pair("corps", result.attacker->_corps.id) );
                aobj.push_back( Pair("cname", result.attacker->_corps.name) );                
                result.report = "\"attacker\":" + json_spirit::write(aobj);
                json_spirit::Object dobj;
                pCombat->m_defender->GetObj(dobj, true);
                dobj.push_back( Pair("corps", result.defender->_corps.id) );
                dobj.push_back( Pair("cname", result.defender->_corps.name) );
                result.report += ",\"defender\":" + json_spirit::write(dobj);
                if (pCombat->m_result_text == "")
                {
                    pCombat->m_result_text = json_spirit::write(pCombat->m_result_array);
                }
                result.report += ",\"cmdlist\":" + pCombat->m_result_text;
                result.report += ",\"result\":" + LEX_CAST_STR(pCombat->m_state);
                corpsFihtingMember* winner = NULL, *loser = NULL;                
                if (result.result == attacker_win)
                {
                    winner = result.attacker.get();
                    loser = result.defender.get();
                    winner->setHp(*(pCombat->m_attacker));
                }
                else
                {
                    loser = result.attacker.get();
                    winner = result.defender.get();
                    winner->setHp(*(pCombat->m_defender));
                }
                ++winner->_wins;
                loser->_lose_round = m_round;
                winner->_corps.m_cur_reports->add(it->second);
                loser->_corps.m_cur_reports->add(it->second);

                loser->setDie(m_round);
                loser->_corps.removeAlive(loser->_cid);

                winner->addRecord(1, loser->_cdata->m_name, winner->_wins);
                loser->addRecord(2, winner->_cdata->m_name, winner->_wins);

                InsertSaveCombat(pCombat);

                //std::string msg = "jtz : " + winner->_corps.name + "@" + winner->_cdata->m_name + " defeat " + loser->_corps.name + "@" + loser->_cdata->m_name;
                //GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
                if (thisRound.m_recv_results == thisRound.m_results.size())
                {
                    onRoundEnd();
                }
            }
        }
    }
    return HC_SUCCESS;
}

int corpsFightingMgr::signUp(int corps, int cid, time_t signup)
{
    if (m_state != 0)
    {
        return HC_ERROR;
    }

    time_t signupTime = signup;
    if (signupTime == 0)
    {
        signupTime = time(NULL);
    }
    bool insert_db = false;
    std::map<int, corpsFightingSignup>::iterator it = m_signup_data.find(corps);
    if (it != m_signup_data.end())
    {
        corpsFightingSignup& sd = it->second;
        if (sd.signups[cid] == 0)
        {
            sd.signups[cid] = signupTime;
            insert_db = true;
        }
    }
    else
    {
        corpsFightingSignup sd;
        sd.corps = corps;
        sd.signups[cid] = signupTime;
        m_signup_data[corps] = sd;
        insert_db = true;
    }

    if (insert_db && signup == 0)
    {
        InsertSaveDb("insert into char_corps_fighting (cid,corps,signupTime) values ("
            + LEX_CAST_STR(cid) + ","
            + LEX_CAST_STR(corps) + ",unix_timestamp())");
    }
    return HC_SUCCESS;
}


//取消报名
int corpsFightingMgr::cancelSignUp(int corps, int cid)
{
    if (m_state != 0)
    {
        return HC_ERROR;
    }    
    std::map<int, corpsFightingSignup>::iterator it = m_signup_data.find(corps);
    if (it != m_signup_data.end())
    {
        corpsFightingSignup& sd = it->second;
        if (sd.signups.find(cid) != sd.signups.end())
        {
            sd.signups.erase(cid);
            InsertSaveDb("delete from char_corps_fighting where cid=" + LEX_CAST_STR(cid) + " and corps=" + LEX_CAST_STR(corps));
        }
    }
    return HC_SUCCESS;
}

int corpsFightingMgr::getSignupState(int corps, int cid)
{
    if (m_state == 2)
    {
        return CORPS_FIGHTING_SIGNUP_STATE_END;
    }
    std::map<int, corpsFightingSignup>::iterator it = m_signup_data.find(corps);
    if (it != m_signup_data.end())
    {
        corpsFightingSignup& sd = it->second;
        if (sd.signups.find(cid) != sd.signups.end() && sd.signups[cid] > 0)
        {
            switch (m_state)
            {
                case 0:
                    return CORPS_FIGHTING_SIGNUP_STATE_SIGNED;
                case 1:
                    return CORPS_FIGHTING_SIGNUP_STATE_SIGNED_START;
                default:
                    return CORPS_FIGHTING_SIGNUP_STATE_END;
            }
        }
        else
        {
            switch (m_state)
            {
                case 0:
                    return CORPS_FIGHTING_SIGNUP_STATE_NOT_SIGN;
                case 1:
                    return CORPS_FIGHTING_SIGNUP_STATE_NOT_SIGNED_START;
                default:
                    return CORPS_FIGHTING_SIGNUP_STATE_END;
            }
        }
    }
    else
    {
        switch (m_state)
        {
            case 0:
                return CORPS_FIGHTING_SIGNUP_STATE_NOT_SIGN;
            case 1:
                return CORPS_FIGHTING_SIGNUP_STATE_NOT_SIGNED_START;
            default:
                return CORPS_FIGHTING_SIGNUP_STATE_END;
        }
    }
}

void corpsFightingMgr::setCorpsFightingRank(corpsFighting& cf, int rank)
{
    cf.rank = rank;
    //广播，给奖励
    std::string msg = strCorpsFightingRankMsg;
    str_replace(msg, "$C", cf.name);
    str_replace(msg, "$R", LEX_CAST_STR(cf.rank));

    JtzAwards* awards = NULL;
    if (cf.rank > m_awards.size())
    {
        awards = &(m_awards[m_awards.size()-1]);
    }
    else
    {
        awards = &(m_awards[cf.rank-1]);
    }

    std::list<Item> awards_item = awards->awards;
    std::string str_awards = "";
    for (std::list<Item>::iterator it = awards_item.begin(); it != awards_item.end(); ++it)
    {
        corpsRealReward(it->nums);
        if (str_awards != "")
        {
            str_awards += ",";
        }
        str_awards += it->toString(false, 0);
    }
    str_replace(msg, "$W", str_awards);
    GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);

#if 0
    //军团全体有奖励
    splsCorps* cps = corpsMgr::getInstance()->findCorps(cf.id);
    if (cps)
    {
        for (std::list<boost::shared_ptr<corps_member> >::iterator it = cps->_members_list.begin();
             it != cps->_members_list.end(); ++it)
        {
            if ((*it).get())
            {
                Singleton<char_rewards_mgr>::Instance().updateCharRewards((*it)->cid, rewards_type_jtz,rank,awards->awards);
            }
        }
    }
#else
    //只有参加的人有奖励
    for (std::list<boost::shared_ptr<corpsFihtingMember> >::iterator it = cf.members.begin(); it != cf.members.end(); ++it)
    {
        Singleton<char_rewards_mgr>::Instance().updateCharRewards((*it)->_cid, rewards_type_jtz, rank, awards_item);
    }
#endif
}

void corpsFightingMgr::onRoundEnd()
{
    //cout<<"onRoundEnd..."<<m_round<<endl;
    if (m_round == 1)
    {
        //广播开放消息
        GeneralDataMgr::getInstance()->broadCastSysMsg(strCorpsFightingStartMsg, -1);
    }
    m_round_state = 1;
    std::list<boost::shared_ptr<corpsFighting> > die_corps;
    std::list<boost::shared_ptr<corpsFighting> >::iterator it = m_alive_corps.begin();
    while ( it != m_alive_corps.end() )
    {
        //处理轮空的情况
        (*it).get()->processSkips();
        //按血量排序
        (*it).get()->members.sort(compare_corpsfighting_member);

        (*it).get()->m_round_reports.push_back((*it).get()->m_cur_reports);

        //是否被淘汰
        if ((*it).get()->alives.size() == 0)
        {
            die_corps.push_back(*it);
            it = m_alive_corps.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //排名次
    if (die_corps.size() > 0)
    {
        die_corps.sort(compare_corps_fighting);
        for (std::list<boost::shared_ptr<corpsFighting> >::iterator it = die_corps.begin(); it != die_corps.end(); ++it)
        {
            corpsFighting& cps = *(*it).get();
            //cps.rank = m_next_rank;
            cps.lose_round = m_round;

            //广播，给奖励
            setCorpsFightingRank(cps, m_next_rank);
            --m_next_rank;
        }
    }
    m_cur_round.reset();
    
    //是否还有下一回合
    if (m_alive_corps.size() == 1)
    {
        //结束了
        m_state = 2;
        boost::shared_ptr<corpsFighting> champion = *(m_alive_corps.begin());
        
        //广播冠军，给奖励
        setCorpsFightingRank(*(champion.get()), 1);

        //广播军团战结束了
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "jtzRound") );
        obj.push_back( Pair("s", 200) );
        obj.push_back( Pair("round", m_round) );
        obj.push_back( Pair("end", 1) );
        _channel->BroadMsg(json_spirit::write(obj));
    }
    else
    {
        //广播回合结束
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "jtzRound") );
        obj.push_back( Pair("s", 200) );
        obj.push_back( Pair("round", m_round) );
        _channel->BroadMsg(json_spirit::write(obj));
        
        if (m_round_start)
        {
            roundMatch();
        }
    }
}

Combat* corpsFightingMgr::createCombat(corpsFihtingMember* c1, corpsFihtingMember* c2)
{
    //被攻击方
    army_data* pArmy_data_d = new army_data;    
    Army* pDefender = new Army(pArmy_data_d);

    //攻击方
    army_data* pArmy_data_a = new army_data;    
    Army* pAttacker = new Army(pArmy_data_a);

    //战斗
    Combat* pCombat = new Combat(pAttacker, pDefender);

    pCombat->m_type = combat_corps_fighting;
    pCombat->m_type_id = 0;

    if (0 != pArmy_data_d->LoadCorpsFighting(c2))
    {
        return NULL;
    }
    if (0 != pArmy_data_a->LoadCorpsFighting(c1))
    {
        return NULL;
    }

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    pAttacker->clearBuff();
    pDefender->clearBuff();

    //战斗双方数据信息
    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );

    return pCombat;
}

int corpsFightingMgr::getJtzInfo(boost::shared_ptr<OnlineCharactor>& ou, int corps, int cid, json_spirit::Object& robj)
{
    //还没开始
    if (m_state == 0)
    {
        return HC_ERROR;
    }
    std::map<int, boost::shared_ptr<corpsFighting> >::iterator it = m_corps_map.find(corps);
    if (it != m_corps_map.end())
    {
        corpsFighting& cf = *(it->second.get());
        if (cf.rank > 0 && cf.lose_round != m_round)
        {
            robj.push_back( Pair("rank", cf.rank) );
            return HC_ERROR_CORPS_FIGHTING_LOSE;
        }
        else
        {
            if (cf.rank > 0)
            {
                robj.push_back( Pair("rank", cf.rank) );
            }
            robj.push_back( Pair("round", m_round) );
            robj.push_back( Pair("secs", m_next_time - time(NULL)) );
            if (cf.m_cur_reports.get())
            {
                robj.push_back( Pair("totalFights", cf.m_cur_reports.get()->m_reports.size()) );
                json_spirit::Array list;
                std::list<boost::shared_ptr<corpsFihtingMember> >::iterator it = cf.members.begin();
                while (it != cf.members.end())
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("name", (*it)->_cdata->m_name) );
                    obj.push_back( Pair("wins", (*it)->_wins) );
                    obj.push_back( Pair("hp", (*it)->m_hp_percent) );
                    list.push_back(obj);
                    ++it;
                }
                robj.push_back( Pair("list", list) );
            }
            if (m_char_map[cid] == 0)
            {
                _channel->Add(ou);
            }
            m_char_map[cid] = corps;
            return HC_SUCCESS;
        }
    }
    else
    {
        return HC_ERROR_CORPS_FIGHTING_NOT_JOIN;
    }
}

int corpsFightingMgr::getMyJtz(int corps, int cid, json_spirit::Object& robj)
{
    //还没开始
    if (m_state == 0)
    {
        return HC_ERROR;
    }
    std::map<int, boost::shared_ptr<corpsFighting> >::iterator it = m_corps_map.find(corps);
    if (it != m_corps_map.end())
    {
        corpsFighting& cf = *(it->second.get());
        {
            std::list<boost::shared_ptr<corpsFihtingMember> >::iterator itm = cf.members.begin();
            while (itm != cf.members.end())
            {
                if ((*itm)->_cid == cid)
                {
                    robj.push_back( Pair("name", (*itm)->_cdata->m_name) );
                    robj.push_back( Pair("list", (*itm)->m_records) );
                    return HC_SUCCESS;
                }
                ++itm;
            }
            return HC_ERROR;
        }
    }
    else
    {
        return HC_ERROR_CORPS_FIGHTING_NOT_JOIN;
    }
}

int corpsFightingMgr::getJtzCombat(net::session_ptr& sp, int corps, int round, int r2, json_spirit::Object& robj)
{
    //还没开始
    if (m_state == 0)
    {
        return HC_ERROR;
    }
    std::map<int, boost::shared_ptr<corpsFighting> >::iterator it = m_corps_map.find(corps);
    if (it != m_corps_map.end())
    {
        corpsFighting& cf = *(it->second.get());
        //回合还未打完
        if (cf.rank == 0 && m_round_state == 0)
        {
            return HC_ERROR;
        }
        else
        {
            corpsFightingCorpsRound* pr = NULL;
            if (round < 1 || round > cf.m_round_reports.size())
            {
                pr = cf.m_cur_reports.get();
            }
            else
            {
                pr = cf.m_round_reports[round-1].get();
            }
            if (r2 < 1 || r2 > pr->m_reports.size())
            {
                return HC_ERROR;
            }

            std::string msg = "{\"cmd\":\"queryJtzCombat\",\"s\":200,\"round\":" + LEX_CAST_STR(pr->round) + ",\"r2\":" + LEX_CAST_STR(r2);
            msg += "," + pr->m_reports[r2-1].get()->report + "}";
            sp->send(msg);
            return HC_SUCCESS_NO_RET;
        }
    }
    else
    {
        return HC_ERROR_CORPS_FIGHTING_NOT_JOIN;
    }
}

int corpsFightingMgr::leaveJtz(int cid)
{
    m_char_map.erase(cid);
    _channel->Remove(cid);
    return HC_SUCCESS;
}

//报名参加军团战
int ProcessSignupJtz(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
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
    if (cp->_level < corpsMgr::getInstance()->getCorpsActionLevel(corps_action_fighting))
    {
        return HC_ERROR;
    }
    return Singleton<corpsFightingMgr>::Instance().signUp(corps, pc->m_id);
}

//查询军团战信息
int ProcessQueryJtzInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    boost::shared_ptr<OnlineUser> account = psession->user();
    robj.push_back( Pair("id", pc->m_corps_member->corps) );
    return Singleton<corpsFightingMgr>::Instance().getJtzInfo(account->m_onlineCharactor, pc->m_corps_member->corps, pc->m_id, robj);
}

//查询军团战战报
int ProcessQueryJtzCombat(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int round = 0, r2 = 1;
    READ_INT_FROM_MOBJ(round,o,"round");
    READ_INT_FROM_MOBJ(r2,o,"r2");    
    return Singleton<corpsFightingMgr>::Instance().getJtzCombat(psession, pc->m_corps_member->corps, round, r2, robj);
}

//查询军团战我的战报
int ProcessQueryMyJtz(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }    
    return Singleton<corpsFightingMgr>::Instance().getMyJtz(pc->m_corps_member->corps, pc->m_id, robj);
}

//离开军团战界面
int ProcessLeaveJtz(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<corpsFightingMgr>::Instance().leaveJtz(pc->m_id);
}

//下一轮匹配
int ProcessJtzMatch(json_spirit::mObject& o)
{
    Singleton<corpsFightingMgr>::Instance().roundMatch();
    return HC_SUCCESS;
}

//军团战开放报名
int ProcessJtzOpenSignup(json_spirit::mObject& o)
{
    Singleton<corpsFightingMgr>::Instance().openSignup();
    return HC_SUCCESS;
}

//开启军团战
int ProcessStartJtz(json_spirit::mObject& o)
{
    Singleton<corpsFightingMgr>::Instance().start();
    return HC_SUCCESS;
}

