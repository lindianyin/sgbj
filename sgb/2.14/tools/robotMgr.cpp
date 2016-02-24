
#include "robotMgr.h"
#include <iostream>
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include<boost/tokenizer.hpp>

using json_spirit::Pair;

io_service_pool::io_service_pool(std::size_t pool_size)
: next_io_service_(0)
{
    if (pool_size == 0)
        throw std::runtime_error("io_service_pool size is 0");

    for (std::size_t i = 0; i < pool_size; ++i)
    {
        io_service_ptr io_service(new boost::asio::io_service);
        work_ptr work(new boost::asio::io_service::work(*io_service));
        io_services_.push_back(io_service);
        work_.push_back(work);
    }
}

void io_service_pool::run()
{
    std::vector<boost::shared_ptr<boost::thread> > threads;
    for (std::size_t i = 0; i < io_services_.size(); ++i)
    {
        boost::shared_ptr<boost::thread> thread(new boost::thread(
            boost::bind(&boost::asio::io_service::run, io_services_[i])));
        threads.push_back(thread);
    }

    for (std::size_t i = 0; i < threads.size(); ++i)
    {
        threads[i]->join();
        cout<<" ************************* io_service_pool::run(),"<<threads[i]->get_id()<<" *************************  "<<endl;
    }
}

void io_service_pool::stop()
{
    for (std::size_t i = 0; i < io_services_.size(); ++i)
        io_services_[i]->stop();
}

boost::asio::io_service& io_service_pool::get_io_service()
{
    boost::asio::io_service& io_service = *io_services_[next_io_service_];
    ++next_io_service_;
    if (next_io_service_ == io_services_.size())
        next_io_service_ = 0;
    return io_service;
}

extern int my_random(int, int);

robotMgr::robotMgr(const char* host, const char* port, const std::string& account_prefix, int from , int to)
:m_account_prefix(account_prefix)
,m_first_account(from)
,m_last_account(to)
,io_service_pool_(1)
,m_resolver(io_service_pool_.get_io_service())
,m_resolver_query(host, port)
{
    m_host = host;
    m_port = atoi(port);
    m_current_account = from;
    m_union_id = 10000;
    m_auth_code = "HYpHnt49dRvTSbBQ";

    Database db("localhost", "c_user", "23rf234", "robotMgr");
    Query q(db);

    q.get_result("select union_id,`key` from `keys` where enable='1'");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_union_id = q.getval();
        m_auth_code = q.getstr();

        cout<<"union_id : "<<m_union_id<<endl;
        cout<<"key : "<<m_auth_code<<endl;
    }
    q.free_result();

    q.get_result("select name from last_names where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        std::string name = q.getstr();
        m_random_last_names.push_back(name);
    }
    q.free_result();
    std::cout<<"total last names "<<m_random_last_names.size()<<endl;

    q.get_result("select name from woman_names where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        std::string name = q.getstr();
        m_random_woman_names.push_back(name);
    }
    q.free_result();
    std::cout<<"total woman's names "<<m_random_woman_names.size()<<endl;

    q.get_result("select name from man_names where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        std::string name = q.getstr();
        m_random_man_names.push_back(name);
    }
    q.free_result();
    std::cout<<"total man's names "<<m_random_man_names.size()<<endl;

    q.get_result("select type,msg from messages where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        std::string type = q.getstr();
        std::string msg = q.getstr();

        if ("say_hello" == type)
        {
            using namespace boost;
            typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
            boost::char_separator<char> sep("|");
            tokenizer tok(msg, sep);
            tokenizer::iterator it = tok.begin();
            while (it != tok.end())
            {
                json_spirit::Object obj;
                //{"m":"hi","cmd":"chat","ctype":2}
                obj.push_back( Pair("cmd", "chat") );
                obj.push_back( Pair("ctype", 2) );
                obj.push_back( Pair("m", *it) );
                m_random_hello.push_back(json_spirit::write(obj));
                ++it;
            }
        }
    }
    q.free_result();
}

boost::asio::io_service& robotMgr::get_io_service()
{
    return io_service_pool_.get_io_service();
}

std::string robotMgr::random_name(int spic)
{    
    int index = my_random(0, m_random_last_names.size() - 1);
    std::string name = m_random_last_names[index];
    if (spic % 2 == 1)
    {
        //ÄÐÐÔ
        index = my_random(0, m_random_man_names.size() - 1);
        name += m_random_man_names[index];
    }
    else
    {
        index = my_random(0, m_random_woman_names.size() - 1);
        name += m_random_woman_names[index];
    }
    return name;
}

std::string robotMgr::random_hello_msg()
{
    int index = my_random(0, m_random_hello.size() - 1);
    return m_random_hello[index];
}

int robotMgr::random_spic()
{
    return my_random(1, 8);
}

void robotMgr::wookLoop()
{
    std::cout<<"run wookloop()...)"<<std::endl;
    
    m_resolver_itr = m_resolver.resolve(m_resolver_query);
    {
        tcp::resolver::iterator iter = m_resolver_itr;
        tcp::resolver::iterator end;
        while (iter != end)
        {
            tcp::endpoint endpoint = *iter++;
            std::cout << endpoint << std::endl;
        }
    }
    std::cout<<"------------"<<endl;

    time_t tnow = time(NULL);
    time_t tstart = tnow;
    uint64_t loop = 0;
    std::map<int,int> tongji;
    std::map<int,int> level_tongji;

    while (true)
    {
        ++loop;
        tnow = time(NULL);

        if (10*(tnow - tstart) >= m_online_robots.size() && (m_online_robots.size() - tongji[7]) < 10)
        {
            if (m_current_account < m_last_account)
            {
                std::string account = m_account_prefix + LEX_CAST_STR(m_current_account);
                boost::shared_ptr<testRobot> new_rob(new testRobot(*this, account));
                m_online_robots[account] = new_rob;
                new_rob->postStart();
                ++m_current_account;
            }
        }

        for (std::map<std::string, boost::shared_ptr<testRobot> >::iterator it = m_online_robots.begin();
             it != m_online_robots.end(); ++it)
        {
            if (it->second->lastHeartbeat() <= (tnow-3))
            {
                it->second->postHeartbeat(tnow, 0);
            }
        }

        //if (loop % 300 == 0)
        {
            level_tongji.clear();
            tongji.clear();
            for (std::map<std::string, boost::shared_ptr<testRobot> >::iterator it = m_online_robots.begin();
             it != m_online_robots.end(); ++it)
            {
                int state = it->second->getState();
                if (CHAR_STATE_CHAR_LOGINED == state)
                {
                    ++level_tongji[it->second->getLevel()];
                }
                ++tongji[state];
            }
             if (loop % 300 == 0)
             {
                std::cout<<"--------------------------------------\n";
                std::cout<<"total "<<m_online_robots.size()<<endl;
                for (std::map<int,int>::iterator it = tongji.begin(); it != tongji.end(); ++it)
                {                
                    std::cout<<"state "<<it->first<<" count "<<it->second<<std::endl;
                }
                std::cout<<"\t\t\t<><><><><><><><><><><><>"<<std::endl;
                for (std::map<int,int>::iterator it = level_tongji.begin(); it != level_tongji.end(); ++it)
                {                
                    std::cout<<"level "<<it->first<<" count "<<it->second<<std::endl;
                }
                std::cout<<"--------------------------------------\n";
             }
        }
        usleep(10000);
    }
}

void robotMgr::run()
{
    for (int i = 1; i <= 100; ++i)
    {
        std::cout<<my_random(1,8)<<",";
        if (i % 10 == 0)
        {
            std::cout<<endl;
        }
    }
    m_io_thread.reset(new boost::thread(boost::bind(&io_service_pool::run, &io_service_pool_)));
    m_work_thread.reset(new boost::thread(boost::bind(&robotMgr::wookLoop, this)));

    m_io_thread->join();
    m_work_thread->join();
}

void init_random_seed()
{
    srand((int)time(0));
}

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        cout<<"xxx host port counts"<<endl;
        return -1;
    }

    init_random_seed();
    
    int totals = atoi(argv[3]);

    if (totals < 5)
    {
        //g_debug = 1;
    }

    int from = 1;
    if (argc >= 5)
    {
        from = atoi(argv[4]);
    }

    robotMgr mgr(argv[1], argv[2], "sgbtest", from, from+totals);
    mgr.run();

    return 0;
}

