
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

#define TO_STR(x) boost::lexical_cast<std::string>(x)

const int i_max_elite_id = 51;

const int i_max_map_id = 8;

const int elite_mapid[] =
{
    3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,
    8,8,8,8,8,8,8,8,8
};

void fixElite(Query& q, int cid)
{
    /*
        map3   1-6
        map
    */

    std::list<std::string> sqls;
    
    int map_count[7] = {0,0,0,0,0,0,0};

    int cur = 0;
    q.get_result("select mapid,eliteid,state,result from char_elite_tempo where cid=" + TO_STR(cid) + " and eliteid>0 and eliteid <=" + TO_STR(i_max_elite_id) + " order by eliteid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mapid = q.getval();
        int eliteid = q.getval();
        int state = q.getval();
        int result = q.getval();
        //cout<<eliteid<<endl;
        while (eliteid != (cur+1))
        {
            ++cur;
            sqls.push_back("insert into char_elite_tempo (cid,mapid,eliteid) values (" + TO_STR(cid) + "," + TO_STR(elite_mapid[cur-1])
                + "," + TO_STR(cur) + ")");            
        }

        if (eliteid >= 1 && eliteid <= i_max_elite_id)
        {
            if (mapid != elite_mapid[eliteid-1])
            {
                sqls.push_back("update char_elite_tempo set mapid=" + TO_STR(elite_mapid[eliteid-1]));
            }
        }
        ++cur;
    }
    q.free_result();

    while (cur < i_max_elite_id)
    {
        ++cur;
        sqls.push_back("insert into char_elite_tempo (cid,mapid,eliteid) values (" + TO_STR(cid) + "," + TO_STR(elite_mapid[cur-1])
                + "," + TO_STR(cur) + ")");        
    }

    int cur_map = 2;
    q.get_result("select mapid,reset_time from char_elite_map_tempo where cid=" + TO_STR(cid));
    while (q.fetch_row())
    {
        int mapid = q.getval();
        if (mapid < 3)
        {
            continue;
        }
        while (mapid != (cur_map+1))
        {
            ++cur_map;
            sqls.push_back("insert into char_elite_map_tempo (cid,mapid,reset_time) values (" + TO_STR(cid) + "," + TO_STR(cur_map) + ","
                + TO_STR(0) + ")");
            
        }
        ++cur_map;
    }
    while (cur_map < i_max_map_id)
    {
        ++cur_map;
        sqls.push_back("insert into char_elite_map_tempo (cid,mapid,reset_time) values (" + TO_STR(cid) + "," + TO_STR(cur_map) + ","
                + TO_STR(0) + ")");        
    }

    while (sqls.size() > 0)
    {
        std::string sql = sqls.front();
        q.execute(sql);
        CHECK_DB_ERR(q);
        sqls.pop_front();
    }
}

int fixElite2(Query& q, int cid)
{
    int min_lock = 0;
    q.get_result("select min(eliteid) from char_elite_tempo where cid=" + TO_STR(cid) + " and state='1' and eliteid>0 and eliteid <=" + TO_STR(i_max_elite_id));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        min_lock = q.getval();
    }
    q.free_result();

    if (min_lock > 1)
    {
        q.get_result("select state,result from char_elite_tempo where cid=" + TO_STR(cid) + " and eliteid=" + TO_STR(min_lock-1));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            int state = q.getval();
            int result = q.getval();
            q.free_result();
            if (state != 2 || result != 2)
            {
                q.execute("update char_elite_tempo set state=2,result=2 where cid=" + TO_STR(cid) + " and eliteid=" + TO_STR(min_lock));
                cout<<"\tfix "<<cid<<endl;
                CHECK_DB_ERR(q);
            }
        }
        else
        {
            q.free_result();
        }
    }
    else if (min_lock == 1)
    {
        q.execute("update char_elite_tempo set state=2,result=2 where cid=" + TO_STR(cid) + " and eliteid=1");
        cout<<"\tfix "<<cid<<endl;
        CHECK_DB_ERR(q);
    }
    return 0;
}

/*
int main(int argc,char *argv[])
{
    std::list<int> cids;
    if (argc < 2)
    {
        cout<<"db name ?"<<endl;
        return 0;
    }
    int fixtype = 0;
    if (argc > 3 && atoi(argv[3]) > 0)
    {
        fixtype = 1;
    }
    Database db("localhost", "c_user", "23rf234", argv[1]);//, CLIENT_INTERACTIVE);
    Query q(db);

    if (argc > 2 && atoi(argv[2]) > 0)
    {        
        //q.execute("delete from char_elite_tempo where eliteid>33");
        int cid = atoi(argv[2]);
        cout<<"cid "<<cid<<endl;
        cids.push_back(cid);
    }
    else
    {
        cout<<"all charactors"<<endl;
        q.get_result("select distinct cid from char_elite_tempo where 1");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            cids.push_back(q.getval());
        }
        q.free_result();
    }

    if (fixtype == 0)
    {
        cout<<"fix 1..."<<endl;
        for (std::list<int>::iterator it = cids.begin(); it != cids.end(); ++it)
        {
            fixElite(q, *it);
        }
    }
    else
    {
        cout<<"fix 2..."<<endl;
        for (std::list<int>::iterator it = cids.begin(); it != cids.end(); ++it)
        {
            fixElite2(q, *it);
        }
    }
    return 0;
}*/

void fixStronghold(Query& q, int cid, int mapid, int level)
{
    q.execute("delete from char_stronghold where cid=" + TO_STR(cid));
    CHECK_DB_ERR(q);

    int stronghold = 1;
    if (level > 1)
    {
        stronghold = 2 * (level - 1);
    }
    cout<<"fixStronghold "<<cid<<",mapid="<<mapid<<",level:"<<level<<",stronghold:"<<stronghold<<endl;

    for (int i = 1; i < mapid; ++i)
    {
        q.execute("insert into char_stronghold (cid,mapid,stageid,pos1,pos2,pos3,pos4,pos5,pos6,pos7,pos8) values (" + TO_STR(cid) +
         "," + TO_STR(i) + "," + TO_STR(1) + ",1,1,1,1,1,1,1,1)"    );
        CHECK_DB_ERR(q);

        q.execute("insert into char_stronghold (cid,mapid,stageid,pos1,pos2,pos3,pos4,pos5,pos6,pos7,pos8) values (" + TO_STR(cid) +
         "," + TO_STR(i) + "," + TO_STR(2) + ",1,1,1,1,1,1,1,1)"    );
        CHECK_DB_ERR(q);

        q.execute("insert into char_stronghold (cid,mapid,stageid,pos1,pos2,pos3,pos4,pos5,pos6,pos7,pos8) values (" + TO_STR(cid) +
         "," + TO_STR(i) + "," + TO_STR(3) + ",1,1,1,1,1,1,1,1)"    );
        CHECK_DB_ERR(q);
    }

    int tmp = stronghold % 24;
    if (tmp == 0)
    {
        tmp = 24;
    }
    int stage = tmp / 8;
    int shold = tmp % 8;
    if (shold == 0)
    {
        shold = 8;
    }
    else
    {
        ++stage;
    }
    cout<<"statge:"<<stage<<","<<shold<<endl;

    for (int s = 1; s <= 3; ++s)
    {
        if (s < stage)
        {
            q.execute("insert into char_stronghold (cid,mapid,stageid,pos1,pos2,pos3,pos4,pos5,pos6,pos7,pos8) values (" + TO_STR(cid) +
                 "," + TO_STR(mapid) + "," + TO_STR(s) + ",1,1,1,1,1,1,1,1)"    );
                CHECK_DB_ERR(q);
        }
        else if (s == stage)
        {
            std::string sql = "insert into char_stronghold (cid,mapid,stageid,pos1,pos2,pos3,pos4,pos5,pos6,pos7,pos8) values (" + TO_STR(cid) +
             "," + TO_STR(mapid) + "," + TO_STR(s);
            
            for (int x = 1; x <= 8; ++x)
            {
                if (x <= shold)
                {
                    sql += ",1";
                }
                else if (x == (shold + 1))
                {
                    sql += ",0";
                }
                else
                {
                    sql += ",-1";
                }
            }
            sql += ")";
            q.execute(sql);
            CHECK_DB_ERR(q);
        }
        else
        {
            if (shold == 8 && s == (stage+1))
            {
                q.execute("insert into char_stronghold (cid,mapid,stageid,pos1,pos2,pos3,pos4,pos5,pos6,pos7,pos8) values (" + TO_STR(cid) +
                 "," + TO_STR(mapid) + "," + TO_STR(s) + ",0,-1,-1,-1,-1,-1,-1,-1)"    );
            }
            else
            {
                q.execute("insert into char_stronghold (cid,mapid,stageid,pos1,pos2,pos3,pos4,pos5,pos6,pos7,pos8) values (" + TO_STR(cid) +
                 "," + TO_STR(mapid) + "," + TO_STR(s) + ",-1,-1,-1,-1,-1,-1,-1,-1)"    );
            }
            CHECK_DB_ERR(q);
        }
    }
}

//ÐÞ¸´¹Ø¿¨Êý¾Ý
int main(int argc,char *argv[])
{
    std::list<int> cids;
    std::list<int> mapids;
    std::list<int> levels;
    if (argc < 2)
    {
        cout<<"db name ?"<<endl;
        return 0;
    }
    Database db("localhost", "c_user", "23rf234", argv[1]);//, CLIENT_INTERACTIVE);
    Query q(db);

    std::string sql = "select c.id,c.level,d.mapid from charactors as c left join char_data as d on c.id=d.cid where 1";
    if (argc > 2 && atoi(argv[2]) > 0)
    {
        int cid = atoi(argv[2]);
        cout<<"fix cid "<<cid<<endl;
        sql = "select c.id,c.level,d.mapid from charactors as c left join char_data as d on c.id=d.cid where c.id=" + TO_STR(cid);
    }
    else
    {
        cout<<"fix all charactors"<<endl;
        sql = "select c.id,c.level,d.mapid from charactors as c left join char_data as d on c.id=d.cid where 1";
        
    }

    q.get_result(sql);
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        cids.push_back(q.getval());
        levels.push_back(q.getval());
        mapids.push_back(q.getval());
    }
    q.free_result();

    std::list<int>::iterator it = cids.begin();
    std::list<int>::iterator it_mapid = mapids.begin();
    std::list<int>::iterator it_level = levels.begin();
    while (it != cids.end())
    {
        fixStronghold(q, *it, *it_mapid, *it_level);

        ++it;
        ++it_mapid;
        ++it_level;
    }
    return 0;
}

