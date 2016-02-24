
/********* 数据统计 ************/

#include "statistics.h"
#include "utils_all.h"

#include "kingnet_analyzer.h"

#define ENABLE_STATISTICS

extern void InsertSaveDb(const std::string& sql);

//增加金币消耗统计
void add_statistics_of_gold_cost(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    InsertSaveDb("insert into admin_costgold (id,cid,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//增加银币消耗统计
void add_statistics_of_silver_cost(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    InsertSaveDb("insert into admin_costsill (id,cid,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//增加金币获得统计
void add_statistics_of_gold_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    InsertSaveDb("insert into admin_sougold (id,cid,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//增加银币获得统计
void add_statistics_of_silver_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    InsertSaveDb("insert into admin_sousill (id,cid,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//增加声望获得统计
void add_statistics_of_prestige_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    InsertSaveDb("insert into admin_souprestige (id,cid,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//军令统计stype:1获得2消耗
void add_statistics_of_ling_cost(int cid, const std::string& strIP, int counts, int type, int stype, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    InsertSaveDb("insert into admin_count_ling (id,cid,hnums,stime,ip,type,stype,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + "," + LEX_CAST_STR(stype) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//宝物统计stype:1获得2消耗
void add_statistics_of_treasure_cost(int cid, const std::string& strIP, int treasure_id, int counts, int type, int stype, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    InsertSaveDb("insert into admin_count_smost (id,cid,treasure_id,hnums,stime,ip,type,stype,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(treasure_id) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + "," + LEX_CAST_STR(stype) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

void add_statistics_of_baoshi_cost(int cid, const std::string& strIP, int union_id, const std::string& server_id, int baoshiType, int baoshiLevel, int baoshiCount, int type)
{
#ifdef ENABLE_STATISTICS
        InsertSaveDb("insert into admin_count_baoshi (id,cid,stype,btype,level,hnums,type,stime,ip,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ",0,"
            + LEX_CAST_STR(baoshiType) + ","
            + LEX_CAST_STR(baoshiLevel) + ","
            + LEX_CAST_STR(baoshiCount) + ","
            + LEX_CAST_STR(type) + ",now(),'" + strIP + "',"
            + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

void add_statistics_of_baoshi_get(int cid, const std::string& strIP, int union_id, const std::string& server_id, int baoshiType, int baoshiLevel, int baoshiCount, int type)
{
#ifdef ENABLE_STATISTICS
            InsertSaveDb("insert into admin_count_baoshi (id,cid,stype,btype,level,hnums,type,stime,ip,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ",1,"
                + LEX_CAST_STR(baoshiType) + ","
                + LEX_CAST_STR(baoshiLevel) + ","
                + LEX_CAST_STR(baoshiCount) + ","
                + LEX_CAST_STR(type) + ",now(),'" + strIP + "',"
                + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

#ifdef QQ_PLAT

//腾讯统计

//功能金币消耗
void cost_todo(CharData* pc, const std::string& gold_price, int type, int nums)
{
    if (pc == NULL)
        return;
    CUserLogger* logger = _analyzer.GetInstance(LEX_CAST_STR(pc->m_id).c_str());
    logger->SetUserIP(pc->m_ip_address.c_str());
    std::string user_msg = pc->getUserMsg();
    logger->SetUserMsg(user_msg.c_str());
    logger->LogMsg("add","1",LEX_CAST_STR(type).c_str(),gold_price.c_str(),"0","","props",nums);
    logger->LogMsg("sub","1",LEX_CAST_STR(type).c_str(),gold_price.c_str(),"0","","props",nums);
}

//道具金币消耗
void cost_tobuy(CharData* pc, const std::string& gold_price, int tid, int nums)
{
    if (pc == NULL)
        return;
    CUserLogger* logger = _analyzer.GetInstance(LEX_CAST_STR(pc->m_id).c_str());
    logger->SetUserIP(pc->m_ip_address.c_str());
    std::string user_msg = pc->getUserMsg();
    logger->SetUserMsg(user_msg.c_str());
    logger->LogMsg("add","1",LEX_CAST_STR(tid).c_str(),LEX_CAST_STR(gold_price).c_str(),"0","","props",nums);
}

//金币消耗道具获得
void gold_cost_tencent(CharData* pc, int gold_price, int type, int tid/*=0*/, int nums/*=1*/)
{
    if (pc == NULL)
        return;
    if (type == gold_cost_for_convert_jade || type == gold_cost_for_buy_daoju || type == gold_cost_for_buy_baoshi)//购买消费
    {
        if (tid != 0)
        {
            cost_tobuy(pc, LEX_CAST_STR(gold_price), tid, nums);
        }
    }
    else//功能消费
    {
        //功能消费类型+100000
        cost_todo(pc, LEX_CAST_STR(gold_price), type + 100000, nums);
    }
}

//金币消耗道具获得
void gold_cost_tencent(CharData* pc, double gold_price, int type, int tid/*=0*/, int nums/*=1*/)
{
    if (pc == NULL)
        return;
    if (type == gold_cost_for_convert_jade || type == gold_cost_for_buy_daoju || type == gold_cost_for_buy_baoshi)//购买消费
    {
        if (tid != 0)
        {
            cost_tobuy(pc, LEX_CAST_STR(gold_price), tid, nums);
        }
    }
    else//功能消费
    {
        //功能消费类型+100000
        cost_todo(pc, LEX_CAST_STR(gold_price), type + 100000, nums);
    }
}

//金币获得
void gold_get_tencent(CharData* pc, int gold, int type/*=1*/)
{
    if (pc == NULL)
        return;
    CUserLogger* logger = _analyzer.GetInstance(LEX_CAST_STR(pc->m_id).c_str());
    logger->SetUserIP(pc->m_ip_address.c_str());
    std::string user_msg = pc->getUserMsg();
    logger->SetUserMsg(user_msg.c_str());
    std::string sType = "";
    if (type == 1)
    {
        sType = "task";
    }
    else if(type == 2)
    {
        sType = "cc";
    }
    logger->LogMsg(sType.c_str(),LEX_CAST_STR(gold).c_str(),"","add","","","currency");
}

//道具消耗
void treasure_cost_tencent(CharData* pc, int tid, int nums)
{
    if (pc == NULL)
        return;
    CUserLogger* logger = _analyzer.GetInstance(LEX_CAST_STR(pc->m_id).c_str());
    logger->SetUserIP(pc->m_ip_address.c_str());
    std::string user_msg = pc->getUserMsg();
    logger->SetUserMsg(user_msg.c_str());
    boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(tid);
    if (!bt.get())
    {
        return;
    }
    if (tid == treasure_type_yushi)
    {
        logger->LogMsg("sub","1",LEX_CAST_STR(tid).c_str(),LEX_CAST_STR(0.007).c_str(),"0","","props",nums);
    }
    else
    {
        logger->LogMsg("sub","1",LEX_CAST_STR(tid).c_str(),LEX_CAST_STR(bt->gold_to_buy).c_str(),"0","","props",nums);
    }
}

//属性变化
void att_change_tencent(CharData* pc, const std::string& type, const std::string& before, const std::string& after)
{
    if (pc == NULL)
        return;
    CUserLogger* logger = _analyzer.GetInstance(LEX_CAST_STR(pc->m_id).c_str());
    logger->SetUserIP(pc->m_ip_address.c_str());
    std::string user_msg = pc->getUserMsg();
    logger->SetUserMsg(user_msg.c_str());
    logger->LogMsg(type.c_str(),before.c_str(),after.c_str(),"","","","att");
}

//背包记录
void bag_to_tencent(CharData* pc, const std::string& info)
{
    if (pc == NULL)
        return;
    CUserLogger* logger = _analyzer.GetInstance(LEX_CAST_STR(pc->m_id).c_str());
    logger->SetUserIP(pc->m_ip_address.c_str());
    std::string user_msg = pc->getUserMsg();
    logger->SetUserMsg(user_msg.c_str());
    logger->LogMsg(info.c_str(),"","","","","","inventory");
}

//服务器记录
void ser_to_tencent(const std::string& type, const std::string& stype, int nums)
{
    CUserLogger* logger = _analyzer.GetInstance("server");
    logger->SetUserIP("");
    logger->SetUserMsg("||||||||||||||||||");
    logger->LogMsg(type.c_str(),stype.c_str(),LEX_CAST_STR(time(NULL)).c_str(),LEX_CAST_STR(nums).c_str(),"","","ser");

    logger = _analyzer.GetInstance("server");

    _analyzer.DestoryInstance("server");
}

//引导记录
void guide_to_tencent(CharData* pc, int guide_id)
{
    if (pc == NULL)
        return;
    CUserLogger* logger = _analyzer.GetInstance(LEX_CAST_STR(pc->m_id).c_str());
    logger->SetUserIP(pc->m_ip_address.c_str());
    std::string user_msg = pc->getUserMsg();
    logger->SetUserMsg(user_msg.c_str());
    logger->LogMsg("guide",LEX_CAST_STR(guide_id).c_str(),"","","","","guide");
}

//登入记录
void login_to_tencent(CharData* pc, const std::string& str1, const std::string& str2, const std::string& friend_id, const std::string& feed_id)
{
    if (pc == NULL)
        return;
    CUserLogger* logger = _analyzer.GetInstance(LEX_CAST_STR(pc->m_id).c_str());
    logger->SetUserIP(pc->m_ip_address.c_str());
    std::string user_msg = pc->getUserMsg();
    logger->SetUserMsg(user_msg.c_str());
    logger->LogMsg(str1.c_str(),str2.c_str(),friend_id.c_str(), feed_id.c_str(),"","0","login");
}

#endif

//操作记录
void act_to_tencent(CharData* pc, int type, int param1, int param2, int param3, std::string param4)
{
    if (pc == NULL)
        return;
    CUserLogger* logger = _analyzer.GetInstance(LEX_CAST_STR(pc->m_id).c_str());
    logger->SetUserIP(pc->m_ip_address.c_str());
    std::string user_msg = pc->getUserMsg();
    logger->SetUserMsg(user_msg.c_str());
    logger->LogMsg(LEX_CAST_STR(type).c_str(),LEX_CAST_STR(param1).c_str(),LEX_CAST_STR(param2).c_str(),LEX_CAST_STR(param3).c_str(),param4.c_str(),"","act");
}

//腾讯操作统计
int ProcessActTencent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int type = 0, param1 = 0, param2 = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    READ_INT_FROM_MOBJ(param1, o, "param1");
    READ_INT_FROM_MOBJ(param2, o, "param2");
    act_to_tencent(cdata.get(), type, param1, param2);
    return HC_SUCCESS;
}

