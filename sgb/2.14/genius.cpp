#include "statistics.h"

#include "genius.h"
#include "utils_all.h"
#include "spls_errcode.h"
#include "utils_lang.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "spls_const.h"
#include "data.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

geniusMgr* geniusMgr::m_handle = NULL;

static void broadGeniusMsg(std::string& msg, const std::string& cName, const std::string& heroName)
{
    str_replace(msg, "$N", MakeCharNameLink(cName));
    str_replace(msg, "$n", heroName);
    GeneralDataMgr::getInstance()->broadCastSysMsg(msg,-1);
}

geniusMgr* geniusMgr::getInstance()
{
    if (NULL == m_handle)
    {
        time_t time_start = time(NULL);
        cout<<"geniusMgr::getInstance()..."<<endl;
        m_handle = new geniusMgr();
        m_handle->reload();
        cout<<"geniusMgr::getInstance() finish,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    }
    return m_handle;
}

int geniusMgr::reload()
{
    m_base_genius.clear();
    Query q(GetDb());
    double now_init_per = 0.0, now_wash_per = 0.0;
    std::string sql = "select id,cname,memo,color,next_gid,upgrade_per,init_per,wash_per,genius_type,genius_per,genius_value from base_genius";
    q.get_result(sql);
    while(q.fetch_row())
    {
        base_genius base_genius;
        base_genius.id = q.getval();
        base_genius.cname = q.getstr();
        base_genius.memo = q.getstr();
        base_genius.color = q.getval();
        base_genius.nextupid = q.getval();
        base_genius.upgrade_per = q.getnum();
        double init_per = q.getnum();
        if (init_per > 0.0)
        {
            now_init_per += init_per;
            base_genius.init_per = now_init_per;
        }
        double wash_per = q.getnum();
        if (wash_per > 0.0)
        {
            now_wash_per += wash_per;
            base_genius.wash_per = now_wash_per;
        }
        base_genius.genius_type = q.getval();
        base_genius.genius_per = q.getval();
        base_genius.genius_val = q.getval();
        switch (base_genius.color)
        {
            case 0:
                base_genius.colorName = "<font color=\"#ffffff\">" + base_genius.cname + "</font>";
                break;
            case 1:
                base_genius.colorName = "<font color=\"#74D3FB\">" + base_genius.cname + "</font>";
                break;
            case 2:
                base_genius.colorName = "<font color=\"#00FF0C\">" + base_genius.cname + "</font>";
                break;
            case 3:
                base_genius.colorName = "<font color=\"#FFFF00\">" + base_genius.cname + "</font>";
                break;
            case 4:
                base_genius.colorName = "<font color=\"#ff0000\">" + base_genius.cname + "</font>";
                break;
            case 5:
                base_genius.colorName = "<font color=\"#f000ff\">" + base_genius.cname + "</font>";
                break;
            case 6:
            default:
                base_genius.colorName = "<font color=\"#FF9B00\">" + base_genius.cname + "</font>";
                break;
        }
        m_base_genius[base_genius.id] = base_genius;
    }
    q.free_result();

    LinkData();

    //加载锁定记录
    sql = "select gid, lock_genius, nums from char_generals_genius_lock";
    q.get_result(sql);
    while(q.fetch_row())
    {
        general_genius_records* temp = new general_genius_records();
        uint64_t gid = q.getval();
        temp->genius_id = q.getval();
        temp->nums = q.getval();
        temp->updateTag = 1;
        if (m_general_geniusrecord_list[gid] == NULL)
        {
            m_general_geniusrecord_list[gid] = new std::vector<general_genius_records*>;
        }
        m_general_geniusrecord_list[gid]->push_back(temp);
    }
    q.free_result();

    cout << "geniusMgr::reload finished" << endl;
    return 0;
}

void geniusMgr::LinkData()
{
    std::map<int, base_genius>::iterator it = m_base_genius.begin();
    while(it != m_base_genius.end())
    {
        base_genius temp = (*it).second;
        if (temp.nextupid != 0)
        {
            m_base_genius[temp.nextupid].m_preupidList.push_back(temp.id);
        }
        it++;
    }
}

const base_genius& geniusMgr::GetGenius(int nGenius_id)
{
    assert(nGenius_id);
    return m_base_genius[nGenius_id];
}

void geniusMgr::UpdateDB(uint64_t gid)
{
    Query q(GetDb());
    if(m_general_geniusrecord_list.find(gid) != m_general_geniusrecord_list.end())
    {
        std::string sql = "";
        std::vector<general_genius_records*>::iterator it = (*m_general_geniusrecord_list[gid]).begin();
        while(it != (*m_general_geniusrecord_list[gid]).end())
        {
            general_genius_records* temp = *it;
            if (temp->updateTag == 0)        // 插入
            {
                sql = "insert into char_generals_genius_lock(gid,lock_genius,nums) values("+LEX_CAST_STR(gid)+","+LEX_CAST_STR(temp->genius_id)+","+LEX_CAST_STR(temp->nums)+")";
                temp->updateTag = 1;
            }
            else if (temp->updateTag == 2)    // 更新
            {
                sql = "update char_generals_genius_lock set nums="+LEX_CAST_STR(temp->nums)+" where gid="+LEX_CAST_STR(gid)+" and lock_genius="+LEX_CAST_STR(temp->genius_id);
                temp->updateTag = 1;
            }
            else if (temp->updateTag == 3)        // 删除
            {
                sql = "delete from char_generals_genius_lock where gid="+LEX_CAST_STR(gid)+" and lock_genius="+LEX_CAST_STR(temp->genius_id);
                InsertSaveDb(sql);
                delete temp;
                temp = NULL;
                it = (*m_general_geniusrecord_list[gid]).erase(it);
                continue;
            }
            if (sql != "")
            {
                InsertSaveDb(sql);
            }
            it++;
        }
    }
}

//获取武将天赋信息
int geniusMgr::GetGeniusUpgradeList(uint64_t cid, uint64_t gid, json_spirit::Object& retObj)
{
    json_spirit::Array list;
    list.clear();
    Query q(GetDb());
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    CharTotalGenerals& char_generals = cdata->GetGenerals();
    boost::shared_ptr<CharGeneralData> cg = char_generals.GetGenral(gid);
    if (cg.get() && cg->m_level >= iGeniusOpenLevel)
    {
        for (int i = 0; i < iGeniusMaxNum; ++i)
        {
            if (cg->m_genius[i] > 0)
            {
                json_spirit::Object robj;
                robj.clear();
                base_genius temgenius = GetGenius(cg->m_genius[i]);
                json_spirit::Object GeniusVO;
                GeniusVO.clear();
                GeniusVO.push_back( Pair("id",temgenius.id) );
                GeniusVO.push_back( Pair("name",temgenius.cname) );
                GeniusVO.push_back( Pair("quality",temgenius.color) );
                GeniusVO.push_back( Pair("memo",temgenius.memo) );
                GeniusVO.push_back( Pair("isOpen",true) );
                GeniusVO.push_back( Pair("isLocked",cg->m_genius_lock[i]) );
                GeniusVO.push_back( Pair("gold",0) );
                robj.push_back( Pair("genius",GeniusVO) );
                if (temgenius.nextupid > 0)
                {
                    base_genius nextgenius = GetGenius(temgenius.nextupid);
                    GeniusVO.clear();
                    GeniusVO.push_back( Pair("name",nextgenius.cname) );
                    GeniusVO.push_back( Pair("quality",nextgenius.color) );
                    GeniusVO.push_back( Pair("more_genius",nextgenius.m_preupidList.size() > 1) );
                    GeniusVO.push_back( Pair("can_up",CheckGeniusCanUp(cid,gid,temgenius.id)) );
                    GeniusVO.push_back( Pair("memo",nextgenius.memo) );
                    robj.push_back( Pair("nextGenius",GeniusVO) );
                }
                list.push_back(robj);
            }
            else
            {
                json_spirit::Object robj;
                robj.clear();
                json_spirit::Object GeniusVO;
                GeniusVO.clear();
                GeniusVO.push_back( Pair("isOpen",false) );
                GeniusVO.push_back( Pair("gold",iGeniusOpenGold[i]) );
                robj.push_back( Pair("genius",GeniusVO) );
                list.push_back(robj);
            }
        }
    }
    retObj.push_back( Pair("list",list) );
    return HC_SUCCESS;
}

// 增加武将天赋
int geniusMgr::OpenGenius(uint64_t cid, uint64_t gid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    CharTotalGenerals& char_generals = cdata->GetGenerals();
    boost::shared_ptr<CharGeneralData> cg = char_generals.GetGenral(gid);
    if (cg.get() && cg->m_level >= iGeniusOpenLevel)
    {
        if (cg->m_genius_count >= iGeniusMaxNum)
        {
            return HC_ERROR;
        }
        if (cdata->m_vip < iGeniusOpenVIP[cg->m_genius_count])
        {
            return HC_ERROR_MORE_VIP_LEVEL;
        }
        if (-1 == cdata->addGold(-iGeniusOpenGold[cg->m_genius_count]))
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        add_statistics_of_gold_cost(cid,cdata->m_ip_address,iGeniusOpenGold[cg->m_genius_count],gold_cost_for_open_genius, cdata->m_union_id, cdata->m_server_id);
        //增加天赋cg->m_genius[genius_num];
        bool wash_suc = false;
        do
        {
            double rand = my_random(0.00,100.00);
            std::map<int, base_genius>::iterator it = m_base_genius.begin();
            while (it != m_base_genius.end())
            {
                base_genius temp = (*it).second;
                if (temp.id > 0 && temp.init_per > 0.0 && temp.init_per >= rand)
                {
                    if (!CheckGeniusInList(temp.id, cg->m_genius))
                    {
                        cg->m_genius[cg->m_genius_count] = temp.id;
                        ++cg->m_genius_count;
                        if (cg->m_genius_count == iGeniusMaxNum && cg->m_color >= 5 && cg->m_baseGeneral->m_nickname != "")
                        {
                            cg->b_nickname = 1;
                            std::string msg = strGeniusOpenBroad;
                            str_replace(msg, "$N", cdata->m_name);
                            str_replace(msg, "$n", cg->colorLink());
                            str_replace(msg, "$G", cg->m_baseGeneral->m_nickname);
                            GeneralDataMgr::getInstance()->broadCastSysMsg(msg,-1);
                            
                            msg = strGeniusOpenFullMsg;
                            str_replace(msg, "$n", cg->colorLink());
                            str_replace(msg, "$G", cg->m_baseGeneral->m_nickname);
                            robj.push_back( Pair("msg", msg) );
                        }
                        else
                        {
                            cg->b_nickname = 0;
                            std::string msg = strGeniusOpenMsg;
                            str_replace(msg, "$N", LEX_CAST_STR(cg->m_genius_count));
                            robj.push_back( Pair("msg", msg) );
                        }
                        //直接开出红色天赋，广播
                        if (temp.color == 4)
                        {
                            std::string msg = strGeniusCleanRed;
                            str_replace(msg, "$G", temp.colorName);
                            broadGeniusMsg(msg, cdata->m_name, cg->colorLink());
                        }
                        wash_suc = true;
                    }
                    //随机到一个已经有的天赋，再次随机
                    break;
                }
                ++it;
            }
        }while(!wash_suc);
        cg->updateGeniusAttribute();
    }
    return HC_SUCCESS;
}

//需要特殊条件的天赋进阶判断
bool geniusMgr::CheckGeniusCanUp(uint64_t cid, uint64_t gid, int genius_id)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return false;
    CharTotalGenerals& char_generals = cdata->GetGenerals();
    boost::shared_ptr<CharGeneralData> cg = char_generals.GetGenral(gid);
    base_genius curGenius = m_base_genius[genius_id];
    base_genius nextGenius = m_base_genius[curGenius.nextupid];
    //进阶天赋有了
    if (curGenius.nextupid <= 0 || CheckGeniusInList(curGenius.nextupid,cg->m_genius))
    {
        return false;
    }
    //紫色橙色天赋需要激活绰号
    if (nextGenius.color >= 5 && !cg->b_nickname)
    {
        return false;
    }
    //进阶天赋需要多个的
    if (nextGenius.m_preupidList.size() > 1)
    {
        // 查找是否存在所有前置特长
        std::vector<int>::iterator it = nextGenius.m_preupidList.begin();
        while(it != nextGenius.m_preupidList.end())
        {
            //*it是否锁定
            if (!CheckGeniusInList(*it, cg->m_genius))
            {
                return false;
            }
            it++;
        }
    }
    return true;
}

bool geniusMgr::CheckGeniusInList(int genius_id, const std::vector<int>& list)
{
    for (size_t i = 0; i < list.size(); ++i)
    {
        if (list[i] == genius_id)
        {
            return true;
        }
    }
    return false;
}

//洗天赋
int geniusMgr::CleanGenius(uint64_t cid, uint64_t gid, const std::vector<int>& lock_list, json_spirit::Object& robj)
{
    cout << "gid=" << gid << " CleanGenius!!!" << endl;
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    int genius_num = 0;
    CharTotalGenerals& char_generals = cdata->GetGenerals();
    boost::shared_ptr<CharGeneralData> cg = char_generals.GetGenral(gid);
    if (cg.get() && cg->m_level >= iGeniusOpenLevel)
    {
        genius_num = cg->m_genius_count;
        if (genius_num < 1 || lock_list.size() > (size_t)genius_num)
        {
            return HC_ERROR;
        }
        int needgold = 0;
        //锁定价格根据颜色定
        for (size_t i = 0; i < lock_list.size(); ++i)
        {
            //锁定特长不在现有特长范围内
            if (!CheckGeniusInList(lock_list[i], cg->m_genius))
            {
                return HC_ERROR;
            }
            if (lock_list[i] > 0)
            {
                base_genius bg = GetGenius(lock_list[i]);
                if (bg.color >= 1 && bg.color <= 6)
                {
                    needgold += iGeniusLockGold[bg.color - 1];
                }
            }
        }
        needgold += iGeniusWashGold;
        if (-1 == cdata->addGold(-needgold))
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        add_statistics_of_gold_cost(cid,cdata->m_ip_address,needgold,gold_cost_for_open_genius, cdata->m_union_id, cdata->m_server_id);

        //锁定情况信息更替
        std::vector<general_genius_records*>*geniuslst = NULL;
        if (m_general_geniusrecord_list.find(gid) != m_general_geniusrecord_list.end())
        {
            geniuslst = m_general_geniusrecord_list[gid];
        }
        else
        {
            cout << "no this vecor:" << gid << endl;
            geniuslst = new std::vector<general_genius_records*>;
            m_general_geniusrecord_list[gid] = geniuslst;
        }
        //清除不再锁定的天赋
        std::vector<general_genius_records*>::iterator it = geniuslst->begin();
        while(it != geniuslst->end())
        {
            if (!CheckGeniusInList((*it)->genius_id, lock_list))
            {
                if ((*it)->updateTag == 0)
                {
                    delete (*it);
                    (*it) = NULL;
                    geniuslst->erase(it);
                }
                else
                    (*it)->SetDeleteFlag();
            }
            it++;
        }
        for (int j = 0; j < iGeniusMaxNum; ++j)
        {
            cg->m_genius_lock[j] = false;
            if (cg->m_genius[j] <= 0)//天赋未开启
            {
                continue;
            }
            else if (CheckGeniusInList(cg->m_genius[j], lock_list))//已经锁定
            {
                cg->m_genius_lock[j] = true;
                bool bFind = false;//是否现有锁定
                std::vector<general_genius_records*>::iterator it = geniuslst->begin();
                while(it != geniuslst->end())
                {
                    if (cg->m_genius[j] == (*it)->genius_id)//老锁定
                    {
                        bFind = true;
                        if ((*it)->nums < 200)
                        {
                            (*it)->nums++;
                            (*it)->SetUpdateFlag();
                        }
                        break;
                    }
                    it++;
                }
                if (!bFind)//新加锁定
                {
                    general_genius_records* general_genius_lock = new general_genius_records();
                    general_genius_lock->genius_id = cg->m_genius[j];
                    general_genius_lock->nums = 1;
                    geniuslst->push_back(general_genius_lock);
                }
            }
        }

        std::vector<int> has_list = lock_list;
        std::vector<int> try_list;
        for (int i = 0; i < genius_num; ++i)
        {
            int new_genius = 0;
            cout << "now----pos=" << i << " genius_id=" << cg->m_genius[i] << endl;
            if (cg->m_genius[i] > 0)
            {
                if (CheckGeniusInList(cg->m_genius[i], lock_list))
                {
                    cout << "this is locked!!!" << endl;
                    //锁定的天赋进阶洗
                    new_genius = cg->m_genius[i];
                    base_genius curGenius = m_base_genius[new_genius];
                    base_genius nextGenius = m_base_genius[curGenius.nextupid];
                    //进阶天赋有了或者本次尝试洗过
                    if (curGenius.nextupid <= 0 || CheckGeniusInList(curGenius.nextupid,cg->m_genius) || CheckGeniusInList(curGenius.nextupid, try_list))
                    {
                        continue;
                    }
                    //紫色橙色天赋需要激活绰号
                    if (nextGenius.color >= 5 && !cg->b_nickname)
                    {
                        continue;
                    }
                    bool wash_suc = WashGeniusLock(gid, try_list, lock_list, new_genius);
                    if (wash_suc)
                    {
                        std::string msg = "";
                        if (nextGenius.color == 6)
                        {
                            msg = strGeniusCleanOrange;
                        }
                        else if (nextGenius.color == 5)
                        {
                            msg = strGeniusCleanPurple;
                        }
                        else if(nextGenius.color == 4)
                        {
                            msg = strGeniusCleanRed;
                        }
                        if (msg != "")
                        {
                            str_replace(msg, "$G", nextGenius.colorName);
                            broadGeniusMsg(msg, cdata->m_name, cg->colorLink());
                        }
                        cg->m_genius[i] = new_genius;
                        has_list.push_back(new_genius);
                        msg = "";
                        //清除前置天赋
                        if (nextGenius.m_preupidList.size() > 1)
                        {
                            msg = strGeniusCleanMoreMsg;
                            std::vector<int>::iterator it = nextGenius.m_preupidList.begin();
                            while(it != nextGenius.m_preupidList.end())
                            {
                                for (int j = 0; j < iGeniusMaxNum; ++j)
                                {
                                    if (cg->m_genius[j] == *it)//现有天赋中是前置则清除
                                    {
                                        cg->m_genius[j] = 0;
                                    }
                                }
                                base_genius tmp = m_base_genius[*it];
                                str_replace(msg, "$n", tmp.cname);
                                it++;
                            }
                            str_replace(msg, "$G", nextGenius.cname);
                        }
                        else
                        {
                            msg = strGeniusCleanMsg;
                            str_replace(msg, "$n", curGenius.cname);
                            str_replace(msg, "$G", nextGenius.cname);
                        }
                        robj.push_back( Pair("msg", msg) );
                    }
                    cout << "after lockwash! genius=" << new_genius << endl;
                }
                else
                {
                    cout << "this is unlocked!!!" << endl;
                    new_genius = cg->m_genius[i];
                    //未锁定的天赋普通洗
                    std::string msg = "";
                    WashGeniusUnlock(has_list, new_genius, msg);
                    if (msg != "")
                    {
                        broadGeniusMsg(msg, cdata->m_name, cg->colorLink());
                    }
                    cg->m_genius[i] = new_genius;
                    has_list.push_back(new_genius);
                    cout << "after unlockwash! genius=" << new_genius << endl;
                }
            }
            else//多天赋进阶之后的空缺天赋用初始天赋补充
            {
                cout << "this need init!!!" << endl;
                bool wash_suc = false;
                do
                {
                    double rand = my_random(0.00,100.00);
                    std::map<int, base_genius>::iterator it = m_base_genius.begin();
                    while (it != m_base_genius.end())
                    {
                        base_genius temp = (*it).second;
                        if (temp.id > 0 && temp.init_per > 0.0 && temp.init_per >= rand && !CheckGeniusInList(temp.id, has_list))
                        {
                            cg->m_genius[i] = temp.id;
                            wash_suc = true;
                            //有可能直接洗出红色天赋
                            if (temp.color == 4)
                            {
                                std::string msg = strGeniusCleanRed;
                                str_replace(msg, "$G", temp.colorName);
                                broadGeniusMsg(msg, cdata->m_name, cg->colorLink());
                            }
                            break;
                        }
                        ++it;
                    }
                    cout << "init is " << wash_suc << endl;
                }while(!wash_suc);
            }
        }
        //test_server
        cout << "after clean!!!" << endl;
        for (size_t i = 0; i < cg->m_genius.size(); ++i)
        {
            if (cg->m_genius[i] > 0)
            {
                cout << "pos" << i << "=" << cg->m_genius[i] << endl;
            }
        }
        cg->updateGeniusAttribute();
        UpdateDB(gid);//更新锁定信息
    }
    return HC_SUCCESS;
}

bool geniusMgr::WashGeniusUnlock(const std::vector<int>& already_list, int& genius_id, std::string& msg)
{
    cout << "geniusMgr::WashGeniusUnlock" << " genius_id=" << genius_id << endl;
    bool wash_suc = false;
    int wash_genius_cnt = 0;
    std::map<int, base_genius>::iterator it = m_base_genius.begin();
    while (it != m_base_genius.end())
    {
        base_genius temp = (*it).second;
        if (temp.id > 0 && temp.wash_per > 0)
        {
            ++wash_genius_cnt;
        }
        ++it;
    }
    //天赋数量错误
    if (wash_genius_cnt < 1 || (size_t)wash_genius_cnt <= already_list.size())
    {
        cout << "wash_genius_cnt = " << wash_genius_cnt << ",already_list.size=" << already_list.size() << endl;
        return false;
    }
    do
    {
        double rand = my_random(0.00,100.00);
        cout << "this time random_result=" << rand << endl;
        std::map<int, base_genius>::iterator it = m_base_genius.begin();
        while (it != m_base_genius.end())
        {
            base_genius temp = (*it).second;
            if (temp.id > 0 && temp.wash_per > 0.0 && temp.wash_per >= rand)
            {
                if (!CheckGeniusInList(temp.id, already_list))
                {
                    cout << "id=" << temp.id << ",wash_per=" << temp.wash_per << endl;
                    genius_id = temp.id;
                    wash_suc = true;
                    //有可能直接洗出红色天赋
                    if (temp.color == 4)
                    {
                        msg = strGeniusCleanRed;
                        str_replace(msg, "$G", temp.colorName);
                    }
                    else if (temp.color == 5)
                    {
                        msg = strGeniusCleanPurple;
                        str_replace(msg, "$G", temp.colorName);
                    }
                }
                else
                {
                    cout << "id=" << temp.id << " is rand,but already!!!" << endl;
                }
                break;
            }
            ++it;
        }
        cout << "wash_suc=" << wash_suc << endl;
    }while(!wash_suc);
    return wash_suc;
}

bool geniusMgr::WashGeniusLock(uint64_t gid, std::vector<int>& try_list, const std::vector<int>& lock_list, int& genius_id)
{
    cout << "geniusMgr::WashGeniusLock" << " genius_id=" << genius_id << endl;
    // 属性是否能进阶
    base_genius curGenius = m_base_genius[genius_id];
    if (curGenius.nextupid <= 0)
    {
        return false;
    }
    base_genius nextGenius = m_base_genius[curGenius.nextupid];
    //把当前尝试洗的天赋记录到trylist
    try_list.push_back(nextGenius.id);
    //进阶天赋需要多个的
    if (nextGenius.m_preupidList.size() > 1)
    {
        cout << "nextGenius need some pre_genius" << endl;
        // 查找是否存在锁定的特长
        std::vector<int>::iterator it = nextGenius.m_preupidList.begin();
        while(it != nextGenius.m_preupidList.end())
        {
            //*it是否锁定
            if (!CheckGeniusInList(*it, lock_list))
            {
                cout << *it << " is unlock!" << endl;
                return false;
            }
            it++;
        }
    }

    bool bFind = false;// 是否存在锁定记录
    bool bCanGet = false;//进阶紫色需要50次以上锁定
    if (m_general_geniusrecord_list.find(gid) != m_general_geniusrecord_list.end())
    {
        std::vector<general_genius_records*>::iterator it = (*m_general_geniusrecord_list[gid]).begin();
        while(it != (*m_general_geniusrecord_list[gid]).end())
        {
            if ((*it)->genius_id == genius_id)
            {
                bFind = true;
                cout << "refresh_genius:id=" << genius_id << ",locknums=" << (*it)->nums << endl;
                if ((*it)->nums >= 50)
                {
                    bCanGet = true;
                }
                break;
            }
            it++;
        }
    }
    if (!bFind)        // 没找到该英雄的记录
    {
        return HC_ERROR;
    }

    double getRand = curGenius.upgrade_per;
    //进阶紫色前50次无概率
    if (nextGenius.color >= 5 && !bCanGet)
    {
        getRand = 0.00;
    }
    // 根据概率获取进阶后的特长
    double nRnd = my_random(0.00,100.00);
    if (getRand == 0.00)
    {
        return false;
    }
    cout << "random_result=" << nRnd << ", suc_per=" << getRand << endl;
    if (nRnd < getRand)// 成功进阶
    {
        genius_id = nextGenius.id;
        if ((nextGenius.color >= 5) && m_general_geniusrecord_list.find(gid) != m_general_geniusrecord_list.end())
        {
            std::vector<general_genius_records*>::iterator it = (*m_general_geniusrecord_list[gid]).begin();
            while(it != (*m_general_geniusrecord_list[gid]).end())
            {
                base_genius bg = GetGenius((*it)->genius_id);
                if (bg.color >= 5)
                {
                    if ((*it)->updateTag == 0)
                    {
                        delete (*it);
                        (*it) = NULL;
                        it = (*m_general_geniusrecord_list[gid]).erase(it);
                        continue;
                    }
                    else
                        (*it)->SetDeleteFlag();
                }
                it++;
            }
        }
        return true;
    }
    return false;
}

//移植天赋
int geniusMgr::GraftGenius(uint64_t cid, uint64_t gid, uint64_t gid2, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    if (gid == gid2)
    {
        return HC_ERROR_GRAFT_GENIUS;
    }
    int genius_num = 0;
    CharTotalGenerals& char_generals = cdata->GetGenerals();
    boost::shared_ptr<CharGeneralData> cg = char_generals.GetGenral(gid);
    boost::shared_ptr<CharGeneralData> cg2 = char_generals.GetGenral(gid2);
    if (cg.get() && cg->m_level >= iGeniusOpenLevel && cg2.get() && cg2->m_level >= iGeniusOpenLevel)
    {
        genius_num = cg->m_genius_count;
        if (genius_num < 1)
        {
            return HC_ERROR;
        }
        if (cg->m_genius_count > cg2->m_genius_count)
        {
            return HC_ERROR_GRAFT_GENIUS_NUM;
        }
        int needgold = 0;
        //移植价格根据颜色定
        for (size_t i = 0; i < cg->m_genius.size(); ++i)
        {
            if (cg->m_genius[i] > 0)
            {
                base_genius bg = GetGenius(cg->m_genius[i]);
                if (bg.color >= 1 && bg.color <= 6)
                {
                    needgold += iGeniusGraftGold[bg.color - 1];
                }
                if (bg.color >= 5 && !cg2->b_nickname)
                {
                    return HC_ERROR_GRAFT_GENIUS_COLOR;
                }
            }
        }
        if (-1 == cdata->addGold(-needgold))
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        add_statistics_of_gold_cost(cid,cdata->m_ip_address,needgold,gold_cost_for_graft_genius, cdata->m_union_id, cdata->m_server_id);

        //锁定情况清除
        std::vector<general_genius_records*>*geniuslst = NULL;
        if (m_general_geniusrecord_list.find(gid) != m_general_geniusrecord_list.end())
        {
            geniuslst = m_general_geniusrecord_list[gid];
        }
        else
        {
            cout << "no this vecor:" << gid << endl;
            geniuslst = new std::vector<general_genius_records*>;
            m_general_geniusrecord_list[gid] = geniuslst;
        }
        std::vector<general_genius_records*>::iterator it = geniuslst->begin();
        while(it != geniuslst->end())
        {
            if ((*it)->updateTag == 0)
            {
                delete (*it);
                (*it) = NULL;
                geniuslst->erase(it);
            }
            else
                (*it)->SetDeleteFlag();
            it++;
        }
        geniuslst = NULL;
        if (m_general_geniusrecord_list.find(gid2) != m_general_geniusrecord_list.end())
        {
            geniuslst = m_general_geniusrecord_list[gid2];
        }
        else
        {
            cout << "no this vecor:" << gid2 << endl;
            geniuslst = new std::vector<general_genius_records*>;
            m_general_geniusrecord_list[gid2] = geniuslst;
        }
        it = geniuslst->begin();
        while(it != geniuslst->end())
        {
            if ((*it)->updateTag == 0)
            {
                delete (*it);
                (*it) = NULL;
                geniuslst->erase(it);
            }
            else
                (*it)->SetDeleteFlag();
            it++;
        }
        int graft_num = 0;
        for (int j = 0; j < iGeniusMaxNum; ++j)
        {
            cg->m_genius_lock[j] = false;
            cg2->m_genius_lock[j] = false;
            if (cg->m_genius[j] <= 0 || cg2->m_genius[j] <= 0)//天赋未开启
            {
                continue;
            }
            ++graft_num;
            cg2->m_genius[j] = cg->m_genius[j];
            bool wash_suc = false;
            do
            {
                double rand = my_random(0.00,100.00);
                std::map<int, base_genius>::iterator it = m_base_genius.begin();
                while (it != m_base_genius.end())
                {
                    base_genius temp = (*it).second;
                    if (temp.id > 0 && temp.init_per > 0.0 && temp.init_per >= rand)
                    {
                        if (!CheckGeniusInList(temp.id, cg->m_genius))
                        {
                            cg->m_genius[j] = temp.id;
                            wash_suc = true;
                        }
                        //随机到一个已经有的天赋，再次随机
                        break;
                    }
                    ++it;
                }
            }while(!wash_suc);
        }
        //test_server
        cout << "after clean!!!" << endl;
        for (size_t i = 0; i < cg->m_genius.size(); ++i)
        {
            if (cg->m_genius[i] > 0)
            {
                cout << "pos" << i << "=" << cg->m_genius[i] << endl;
            }
        }
        cg->updateGeniusAttribute();
        cg2->updateGeniusAttribute();
        UpdateDB(gid);
        UpdateDB(gid2);
        std::string msg = strGeniusGraftMsg;
        str_replace(msg, "$N", cg->m_baseGeneral->m_name);
        str_replace(msg, "$n", cg2->m_baseGeneral->m_name);
        str_replace(msg, "$n", cg2->m_baseGeneral->m_name);
        str_replace(msg, "$G", LEX_CAST_STR(graft_num));
        robj.push_back( Pair("msg", msg) );
        json_spirit::Array list1,list2;
        list1.clear();
        list2.clear();
        for (int i = 0; i < iGeniusMaxNum; ++i)
        {
            if (cg->m_genius[i] > 0)
            {
                json_spirit::Object o;
                o.clear();
                base_genius temgenius = GetGenius(cg->m_genius[i]);
                json_spirit::Object GeniusVO;
                GeniusVO.clear();
                GeniusVO.push_back( Pair("id",temgenius.id) );
                GeniusVO.push_back( Pair("name",temgenius.cname) );
                GeniusVO.push_back( Pair("quality",temgenius.color) );
                GeniusVO.push_back( Pair("memo",temgenius.memo) );
                GeniusVO.push_back( Pair("isOpen",true) );
                GeniusVO.push_back( Pair("isLocked",cg->m_genius_lock[i]) );
                GeniusVO.push_back( Pair("gold",0) );
                o.push_back( Pair("genius",GeniusVO) );
                list1.push_back(o);
            }
            else
            {
                json_spirit::Object o;
                o.clear();
                json_spirit::Object GeniusVO;
                GeniusVO.clear();
                GeniusVO.push_back( Pair("isOpen",false) );
                GeniusVO.push_back( Pair("gold",iGeniusOpenGold[i]) );
                o.push_back( Pair("genius",GeniusVO) );
                list1.push_back(o);
            }
            if (cg2->m_genius[i] > 0)
            {
                json_spirit::Object o;
                o.clear();
                base_genius temgenius = GetGenius(cg2->m_genius[i]);
                json_spirit::Object GeniusVO;
                GeniusVO.clear();
                GeniusVO.push_back( Pair("id",temgenius.id) );
                GeniusVO.push_back( Pair("name",temgenius.cname) );
                GeniusVO.push_back( Pair("quality",temgenius.color) );
                GeniusVO.push_back( Pair("memo",temgenius.memo) );
                GeniusVO.push_back( Pair("isOpen",true) );
                GeniusVO.push_back( Pair("isLocked",cg->m_genius_lock[i]) );
                GeniusVO.push_back( Pair("gold",0) );
                o.push_back( Pair("genius",GeniusVO) );
                list2.push_back(o);
            }
            else
            {
                json_spirit::Object o;
                o.clear();
                json_spirit::Object GeniusVO;
                GeniusVO.clear();
                GeniusVO.push_back( Pair("isOpen",false) );
                GeniusVO.push_back( Pair("gold",iGeniusOpenGold[i]) );
                o.push_back( Pair("genius",GeniusVO) );
                list2.push_back(o);
            }
        }
        robj.push_back( Pair("list1",list1) );
        robj.push_back( Pair("list2",list2) );
    }
    return HC_SUCCESS;
}

int geniusMgr::setGenius(int cid, int gid, int genius_id, int pos)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    int genius_num = 0;
    CharTotalGenerals& char_generals = cdata->GetGenerals();
    boost::shared_ptr<CharGeneralData> cg = char_generals.GetGenral(gid);
    if (cg.get() && cg->m_level >= iGeniusOpenLevel)
    {
        //已有天赋不能设置
        if (genius_id <= 0 || CheckGeniusInList(genius_id, cg->m_genius))
        {
            return HC_ERROR;
        }
        genius_num = cg->m_genius_count;
        //不能超出上限
        if (pos > iGeniusMaxNum)
        {
            return HC_ERROR;
        }
        if (cdata->m_vip < iGeniusOpenVIP[genius_num])
        {
            return HC_ERROR_MORE_VIP_LEVEL;
        }
        //不能设置未开放的天赋
        if ((pos - 1) < genius_num)
        {
            cg->m_genius[pos-1] = genius_id;
        }
        cg->updateGeniusAttribute();
    }
    return HC_SUCCESS;
}

