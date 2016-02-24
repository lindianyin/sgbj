
#include "libao.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "errcode_def.h"
#include "utils_all.h"
#include "singleton.h"
#include "new_combat.hpp"
#include "net.h"

extern Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
int getSessionChar(net::session_ptr& psession, boost::shared_ptr<CharData>& cdata);

extern std::string strLibaoMsg;

#ifdef QQ_PLAT
//查询黄钻界面：cmd:getYellowEvent
int ProcessQueryQQYellowEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    robj.push_back( Pair("vip_is_year_yellow", cdata->m_qq_yellow_year) );
    robj.push_back( Pair("vip_yellow_level", cdata->m_qq_yellow_level) );

    robj.push_back( Pair("heroGet", cdata->queryExtraData(char_data_type_normal, char_data_normal_qq_yellow_special) ) );
    robj.push_back( Pair("newbieGet", cdata->queryExtraData(char_data_type_normal, char_data_normal_qq_yellow_newbie) ) );
    robj.push_back( Pair("levelGet", libaoMgr::getInstance()->allGetQQLevelLibao(cdata.get()) ? 1 : 0));
    return ret;
}

//查询黄钻新手礼包 cmd：queryQQnewbieLibao
int ProcessQueryQQNewbieLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    robj.push_back( Pair("vip_is_year_yellow", cdata->m_qq_yellow_year) );
    robj.push_back( Pair("vip_yellow_level", cdata->m_qq_yellow_level) );

    if (cdata->m_qq_yellow_level > 0)
    {
        int get = cdata->queryExtraData(char_data_type_normal, char_data_normal_qq_yellow_newbie);
        robj.push_back( Pair("get", get));
    }
    baseLibao* lb = libaoMgr::getInstance()->getQQNewbieLibao();
    if (lb)
    {
        robj.push_back( Pair("list", lb->getArray()) );
    }
    return ret;
}

//查询黄钻每日礼包 cmd：queryQQDailyLibao
int ProcessQueryQQDailyLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    robj.push_back( Pair("vip_is_year_yellow", cdata->m_qq_yellow_year) );
    robj.push_back( Pair("vip_yellow_level", cdata->m_qq_yellow_level) );

    robj.push_back( Pair("get", cdata->queryExtraData(char_data_type_daily, char_data_daily_qq_yellow_libao)));
    robj.push_back( Pair("yearGet", cdata->queryExtraData(char_data_type_daily, char_data_daily_qq_year_yellow_libao)));
    robj.push_back( Pair("list", libaoMgr::getInstance()->getQQDailyList()) );
    baseLibao* lb = libaoMgr::getInstance()->getQQYearDailyLibao();
    if (lb)
    {
        robj.push_back( Pair("list2", lb->getArray()) );
    }
    return ret;
}

//查询黄钻成长礼包 cmd：queryQQLevelLibao
int ProcessQueryQQLevelLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    robj.push_back( Pair("vip_is_year_yellow", cdata->m_qq_yellow_year) );
    robj.push_back( Pair("vip_yellow_level", cdata->m_qq_yellow_level) );

    json_spirit::Array list;
    libaoMgr::getInstance()->getCharQQLevelLibao(*(cdata.get()), list);
    robj.push_back( Pair("list", list) );
    return ret;
}

#endif

const json_spirit::Object& baseLibao::getObj() const
{
    return m_obj;
}

void baseLibao::getObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", m_libao_id) );
    obj.push_back( Pair("libao_type", m_type) );
    obj.push_back( Pair("need_extra", m_need_extra) );
    obj.push_back( Pair("name", m_name) );
    obj.push_back( Pair("memo", m_memo) );
    obj.push_back( Pair("spic", m_spic) );
    obj.push_back( Pair("quality", m_quality) );
    return;
}

const json_spirit::Array& baseLibao::getArray() const
{
    return m_item_list;
}

void baseLibao::getArray(CharData& cdata, json_spirit::Array& list)
{
    for (std::list<Item>::iterator it_i = m_list.begin(); it_i != m_list.end(); ++it_i)
    {
        //cout << "updateObj libao_id=" << m_libao_id << ",item_type=" << it_i->type << ",item_id=" << it_i->id << endl;
        boost::shared_ptr<json_spirit::Object> p_obj;
        Item item = *it_i;
        item.toPointObj(p_obj);
        if (p_obj.get())
        {
            list.push_back(*(p_obj.get()));
        }
    }
}

void baseLibao::updateObj()
{
    m_obj.clear();
    m_obj.push_back( Pair("id", m_libao_id) );
    m_obj.push_back( Pair("libao_type", m_type) );
    m_obj.push_back( Pair("need_extra", m_need_extra) );
    m_obj.push_back( Pair("name", m_name) );
    m_obj.push_back( Pair("memo", m_memo) );
    m_obj.push_back( Pair("spic", m_spic) );
    m_obj.push_back( Pair("quality", m_quality) );
    json_spirit::Array getlist;
    m_item_list.clear();

    need_slot_num = 0;
    for (std::list<Item>::iterator it_i = m_list.begin(); it_i != m_list.end(); ++it_i)
    {
        //cout << "updateObj libao_id=" << m_libao_id << ",item_type=" << it_i->type << ",item_id=" << it_i->id << endl;
        boost::shared_ptr<json_spirit::Object> p_obj;
        (*it_i).toPointObj(p_obj);
        if (p_obj.get())
        {
            getlist.push_back(*(p_obj.get()));
        }
        json_spirit::Object obj;
        it_i->toObj(obj);
        m_item_list.push_back(obj);

        if (it_i->type == ITEM_TYPE_EQUIPMENT)
        {
            need_slot_num += it_i->nums;
        }
        else if(it_i->type == ITEM_TYPE_LIBAO)
        {
            need_slot_num += it_i->nums;
        }
        else if(it_i->type == ITEM_TYPE_GEM)
        {
            boost::shared_ptr<baseGem> tr = GeneralDataMgr::getInstance()->GetBaseGem(it_i->id);
            if (tr.get() && !tr->currency)
            {
                int32_t max_c = tr->max_size;
                if (max_c > 1)
                {
                    int need = it_i->nums / max_c;
                    if (it_i->nums % max_c > 0)
                        ++need;
                    need_slot_num += need;
                }
                else
                {
                    ++need_slot_num;
                }
            }
        }
    }
    m_obj.push_back( Pair("get", getlist) );
}

libao::libao(int id, baseLibao& base)
:item_base(ITEM_TYPE_LIBAO, base.m_libao_id, id, 1)
,m_base(base)
{
    m_id = id;
}

std::string libao::name() const
{
    return m_base.m_name;
}

std::string libao::memo() const
{
    return m_base.m_memo;
}

uint16_t libao::getSpic() const
{
    return m_base.m_spic;
}

int libao::getQuality() const
{
    return m_base.m_quality;
}

void libao::Save()
{
    if (m_changed)
    {
        int32_t c = getCount();
        if (c > 0)
        {
            CharData* pc = getChar();
            InsertSaveDb("replace into char_libao (id,libao_id,cid,slot) values ("
                +LEX_CAST_STR(getId())+","
                +LEX_CAST_STR(m_base.m_libao_id)+","
                +LEX_CAST_STR(pc!=NULL ? pc->m_id : 0)+","
                +LEX_CAST_STR((int)(getSlot()))+")");
        }
        else
        {
            InsertSaveDb("delete from char_libao where id=" + LEX_CAST_STR(getId()));
        }
        m_changed = false;
    }
}

libaoMgr* libaoMgr::m_handle = NULL;

libaoMgr* libaoMgr::getInstance()
{
    if (NULL == m_handle)
    {
        m_handle = new libaoMgr();
        m_handle->load();
    }
    return m_handle;
}

void libaoMgr::load()
{
    Query q(GetDb());
    q.get_result("select id,type,need_extra,name,memo,spic,quality,notify_msg from base_libao where 1");
    while (q.fetch_row())
    {
        int libao_id = q.getval();
        int libao_type = q.getval();
        baseLibao*p = new baseLibao;
        p->m_libao_id = libao_id;
        p->m_type = libao_type;
        p->m_need_extra = q.getval();
        p->m_name = q.getstr();
        p->m_memo = q.getstr();
        p->m_spic = q.getval();
        p->m_quality = q.getval();
        p->m_notify_msg = q.getval();
        p->need_slot_num = 0;
        m_base_libaos[p->m_libao_id] = p;
        //等级礼包
        if (p->m_type == libao_type_level)
            m_chengzhang_Libaos.push_back(p->m_libao_id);
    }
    q.free_result();

    q.get_result("select libao_id,itemType,itemId,counts,extra from base_libao_reward where 1 order by libao_id");
    while (q.fetch_row())
    {
        int libao_id = q.getval();
        baseLibao* p = m_base_libaos[libao_id];
        if (p != NULL)
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.nums = q.getval();
            it.extra = q.getval();
            p->m_list.push_back(it);
        }
        //cout << "push_into libao_id=" << libao_id << " now_size=" << p->m_list.size() << endl;
    }
    q.free_result();
    for (std::map<int, baseLibao* >::iterator it = m_base_libaos.begin(); it != m_base_libaos.end(); ++it)
    {
        it->second->updateObj();
    }
    m_libao_id = 0;
    q.get_result("select max(id) from char_libao where 1");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_libao_id = q.getval();
    }
    q.free_result();
#ifdef QQ_PLAT
    //黄钻等级礼包
    q.get_result("select level,itemType,itemId,counts,extra from base_qq_level_libao where 1 order by level,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        baseLibao* lb = getQQLevelLibao(level);
        if (NULL == lb)
        {
            boost::shared_ptr<baseLibao> splb(new baseLibao);
            m_qq_levelLibaos[level] = splb;
            lb = splb.get();
            lb->m_libao_id = 0;
            lb->m_type = 0;
            lb->m_need_extra = level;
            lb->m_name = "qq yellow level";
            lb->m_memo = "";
            lb->m_spic = 0;
            lb->m_quality = 0;
            lb->need_slot_num = 0;
        }
        if (lb)
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.nums = q.getval();
            it.extra = q.getval();
            lb->m_list.push_back(it);
        }
    }
    q.free_result();

    //黄钻新手礼包
    q.get_result("select itemType,itemId,counts,extra from base_qq_newbie_libao where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        //int level = q.getval();
        baseLibao* lb = getQQNewbieLibao();
        if (NULL == lb)
        {
            boost::shared_ptr<baseLibao> splb(new baseLibao);
            m_qq_newbie_libao = splb;
            lb = splb.get();
            lb->m_libao_id = 0;
            lb->m_type = 0;
            lb->m_need_extra = 1;
            lb->m_name = "qq yellow newbie";
            lb->m_memo = "";
            lb->m_spic = 0;
            lb->m_quality = 0;
            lb->need_slot_num = 0;
        }
        if (lb)
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.nums = q.getval();
            it.extra = q.getval();
            lb->m_list.push_back(it);
        }
    }
    q.free_result();

    //黄钻专属礼包
    q.get_result("select level,itemType,itemId,counts,extra from base_qq_yellow_hero where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        baseLibao* lb = getQQSpecialLibao();
        if (NULL == lb)
        {
            boost::shared_ptr<baseLibao> splb(new baseLibao);
            m_qq_special_libao = splb;
            lb = splb.get();
            lb->m_libao_id = 0;
            lb->m_type = 0;
            lb->m_need_extra = level;
            lb->m_name = "qq yellow special";
            lb->m_memo = "";
            lb->m_spic = 0;
            lb->m_quality = 0;
            lb->need_slot_num = 0;
        }
        else
        {
            if (level > lb->m_need_extra)
            {
                lb->m_need_extra = level;
            }
        }
        if (lb)
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.nums = q.getval();
            it.extra = q.getval();
            lb->m_list.push_back(it);
        }
    }
    q.free_result();

    //黄钻每日礼包
    q.get_result("select yellow_level,itemType,itemId,counts,extra from base_qq_yellow_daily_libao where 1 order by yellow_level,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        //cout<<"qq yellow daily libao level "<<level<<endl;
        baseLibao* lb = NULL;
        if (level == 0)
        {
            lb = getQQYearDailyLibao();
            if (NULL == lb)
            {
                //cout<<"add qq year daily libao"<<endl;
                boost::shared_ptr<baseLibao> splb(new baseLibao);
                m_qq_year_daily_libao = splb;
                lb = splb.get();
                lb->m_libao_id = 0;
                lb->m_type = 0;
                lb->m_need_extra = level;
                lb->m_name = "qq year yellow daily";
                lb->m_memo = "";
                lb->m_spic = 0;
                lb->m_quality = 0;
                lb->need_slot_num = 0;
            }
        }
        else
        {
            lb = getQQDailyLibao(level);
            if (NULL == lb)
            {
                //cout<<"add qq daily libao"<<endl;
                boost::shared_ptr<baseLibao> splb(new baseLibao);
                m_qq_daily_libao[level] = splb;
                lb = splb.get();
                lb->m_libao_id = 0;
                lb->m_type = 0;
                lb->m_need_extra = level;
                lb->m_name = "qq yellow daily";
                lb->m_memo = "";
                lb->m_spic = 0;
                lb->m_quality = 0;
                lb->need_slot_num = 0;
            }
        }

        if (lb)
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.nums = q.getval();
            it.extra = q.getval();
            lb->m_list.push_back(it);
            //cout<<"add qq year daily libao, add item "<<it.type<<","<<it.id<<","<<it.nums<<endl;
        }
    }
    q.free_result();

    if (m_qq_newbie_libao.get())
    {
        m_qq_newbie_libao->updateObj();
    }
    if (m_qq_special_libao.get())
    {
        m_qq_special_libao->updateObj();
    }
    if (m_qq_year_daily_libao.get())
    {
        m_qq_year_daily_libao->updateObj();
    }
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_qq_levelLibaos.begin(); it != m_qq_levelLibaos.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->updateObj();
        }
    }
    m_qq_daily_list.clear();
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_qq_daily_libao.begin(); it != m_qq_daily_libao.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->updateObj();
            //cout<<"update obj ->"<<it->second->m_name<<",level:"<<it->second->m_level<<endl;
            //cout<<it->second->m_list.size()<<","<<it->second->m_item_list.size()<<endl;
            json_spirit::Object obj;
            obj.push_back( Pair("level", it->first) );
            obj.push_back( Pair("list", it->second->getArray()) );
            m_qq_daily_list.push_back(obj);
        }
    }
#endif
}

baseLibao* libaoMgr::getBaselibao(int id)
{
    return m_base_libaos[id];
}

std::string libaoMgr::getlibaoMemo(int sid)
{
    std::string memo = "";
    baseLibao* p = NULL;
    p = getBaselibao(sid);
    if (p == NULL)
    {
        return memo;
    }
    return p->m_memo;
}

//增加礼包
int libaoMgr::addLibao(CharData* pc, int libao_id)
{
    if (!pc)
    {
        return HC_ERROR;
    }
    if (pc->m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }
    boost::shared_ptr<item_base> bs;
    baseLibao* pl = getBaselibao(libao_id);
    if (!pl)
    {
         return HC_ERROR;
    }
    int id = newLibaoId();
    libao* pb = new libao(id, *pl);
    bs.reset(pb);
    if (!bs.get())
    {
        return HC_ERROR;
    }
    pc->m_bag.addItem(bs);
    pb->Save();
    //通知客户端
    json_spirit::Object obj;
    json_spirit::Object item;
    item.push_back( Pair("type", ITEM_TYPE_LIBAO) );
    item.push_back( Pair("spic", pl->m_spic) );
    obj.push_back( Pair("item", item) );
    obj.push_back( Pair("cmd", "notifyGet") );
    obj.push_back( Pair("s", 200) );
    pc->sendObj(obj);
    return HC_SUCCESS;
}

//打开礼包
int libaoMgr::openLibao(CharData* pc, int slot, json_spirit::Object& robj)
{
    if (!pc)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<item_base> itm = pc->m_bag.getItem(slot);
    if (!itm.get())
    {
        return HC_ERROR;
    }
    if (itm->getType() != ITEM_TYPE_LIBAO)
    {
        return HC_ERROR;
    }
    libao* pl = dynamic_cast<libao*>(itm.get());
    if (pl->m_base.m_type == libao_type_level)
    {
        if (pc->m_level < pl->m_base.m_need_extra)
        {
            return HC_ERROR_NEED_MORE_LEVEL;
        }
    }
    int need_slot_num = pl->m_base.need_slot_num;
    std::list<Item>::iterator it = pl->m_base.m_list.begin();
    while (it != pl->m_base.m_list.end())
    {
        if (it->type == ITEM_TYPE_GEM)
        {
            boost::shared_ptr<baseGem> tr = GeneralDataMgr::getInstance()->GetBaseGem(it->id);
            if (tr.get() && tr->currency == 0)
            {
                int32_t max_c = tr->max_size;
                if (max_c > 1 || 0 == max_c)
                {
                    for (size_t i = 0; i < pc->m_bag.getSize() && it->nums > 0; ++i)
                    {
                        boost::shared_ptr<item_base> p = pc->m_bag.getItem(i);
                        if (p.get() && p->getType() == ITEM_TYPE_GEM && p->getSubType() == it->id)
                        {
                            int32_t c = p->getCount();
                            if (0 == max_c || (c + it->nums) <= max_c)
                            {
                                --need_slot_num;
                                break;
                            }
                        }
                    }
                }
            }
        }
        ++it;
    }
    if (need_slot_num > 0 && !pc->m_bag.hasSlot(need_slot_num))
    {
        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
    }
    std::list<Item> items = pl->m_base.m_list;
    giveLoots(pc, items, NULL, &robj, true, loot_libao);
    if (pl->m_base.m_notify_msg)
    {
		std::string msg = strLibaoMsg;
		str_replace(msg, "$W", MakeCharNameLink(pc->m_name,pc->m_nick.get_string()));
		str_replace(msg, "$L", pl->name());
        str_replace(msg, "$R", itemlistToString(items));
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
    }
    pc->m_bag.removeItem(slot);
    itm->Clear();
    itm->Save();
    robj.push_back( Pair("refresh", 1) );
    return HC_SUCCESS;
}

//直接奖励礼包
int libaoMgr::rewardLibao(CharData* pc, int libao_id, json_spirit::Object& robj)
{
    baseLibao* p = NULL;
    p = getBaselibao(libao_id);
    if (p == NULL)
    {
        return HC_ERROR;
    }
    if (!pc->m_bag.hasSlot(p->need_slot_num))
    {
        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
    }
    //给东西
    std::list<Item> items = p->m_list;
    giveLoots(pc, items, NULL, &robj, true, loot_libao);
    return HC_SUCCESS;
}

int libaoMgr::getLibaoInfo(int libao_id, json_spirit::Object& robj)
{
    baseLibao* p = NULL;
    p = getBaselibao(libao_id);
    if (p == NULL)
    {
        return HC_ERROR;
    }
    robj.push_back( Pair("libaoVO", p->getObj()) );
    return HC_SUCCESS;
}

int libaoMgr::isChengzhangLibao(int id)
{
    for (int i = 0; i <= m_chengzhang_Libaos.size(); ++i)
    {
        if (m_chengzhang_Libaos[i] == id)
        {
            return i+1;
        }
    }
    return 0;
}

int libaoMgr::getChengzhangLibao(int pos)
{
    if (pos > 0 && pos <= m_chengzhang_Libaos.size())
    {
        return m_chengzhang_Libaos[pos-1];
    }
    else
    {
        return 0;
    }
}

#ifdef QQ_PLAT
baseLibao* libaoMgr::getQQLevelLibao(int level)
{
    if (m_qq_levelLibaos.find(level) != m_qq_levelLibaos.end())
    {
        return m_qq_levelLibaos[level].get();
    }
    else
    {
        return NULL;
    }
}

//是否已经领取全部qq成长礼包
bool libaoMgr::allGetQQLevelLibao(CharData* pc)
{
    if (!pc)
    {
        return false;
    }
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_qq_levelLibaos.begin(); it != m_qq_levelLibaos.end(); ++it)
    {
        if (pc->queryExtraData(char_data_type_normal, char_data_normal_qq_yellow_level_libao + it->first))
        {
        }
        else
        {
            return false;
        }
    }
    return true;
}

baseLibao* libaoMgr::getQQDailyLibao(int yellow_level)
{
    if (m_qq_daily_libao.find(yellow_level) != m_qq_daily_libao.end())
    {
        return m_qq_daily_libao[yellow_level].get();
    }
    else
    {
        return NULL;
    }
}

baseLibao* libaoMgr::getQQYearDailyLibao()
{
    return m_qq_year_daily_libao.get();
}

baseLibao* libaoMgr::getQQNewbieLibao()
{
    return m_qq_newbie_libao.get();
}

baseLibao* libaoMgr::getQQSpecialLibao()
{
    return m_qq_special_libao.get();
}

void libaoMgr::getCharQQLevelLibao(CharData& cdata, json_spirit::Array& list)
{
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_qq_levelLibaos.begin(); it != m_qq_levelLibaos.end(); ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            if (cdata.m_qq_yellow_level > 0 && cdata.m_level >= it->first)
            {
                int get = cdata.queryExtraData(char_data_type_normal, it->first + char_data_normal_qq_yellow_level_libao);
                if (get == 0)
                {
                    obj.push_back( Pair("canGet", 1) );
                }
                else
                {
                    continue;
                }
            }
            obj.push_back( Pair("level", it->first) );
            obj.push_back( Pair("list", it->second->getArray()) );

            list.push_back(obj);
        }
    }
}
#endif

