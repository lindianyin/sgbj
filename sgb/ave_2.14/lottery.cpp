
#include "lottery.h"
#include "loot.h"
#include "utils_all.h"
#include "data.h"
#include "spls_errcode.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include <list>
#include "statistics.h"
#include "combat.h"
#include "daily_task.h"
#include "singleton.h"

using namespace std;
using namespace net;
using namespace json_spirit;

extern std::string strLotteryGetHeroMsg;
//�佫����
const std::string strLotteryGeneralLink = "<A HREF=\"event:{'purpose':4,'id':$G,'cmd':'showGeneral','cid':$C}\" TARGET=\"\"><U>$N</U></A>";

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

int getSessionChar(net::session_ptr& psession, CharData* &pc);

//÷��������ȡ��Ʒ
int ProcessLottery(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    int counts = 1;
    //�ȼ��Ƿ����
    if (pc->m_level < iLotteryOpenLevel)
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");

    //vip�ȼ�
    if (type == 3 && pc->m_vip < iLottery_50_vip_level)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    if (type == 2 && pc->m_vip < iLottery_10_vip_level)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    switch (type)
    {
        case 1:
            break;
        case 2:
            counts = 10;
            break;
        case 3:
            counts = 50;
            break;
    }
    //cout<<"lottery "<<counts<<endl;
    //�����Ƿ�
    int tr_counts = pc->treasureCount(treasure_type_yinyang_fish);
    if (tr_counts < counts)
    {
        //����Ƿ�
        int gold_need = (counts - tr_counts) * iLotteryCostGold;
        if (gold_need < 0 || pc->addGold(-gold_need) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //cout<<"cost gold "<<gold_need<<endl;
        //����ͳ��
        add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, gold_need, gold_cost_for_lottery, pc->m_union_id, pc->m_server_id);
        if (tr_counts)
        {
            //cout<<"cost treasure yinyang fish "<<tr_counts<<endl;
            pc->addTreasure(treasure_type_yinyang_fish, -tr_counts);
        }
    }
    else
    {
        //cout<<"cost treasure yinyang fish "<<counts<<endl;
        pc->addTreasure(treasure_type_yinyang_fish, -counts);
    }

    //�ܻ���
    int score = pc->queryExtraData(char_data_type_week, char_data_extra_lottery_score);
    //�ܻ���
    int total_score = pc->queryExtraData(char_data_type_normal, char_data_lottery_total_score);
    //�ܻ��ֺ����żӵģ������������⴦����
    if (total_score < score)
    {
        total_score = score;
    }
    std::list<Item> items;

    Item bestitem; //��õģ�������С
    bestitem.type = 0;
    bestitem.id = 0;
    bestitem.nums = 0;
    
    int single_score = 0;
    for (int i = 0; i < counts; ++i)
    {
        int _score = lootMgr::getInstance()->getLotteryItem(items);
        if (_score >= 0)
        {
            Item& item = *(items.rbegin());
            /*            �ܻ��ִﵽ���ٲ��п���
                ����     500
                �˽���    250
                ������    100
                ����ʯ    200
                �Ͼ�ʯ    300
            */
            int needScore = 0;
            if (item.type == item_type_general)
            {
                switch (item.id)
                {
                    case 53:    //����
                        needScore = 500;
                        break;
                    case 54:    //�˽���
                        needScore = 250;
                        break;
                    case 55:    //������
                        needScore = 100;
                        break;
                }
            }
            else if (item_type_baoshi == item.type)
            {
                switch (item.id)
                {
                    case 4:
                        //�Ͼ�ʯ    300
                        needScore = 300;
                        break;
                    case 3:
                        //����ʯ    200
                        needScore = 200;
                        break;
                }
            }

            //���ֲ����ģ�������� 1000*�����ȼ�
            if (score < needScore)
            {
                item.type = item_type_silver;
                item.nums = 20000;
                item.fac = 2;
                _score = 2;
            }

            score += _score;
            total_score += _score;
            
            //Ŀǰ��õĵ�����ʲô
            if (_score > single_score)
            {
                bestitem.id = item.id;
                bestitem.fac = item.fac;
                bestitem.type = item.type;
                bestitem.nums = item.nums;
                single_score = _score;
                //cout<<"get item type:"<<item.type<<",id:"<<item.id<<",nums:"<<item.nums<<",score:"<<_score<<endl;
            }
            //else
            //{
            //    Item& item = *(items.rbegin());
            //    cout<<"get item type:"<<item.type<<",id:"<<item.id<<",nums:"<<item.nums<<",score:"<<_score<<endl;
            //}
            //ȫ������
            if (_score >= 5)
            {
                Item& item = *(items.rbegin());
                Singleton<lotteryMgr>::instance()->addLotteryNotice("<font color=\"#00ff00\">" + pc->m_name + "</font>", item.toString(true));
                //lotteryMgr::getInstance()->addLotteryNotice("<font color=\"#00ff00\">" + pc->m_name + "</font>", item.toString(true));

                //����佫ȫ���㲥 (�����ֳ壬���ã��˽���)
                if (item.type == item_type_general)// && (item.id == 53 || item.id == 20 || item.id == 30))
                {
                    std::string msg = strLotteryGetHeroMsg;
                    str_replace(msg, "$W", MakeCharNameLink(pc->m_name));
                    boost::shared_ptr<GeneralTypeData> gr = GeneralDataMgr::getInstance()->GetBaseGeneral(item.id);
                    if (gr.get())
                    {
                        std::string link_name = Singleton<lotteryMgr>::instance()->NameToLink(pc->m_id,item.id,gr->m_name);
                        str_replace(msg, "$H", "<font color=\"#ffffff\">" + link_name + "</font>");
                        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
                    }
                }
            }
        }
    }
    //�����ܻ���
    pc->setExtraData(char_data_type_week, char_data_extra_lottery_score, score);
    //�����ܻ���
    pc->setExtraData(char_data_type_normal, char_data_lottery_total_score, total_score);

    robj.push_back( Pair("score", score) );

    if (items.size() > 0)
    {
        std::list<Item> getItems;        //�ϲ���Ļ��
        for (std::list<Item>::iterator it = items.begin(); it != items.end(); ++it)
        {
            Item& item = *it;
            if (item_type_silver_level == item.type)
            {
                item.type = item_type_silver;
                item.nums = item.nums * pc->m_level;
            }
            else if (item_type_silver_map == item.type)
            {
                item.type = item_type_silver;
                item.nums = item.nums * pc->m_area;
            }
            else if (item_type_treasure_level == item.type)
            {
                item.type = item_type_treasure;
                item.nums = item.nums * pc->m_level;
            }
            bool new_item = true;
            for (std::list<Item>::iterator it2 = getItems.begin(); it2 != getItems.end(); ++it2)
            {
                if (it2->type == item.type && it2->id == item.id)
                {
                    it2->nums += item.nums;
                    new_item = false;
                    break;
                }
            }
            if (new_item)
            {
                getItems.push_back(item);
            }
        }
        //������
        giveLoots(pc, getItems, 0, pc->m_level, 0, NULL, &robj, true, give_lottery);

        //������˼�¼
        Singleton<lotteryMgr>::instance()->addLotteryRecord(pc->m_id, counts, getItems);
        #if 0
        json_spirit::Array getlist;
        //��ʾ�ۼƻ����Ʒ
        for (std::list<Item>::iterator it = getItems.begin(); it != getItems.end(); ++it)
        {
            Item& item = *it;
            json_spirit::Object obj;
            obj.push_back( Pair("type", item.type) );
            obj.push_back( Pair("count", item.nums) );
            obj.push_back( Pair("id", item.id) );
            getlist.push_back(obj);
        }
        robj.push_back( Pair("list", alist) );
        #endif

        //������ʾ
        std::list<Item> rand_items;
        lootMgr::getInstance()->getLotteryRandItems(bestitem, rand_items);
        json_spirit::Array alist;
        for (std::list<Item>::iterator it = rand_items.begin(); it != rand_items.end(); ++it)
        {
            Item& item = *it;
            json_spirit::Object obj;
            if (bestitem.id == item.id && bestitem.type == item.type
                && bestitem.nums == item.nums && bestitem.fac == item.fac)
            {
                obj.push_back( Pair("get", 1) );
            }
            if (item_type_silver_level == item.type)
            {
                item.type = item_type_silver;
                item.nums = item.nums * pc->m_level;
            }
            else if (item_type_silver_map == item.type)
            {
                item.type = item_type_silver;
                item.nums = item.nums * pc->m_area;
            }
            else if (item_type_treasure_level == item.type)
            {
                item.type = item_type_treasure;
                item.nums = item.nums * pc->m_level;
            }
            item.toObj(obj);
            alist.push_back(obj);
        }
        robj.push_back( Pair("list", alist) );
    }
    return HC_SUCCESS;
}

//��ѯ÷���������˼�¼
int ProcessQueryLotteryRecords(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    //���˼�¼
    Singleton<lotteryMgr>::instance()->queryLotteryRecord(pc->m_id, robj);
    return HC_SUCCESS;
}

//��ѯ÷������ȫ������
int ProcessQueryLotteryNotice(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    //ȫ������
    Singleton<lotteryMgr>::instance()->queryLotteryNotice(robj);
    return HC_SUCCESS;
}

//��ѯ÷����������
int ProcessQueryScore(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    robj.push_back( Pair("score", pc->queryExtraData(char_data_type_week, char_data_extra_lottery_score)) );
    robj.push_back( Pair("nums", pc->treasureCount(treasure_type_yinyang_fish)) );
    return HC_SUCCESS;
}

//lotteryMgr* lotteryMgr::m_handle = NULL;

/*
lotteryMgr* lotteryMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new lotteryMgr();
        m_handle->reload();
    }
    return m_handle;
}*/

lotteryMgr::lotteryMgr()
{
    reload();
}

void lotteryMgr::reload()
{
    Query q(GetDb());
    q.get_result("select value from custom_settings where code='lottery_notices'");
    if (q.fetch_row())
    {
        std::string lottery_notices = q.getstr();
        q.free_result();
        json_spirit::read(lottery_notices, m_notices_value);
    }
    else
    {
        std::string lottery_notices = "[]";
        q.free_result();
        json_spirit::read(lottery_notices, m_notices_value);
    }
}

std::string lotteryMgr::NameToLink(int cid, int gid, std::string name)
{
    std::string result = strLotteryGeneralLink;
    str_replace(result, "$G", LEX_CAST_STR(gid));
    str_replace(result, "$C", LEX_CAST_STR(cid));
    str_replace(result, "$N", name);
    return result;
}

//��ѯ��ɫ÷��������¼
boost::shared_ptr<char_lottery_records> lotteryMgr::getCharRecords(int cid)
{
    boost::shared_ptr<char_lottery_records> rd;    
    std::map<int, boost::shared_ptr<char_lottery_records> >::iterator it = m_char_lottery_records.find(cid);
    if (it != m_char_lottery_records.end())
    {
        return it->second;
    }
    else
    {
        char_lottery_records* precord = new char_lottery_records;
        precord->cid = cid;
        Query q(GetDb());
        q.get_result("select records from char_lottery_records where cid=" + LEX_CAST_STR(cid));
        if (q.fetch_row())
        {            
            precord->record_list_string = q.getstr();
            json_spirit::read(precord->record_list_string, precord->record_list);
        }
        else
        {
            precord->record_list_string = "[]";
            json_spirit::read(precord->record_list_string, precord->record_list);
        }
        q.free_result();
        rd.reset(precord);
    }
    return rd;
}

//���ӽ�ɫ÷��������¼
void lotteryMgr::addLotteryRecord(int cid, int count, const std::list<Item>& getlist)
{
    boost::shared_ptr<char_lottery_records> rd = getCharRecords(cid);
    if (rd.get())
    {
        rd->add(count, getlist);
    }    
}

//����ȫ������
void lotteryMgr::addLotteryNotice(const std::string& name, const std::string& what)
{
    json_spirit::Array& notice_array = m_notices_value.get_array();
    json_spirit::Object obj;
    obj.push_back( Pair("name", name) );
    obj.push_back( Pair("get", what) );
    notice_array.push_back(obj);
    while ((int)notice_array.size() > iLotteryNoticeNum)
    {
        notice_array.erase(notice_array.begin());
    }
    //����
    InsertSaveDb("replace into custom_settings (code,value) values ('lottery_notices','" +GetDb().safestr(json_spirit::write(m_notices_value)) + "')");
}

//��ѯȫ������
int lotteryMgr::queryLotteryNotice(json_spirit::Object& robj)
{
    robj.push_back( Pair("list", m_notices_value.get_array()) );
    return HC_SUCCESS;
}

//��ѯ���˹���
int lotteryMgr::queryLotteryRecord(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<char_lottery_records> rd = getCharRecords(cid);
    if (rd.get())
    {
        //std::string msg = "\"cmd\":\"\",\"s\":200,list:";
        //msg += rd->record_list_string + "}";
        robj.push_back( Pair("list", rd->record_list) );
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//����÷����������
int lotteryMgr::setLotteryScore(int cid, int score, int total_score)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (cdata.get())
    {
        //�����ܻ���
        cdata->setExtraData(char_data_type_week, char_data_extra_lottery_score, score);
        //�����ܻ���
        cdata->setExtraData(char_data_type_normal, char_data_lottery_total_score, total_score);
    }
    return HC_SUCCESS;
}

void char_lottery_records::add(int count, const std::list<Item>& getlist)
{
    json_spirit::Object obj;
    obj.push_back( Pair("times", count) );

    json_spirit::Array str_getlist;
    for (std::list<Item>::const_iterator it = getlist.begin(); it != getlist.end(); ++it)
    {
        str_getlist.push_back(it->toString(true));
    }
    json_spirit::Value glist(str_getlist);    
    obj.push_back( Pair("getlist", glist) );

    json_spirit::Array& array = record_list.get_array();
    array.push_back(obj);

    //ֻ����5��
    while ((int)array.size() > iLotteryRecordNum)
    {
        array.erase(array.begin());
    }
    //���浽���ݿ�
    save();
}

void char_lottery_records::save()
{
    record_list_string = json_spirit::write(record_list);
    InsertSaveDb("replace into char_lottery_records (cid,records) values (" + LEX_CAST_STR(cid) + ",'" + GetDb().safestr(record_list_string) + "')");
}

