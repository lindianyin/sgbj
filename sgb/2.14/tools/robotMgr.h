#pragma once

#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include "json_spirit_value.h"
#include <string>
#include <boost/asio.hpp>
#include "testChar.h"
#include <boost/thread.hpp>

using namespace std;

using boost::asio::ip::tcp;

class io_service_pool
    : private boost::noncopyable
{
public:
    explicit io_service_pool(std::size_t pool_size);

    void run();
    void stop();

    boost::asio::io_service& get_io_service();

private:
    typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
    typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

    std::vector<io_service_ptr> io_services_;
    std::vector<work_ptr> work_;
    std::size_t next_io_service_;
};

class robotMgr
{
public:
    robotMgr(const char*, const char* ,const std::string& account_prefix, int from , int to);
    boost::asio::io_service& get_io_service();
    int get_union_id() { return m_union_id; }
    std::string get_auth_code() { return m_auth_code; }
    tcp::resolver::iterator getServerEndpoint() { return m_resolver_itr; }
    void wookLoop();

    std::string random_name(int spic);
    int random_spic();
    std::string random_hello_msg();

    void run();

    std::string gethost() {return m_host;}
    int getport() {return m_port;}

private:
    std::string m_account_prefix;
    int m_first_account;
    int m_last_account;
    std::string m_host;
    int m_port;

    int m_current_account;

    int m_union_id;

    std::string m_auth_code;

    io_service_pool io_service_pool_;

    tcp::resolver::iterator m_resolver_itr;
    tcp::resolver m_resolver;
    tcp::resolver::query m_resolver_query;

    std::map<std::string, int> m_accounts;

    std::map<std::string, boost::shared_ptr<testRobot> > m_online_robots;

    boost::shared_ptr<boost::thread> m_io_thread;
    boost::shared_ptr<boost::thread> m_work_thread;

    std::vector<std::string> m_random_man_names;
    std::vector<std::string> m_random_woman_names;
    std::vector<std::string> m_random_last_names;

    std::vector<std::string> m_random_hello;
};

