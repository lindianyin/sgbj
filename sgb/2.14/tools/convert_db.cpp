

/*
select concat(
char(round((rand())*9)+48),
char(round((rand())*9)+48),
char(round((rand())*9)+48),
char(round((rand())*9)+48),
char(round((rand())*9)+48),
char(round((rand())*9)+48)
)

update temp_test_accounts set password=concat(
char(round((rand())*9)+48),
char(round((rand())*9)+48),
char(round((rand())*9)+48),
char(round((rand())*9)+48),
char(round((rand())*9)+48),
char(round((rand())*9)+48) where 1

*/
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include <boost/lexical_cast.hpp>

#include <map>
#include <iostream>
#include <list>
#include "boost/smart_ptr/shared_ptr.hpp"

using namespace std;


#define INFO(x) cout<<x<<endl

#define CHECK_DB_ERR(q) if (q.GetErrno())\
    {\
         cout<<"sql:"<<q.GetLastQuery()<<endl<<"error:"<<q.GetError()<<",errno:"<<q.GetErrno()<<endl;\
    }

class testSharePtr
{
public:
    testSharePtr(int id)
    {
        _id = id;
        cout<<"testSharePtr("<<id<<")"<<endl;
    }
    ~testSharePtr()
    {
        cout<<"~testSharePtr("<<_id<<")"<<endl;
    }
private:
    int _id;
};

std::string db_name = "spls";    

int ConvertBaseSoldiers()
{
    INFO("ConvertBaseSoldiers()...");
    Database db("localhost", "c_user", "23rf234", db_name);//, CLIENT_INTERACTIVE);
    Query q(db);

    std::map<std::string, int> baseSodierMap;   //基础兵种名->基础兵种id
    q.get_result("select name,btype from base_stypes where 1");
    while (q.fetch_row())
    {
        baseSodierMap[q.getstr()] = q.getval();
    }
    q.free_result();

    //std::map<std::string, int> sodierMap;       //兵种名->兵种id

    std::map<std::string, int> act_TypeMap;     //行动方式映射
    act_TypeMap["普攻"] = 1;
    act_TypeMap["策攻"] = 2;
    act_TypeMap["治疗"] = 3;
    act_TypeMap["鼓舞"] = 4;

    std::map<std::string, int> act_TargetMap;   //行动目标映射
/*
    range_single = 1,   //单体
    range_chuantou,     //穿透
    range_fenlie,       //分裂
    range_around,       //眦邻
    range_single_back,  //单体 (后排)
    range_three,        //引导
    range_all,          //全体
*/
    act_TargetMap["单体（前排）"] = 1;
    act_TargetMap["单体（前排）"] = 1;
    act_TargetMap["穿透（一列）"] = 2;
    act_TargetMap["分裂（一行）"] = 3;
    act_TargetMap["溅射（多个）"] = 4;
    act_TargetMap["单体（后排）"] = 5;
    act_TargetMap["引导（每列）"] = 6;
    act_TargetMap["全体（对方）"] = 7;
    act_TargetMap["全体（己方）"] = 8;
    act_TargetMap["单体（己方）"] = 9;

    std::map<std::string, int> state_map;    //状态
    q.get_result("select name,id from base_states where 1");
    while (q.fetch_row())
    {
        state_map[q.getstr()] = q.getval();
    }
    q.free_result();

    q.execute("TRUNCATE table base_soldiers");
    q.execute("insert into base_soldiers (stype,spic,name,fail,attack,pufang,cefang,memo) (select stype,stype,name,100*fail,attack,pufang,cefang,memo from temp_base_soldiers where 1)");
    CHECK_DB_ERR(q);
    Query q2(db);
    q.get_result("select stype,base_stype,act_type,act_target,spe1,spe2,spe3 from temp_base_soldiers where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int stype = q.getval();
        std::string base_type = q.getstr();
        std::string act_type = q.getstr();
        std::string act_target = q.getstr();
        int ibase_type = baseSodierMap[base_type];
        int iact_type = act_TypeMap[act_type];
        int iact_target = act_TargetMap[act_target];
        std::string spe1 = q.getstr();
        std::string spe2 = q.getstr();
        std::string spe3 = q.getstr();
        
        int ispe1 = state_map[spe1];
        int ispe2 = state_map[spe2];
        int ispe3 = state_map[spe3];
        if (spe1 != "" && ispe1 == 0)
        {
            cout<<"soldier id "<<stype<<",error spe1"<<endl;
        }
        if (spe2 != "" && ispe2 == 0)
        {
            cout<<"soldier id "<<stype<<",error spe2"<<endl;
        }
        if (spe3 != "" && ispe3 == 0)
        {
            cout<<"soldier id "<<stype<<",error spe3"<<endl;
        }
        INFO("update base_soldiers set ["<<stype<<"]:"<<ibase_type<<","<<iact_type<<","<<iact_target);
        if (!q2.execute("update base_soldiers set base_stype=" + boost::lexical_cast<string>(ibase_type)
                    + ",act_type=" + boost::lexical_cast<string>(iact_type) + ",act_target=" + boost::lexical_cast<string>(iact_target)
                    + ",spe1=" + boost::lexical_cast<string>(ispe1) + ",spe2=" + boost::lexical_cast<string>(ispe2) + ",spe3=" + boost::lexical_cast<string>(ispe3)
                    + " where stype=" + boost::lexical_cast<string>(stype)))
        {
            INFO(q2.GetLastQuery()<<endl<<"error:"<<q2.GetError()<<",errno:"<<q2.GetErrno());
        }
    }
    q.free_result();
    return 0;   
}

int ConvertBaseGenerals()
{
    Database db("localhost", "c_user", "23rf234", db_name);//, CLIENT_INTERACTIVE);
    Query q(db);

    std::map<std::string, int> soldierMap;       //兵种名->兵种id
    q.get_result("select name,stype from base_soldiers where 1");
    while (q.fetch_row())
    {
        soldierMap[q.getstr()] = q.getval();
    }
    q.free_result();

    std::map<std::string, int> bwTypeMap;       //宝物种类类型
    bwTypeMap["勇"] = 1;
    bwTypeMap["智"] = 2;
    bwTypeMap["统"] = 3;
    
    q.execute("TRUNCATE table base_generals");
    q.execute("insert into base_generals (gid,spic,name,tong,str,wisdom,baowu,bwBaseValue,bwAddPerLev,bwMaxLevel,memo) (select gid,gid,name,tong,str,wisdom,baowu,bwBaseValue,bwAddPerLev,bwMaxLevel,memo from temp_base_generals where 1)");
    CHECK_DB_ERR(q);
    Query q2(db);
    q.get_result("select gid,stype,bwType from temp_base_generals where 1");
    while (q.fetch_row())
    {
        int gid = q.getval();
        std::string stype = q.getstr();
        std::string bwType = q.getstr();

        int istype = soldierMap[stype];
        int ibwType = bwTypeMap[bwType];

        INFO("update base_generals set ["<<gid<<"]:"<<istype<<","<<ibwType);
        if (!q2.execute("update base_generals set stype=" + boost::lexical_cast<string>(istype)
                    + ",bwType=" + boost::lexical_cast<string>(ibwType)
                    + " where gid=" + boost::lexical_cast<string>(gid)))
        {
            INFO(q2.GetLastQuery()<<endl<<"error:"<<q2.GetError()<<",errno:"<<q2.GetErrno());
        }
    }
    q.free_result();
    return 0;   
}

int ConvertBaseStronghold()
{
    Database db("localhost", "c_user", "23rf234", db_name);//, CLIENT_INTERACTIVE);
    Query q(db);

    std::map<std::string, int> sType;       //关卡类型
    sType["普通"] = 1;
    sType["精英"] = 2;
    sType["BOSS"] = 3;
    
    q.execute("TRUNCATE table base_stronghold");
    CHECK_DB_ERR(q);
    q.execute("insert into base_stronghold (id,name,level,mapid,stageid) (select id,name,level,mapid,stageid from temp_base_stronghold where 1)");
    CHECK_DB_ERR(q);
    Query q2(db);
    q.get_result("select id,type from temp_base_stronghold where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        std::string stype = q.getstr();
        int type = sType[stype];
        if (!q2.execute("update base_stronghold set type=" + boost::lexical_cast<string>(type)
                    + " where id=" + boost::lexical_cast<string>(id)))
        {
            INFO(q2.GetLastQuery()<<endl<<"error:"<<q2.GetError()<<",errno:"<<q2.GetErrno());
        }
    }
    q.free_result();
    return 0;   
}

int ConvertBaseStrongholdGenerals()
{
    Database db("localhost", "c_user", "23rf234", db_name);//, CLIENT_INTERACTIVE);
    Query q(db);

    std::map<std::string, int> soldierMap;       //兵种名->兵种id
    q.get_result("select name,stype from base_soldiers where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        soldierMap[q.getstr()] = q.getval();
    }
    q.free_result();
    
    q.execute("TRUNCATE table base_stronghold_generals");
    CHECK_DB_ERR(q);
    q.execute("insert into base_stronghold_generals (stronghold,pos,name,hp,attack,pufang,cefang,str,wisdom) (select stronghold,pos,name,hp,attack,pufang,cefang,str,wisdom from temp_base_stronghold_generals where 1)");
    CHECK_DB_ERR(q);
    Query q2(db);
    q.get_result("select stronghold,pos,stype from temp_base_stronghold_generals where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int sgid = q.getval();
        int pos = q.getval();
        std::string stype = q.getstr();

        int istype = soldierMap[stype];

        INFO("update base_stronghold_generals set ["<<sgid<<":"<<pos<<"]:"<<istype);
        if (!q2.execute("update base_stronghold_generals set stype=" + boost::lexical_cast<string>(istype)
                    + " where stronghold=" + boost::lexical_cast<string>(sgid) + " and pos=" + boost::lexical_cast<string>(pos)))
        {
            INFO(q2.GetLastQuery()<<endl<<"error:"<<q2.GetError()<<",errno:"<<q2.GetErrno());
        }
    }
    q.free_result();
    return 0;   
}

int ConvertBaseStrongholdLoots()
{
    Database db("localhost", "c_user", "23rf234", db_name);//, CLIENT_INTERACTIVE);
    Query q(db);
    Query q2(db);

    std::map<std::string, int> Type;       //关卡类型
    Type["银币"] = 1;
    Type["物品"] = 2;
    Type["装备"] = 3;
    Type["英雄"] = 4;
    Type["阵型"] = 5;
    Type["技能"] = 6;
    Type["金币"] = 7;
    Type["世界银币"] = 8;
    q.execute("TRUNCATE table base_stronghold_loots");
    CHECK_DB_ERR(q);
    q.execute("insert into base_stronghold_loots (id,limits,counts,chance) (select id,limits,counts,100*chance from temp_base_stronghold_loots where 1)");
    CHECK_DB_ERR(q);
    
    q.get_result("select id,itemType,itemid from temp_base_stronghold_loots where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        //cout<<id<<endl;
        std::string itemType = q.getstr();
        std::string item = q.getstr();

        int type = Type[itemType];
        if (type == 0)
        {
            cout<<" error itemType id = "<<id<<endl;
            continue;
        }
        
        int item_id = 0;
        switch (type)
        {
            case 1:
                //银币无需处理
                item_id = 0;
                cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                break;
            case 2:
                //物品
                q2.get_result("select id from base_treasures where name='" + item + "'");
                CHECK_DB_ERR(q2);
                if (q2.fetch_row())
                {
                    item_id = q2.getval();
                    cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                }
                else
                {
                    item_id = -1;
                    cout<<"*****************error item id ,id ="<<id<<endl;
                }
                break;
            case 3:
                //装备
                q2.get_result("select id from base_equipts where name='" + item + "'");
                CHECK_DB_ERR(q2);
                if (q2.fetch_row())
                {
                    item_id = q2.getval();
                    cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                }
                else
                {
                    item_id = -1;
                    cout<<"*******************error item id ,id ="<<id<<endl;
                }
                break;
            case 4:
                //英雄
                q2.get_result("select gid from base_generals where name='" + item + "'");
                CHECK_DB_ERR(q2);
                if (q2.fetch_row())
                {
                    item_id = q2.getval();
                    cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                }
                else
                {
                    item_id = -1;
                    cout<<"*******************error item id ,id ="<<id<<endl;
                }
                break;
            case 5:
                //阵型
                q2.get_result("select type from base_zhens where name='" + item + "'");
                CHECK_DB_ERR(q2);
                if (q2.fetch_row())
                {
                    item_id = q2.getval();
                    cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                }
                else
                {
                    item_id = -1;
                    cout<<"*******************error item id ,id ="<<id<<endl;
                }
                break;
            case 6:
                //技能
                q2.get_result("select id from base_skills where name='" + item + "'");
                CHECK_DB_ERR(q2);
                if (q2.fetch_row())
                {
                    item_id = q2.getval();
                    cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                }
                else
                {
                    item_id = -1;
                    cout<<"********************error item id ,id ="<<id<<endl;
                }
                break;
            default:
                cout<<"********************error unknow type ,id ="<<id<<","<<type<<endl;
                break;
        }
        q2.free_result();
        if (!q2.execute("update base_stronghold_loots set itemType=" + boost::lexical_cast<string>(type)
                    + ",itemId=" + boost::lexical_cast<string>(item_id)
                    + " where id=" + boost::lexical_cast<string>(id)))
        {
            INFO(q2.GetLastQuery()<<endl<<"error:"<<q2.GetError()<<",errno:"<<q2.GetErrno());
        }
    }
    q.free_result();
    return 0;   
}

int ConvertBaseTasks()
{
    Database db("localhost", "c_user", "23rf234", db_name);//, CLIENT_INTERACTIVE);
    Query q(db);
    Query q2(db);

    std::map<std::string, int> Type;
    //1、关卡 2、主将等级 3、获得英雄 4、英雄等级 5、进入场景 6、进入地图 7、技能等级 8、阵型等级 9、购买兵器 10、刷新兵器 11、布阵操作 12、屯田操作 13、收藏游戏
    Type["关卡"] = 1;
    Type["主将等级"] = 2;
    Type["获得英雄"] = 3;
    Type["英雄等级"] = 4;
    Type["进入场景"] = 5;
    Type["进入地图"] = 6;
    Type["技能等级"] = 7;
    Type["阵型等级"] = 8;
    Type["购买兵器"] = 9;
    Type["刷新兵器"] = 10;
    Type["布阵操作"] = 11;
    Type["屯田操作"] = 12;
    Type["收藏游戏"] = 13;

    Type["探索操作"] = 14;
    Type["装备等级"] = 15;
    Type["技能研究"] = 16;
    Type["加入阵营"] = 17;
    Type["竞技战斗"] = 18;
    Type["状态刷新"] = 19;
    Type["银币数量"] = 20;
    Type["武器等级"] = 21;
    Type["道具"] = 22;
    Type["多英雄等级"] = 23;
    Type["冶炼操作"] = 24;
    Type["升级兵器"] = 25;

    std::map<std::string, int> xxType;       //关卡类型
    xxType["银币"] = 1;
    xxType["道具"] = 2;
    xxType["物品"] = 2;
    xxType["装备"] = 3;
    xxType["英雄"] = 4;
    xxType["阵型"] = 5;
    xxType["技能"] = 6;
    xxType["金币"] = 7;
    xxType["世界银币"] = 8;
    xxType["军令"] = 9;
    
    q.execute("TRUNCATE table base_tasks");
    CHECK_DB_ERR(q);
    q.execute("insert into base_tasks (id,title,memo,need1,need2,counts) (select id,title,memo,need1,need2,counts from temp_base_tasks where 1)");
    CHECK_DB_ERR(q);
    
    q.get_result("select id,type,itemType,itemId,need1 from temp_base_tasks where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        //cout<<id<<endl;
        std::string tasktype = q.getstr();
        std::string itemType = q.getstr();
        std::string item = q.getstr();
        std::string need1 = q.getstr();

        int ttype = Type[tasktype];
        if (ttype == 0)
        {
            cout<<" error task type "<<id<<endl;
            continue;
        }
        
        int type = xxType[itemType];
        if (type == 0)
        {
            cout<<" error itemType id = "<<id<<endl;
            continue;
        }
        
        int ineed1 = -1;
        if (ttype == 7)
        {
            q2.get_result("select id from base_skills where name='" + need1 + "'");
            CHECK_DB_ERR(q2);
            if (q2.fetch_row())
            {
                ineed1 = q2.getval();                
            }
            else
            {
                cout<<" error need 1 "<<need1<<endl;
            }
            q2.free_result();
        }
        else if (ttype == 8)
        {
            q2.get_result("select type from base_zhens where name='" + need1 + "'");
            CHECK_DB_ERR(q2);
            if (q2.fetch_row())
            {
                ineed1 = q2.getval();
            }
            else
            {
                cout<<"*******************error need 1 "<<need1<<endl;
            }
            q2.free_result();
        }
    
        int item_id = 0;
        switch (type)
        {
            case 1:
                //银币无需处理
                item_id = 0;
                cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                break;
            case 2:
                //物品
                q2.get_result("select id from base_treasures where name='" + item + "'");
                CHECK_DB_ERR(q2);
                if (q2.fetch_row())
                {
                    item_id = q2.getval();
                    cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                }
                else
                {
                    item_id = -1;
                    cout<<"*****************error item id ,id ="<<id<<endl;
                }
                break;
            case 3:
                //装备
                q2.get_result("select id from base_equipts where name='" + item + "'");
                CHECK_DB_ERR(q2);
                if (q2.fetch_row())
                {
                    item_id = q2.getval();
                    cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                }
                else
                {
                    item_id = -1;
                    cout<<"*******************error item id ,id ="<<id<<endl;
                }
                break;
            case 4:
                //英雄
                q2.get_result("select gid from base_generals where name='" + item + "'");
                CHECK_DB_ERR(q2);
                if (q2.fetch_row())
                {
                    item_id = q2.getval();
                    cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                }
                else
                {
                    item_id = -1;
                    cout<<"*******************error item id ,id ="<<id<<endl;
                }
                break;
            case 5:
                //阵型
                q2.get_result("select type from base_zhens where name='" + item + "'");
                CHECK_DB_ERR(q2);
                if (q2.fetch_row())
                {
                    item_id = q2.getval();
                    cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                }
                else
                {
                    item_id = -1;
                    cout<<"*******************error item id ,id ="<<id<<endl;
                }
                break;
            case 6:
                //技能
                q2.get_result("select id from base_skills where name='" + item + "'");
                CHECK_DB_ERR(q2);
                if (q2.fetch_row())
                {
                    item_id = q2.getval();
                    cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                }
                else
                {
                    item_id = -1;
                    cout<<"********************error item id ,id ="<<id<<endl;
                }
                break;
            case 7:
            case 9:
                item_id = 0;
                cout<<id<<"\t"<<type<<"\t"<<item_id<<endl;
                break;
            default:
                cout<<"********************error unknow type ,id ="<<id<<","<<type<<endl;
                break;
        }
        q2.free_result();
    
        std::string sql;
        if (ineed1 == -1)
        {
            sql = "update base_tasks set itemType=" + boost::lexical_cast<string>(type)
                    + ",itemId=" + boost::lexical_cast<string>(item_id)
                    + ",type=" + boost::lexical_cast<string>(ttype)
                    + " where id=" + boost::lexical_cast<string>(id);
        }
        else
        {
            sql = "update base_tasks set itemType=" + boost::lexical_cast<string>(type)
                    + ",itemId=" + boost::lexical_cast<string>(item_id)
                    + ",need1=" + boost::lexical_cast<string>(ineed1)
                    + ",type=" + boost::lexical_cast<string>(ttype)
                    + " where id=" + boost::lexical_cast<string>(id);
        }
        if (!q2.execute(sql))
        {
            INFO(q2.GetLastQuery()<<endl<<"error:"<<q2.GetError()<<",errno:"<<q2.GetErrno());
        }
    }
    q.free_result();
    return 0;   
}

void createTestAccount(const std::string& prefix, int from, int num)
{
    Database db("localhost", "c_user", "23rf234", db_name);//, CLIENT_INTERACTIVE);
    Query q(db);
    for (int i = from; i <= (from + num); ++i)
    {
        std::string account = prefix + boost::lexical_cast<string>(i);
        if (!q.execute("insert into temp_test_accounts (account,password) values ('" + account + "','111111')"))
        {
            INFO(q.GetLastQuery()<<endl<<"error:"<<q.GetError()<<",errno:"<<q.GetErrno());
        }
    }
    INFO("create test account success");
}

int main(int argc,char *argv[])
{
    if (argc < 2)
    {
        INFO("convert witch table?");
        return 0;
    }
    if (argc >= 3)
    {
        db_name = argv[2];
    }
 
    std::string tableName = argv[1];
    if (tableName == "base_soldiers")
    {
        ConvertBaseSoldiers();
    }
    else if (tableName == "base_generals")
    {
        ConvertBaseGenerals();
    }
    else if (tableName == "base_stronghold")
    {
        ConvertBaseStronghold();
    }
    else if (tableName == "base_stronghold_generals")
    {
        ConvertBaseStrongholdGenerals();
    }
    else if (tableName == "base_stronghold_loots")
    {
        ConvertBaseStrongholdLoots();
    }
    else if (tableName == "base_tasks")
    {
        ConvertBaseTasks();
    }
    else if (tableName == "createTestAccount")
    {
        std::string prefix = "robot";
        if (argc >= 4)
        {
            db_name = argv[3];
        }
        int from = 1, num = 5000;
        if (argc >= 5)
        {
            from = atoi(argv[4]);
        }
        if (argc >= 6)
        {
            num = atoi(argv[5]);
        }
        createTestAccount(prefix, from, num);
    }
    else
    {
        INFO("wrong table!");
        INFO("1");
        boost::shared_ptr<testSharePtr> tp;
        tp.reset(new testSharePtr(1));
        boost::shared_ptr<testSharePtr> tp2;
        tp2.reset(new testSharePtr(2));
        INFO("2");
        tp2 = tp;
        INFO("3");
        tp.reset();
        tp2.reset();
        
        INFO("######################");
        std::map<int,boost::shared_ptr<testSharePtr> > testMap;
        for (int i = 1; i <= 10; ++i)
        {
            testMap[i].reset(new testSharePtr(i));
        }
        INFO("@@@@@@@@@@@@@@@@@@@@@@@");
        std::map<int,boost::shared_ptr<testSharePtr> >::iterator it = testMap.find(4);
        if (it != testMap.end())
        {
            boost::shared_ptr<testSharePtr> tmp = it->second;
            (void)tmp;
        }
        INFO("$$$$$$$$$$$$$$$$$$$$$$$$1");
        testMap.erase(3);
        INFO("$$$$$$$$$$$$$$$$$$$$$$$$2");

        testMap[5].reset();
        if (testMap.find(5) != testMap.end())
        {
            INFO("have 5 !!!!!!!!");
        }
        INFO("$$$$$$$$$$$$$$$$$$$$$$$$3");
        testMap.clear();
        INFO("$$$$$$$$$$$$$$$$$$$$$$$$4");
        return 0;
    }
    return 0;
}

