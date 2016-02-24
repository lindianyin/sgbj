/*
 *
 * Copyright (C) 2009 jack.wgm, microcai.
 * For conditions of distribution and use, see copyright notice 
 * in (http://code.google.com/p/netsever/source/browse/trunk/COPYING)
 *
 * Author: jack.wgm
 * Email:  jack.wgm@gmail.com
 */

#ifndef NET_H__
#define NET_H__

#include "config.h"

#include "jobqueue.hpp"
#include "BasePacket.h"
#include "defs.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include "json_spirit_value.h"
#include <string>
using namespace std;
using namespace json_spirit;

using boost::asio::ip::tcp;

class OnlineUser;
enum {
    STATE_CONNECTED = 0,
    STATE_AUTHED = 1,       //已经验证
    STATE_DISCONNECT = 2
};

#define NET_HEAD_SIZE       16
#define NET_MAX_BUFF_SIZE 4096

namespace net 
{
    class server;
    class session;
    class io_service_pool;

    typedef boost::shared_ptr<session> session_ptr;

    typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
    typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

    enum httpRequestState
    {
        sw_start = 0,
        sw_req_url_CR,
        sw_req_url_LF,
        sw_end_CR1,
        sw_end_LF1,
        sw_end_CR2,
        sw_end_LF2
    };
    struct httpRequest
    {
        int _type;    // 1 get 2 post
        char _req_url[256];
        int _pos;
        int _state;
        bool _bComplete;
    };

    struct recvbuff
    {
        char data_[NET_HEAD_SIZE + NET_MAX_BUFF_SIZE];
        size_t body_length_;
        char state;     // 0 未收到固定字节  1 收到固定字节  2 收到包头 3 收到整包
        char* point1;
        char* point2;
        int encryption;    // 0,1,2
        recvbuff();
        void clear();
        int check(session& se);     // 0 继续 1 继续收 2 断开
        void adddata(void* ad, size_t len);
    };

//////////////////////////////////////////////////////////////////////////
    class loginmessage
    {
    public:
        loginmessage();
        loginmessage(const loginmessage& msg);

        void setsession(const session_ptr& _session);
        void getsession(session_ptr& _session);
        loginmessage& operator =(loginmessage &msg);
        std::string username;
        std::string password;

        std::string _qid;
        std::string _qname;
        std::string _server_id;
        time_t _time;
        std::string _sign;
        int _isAdult;        /*    0    用户未填写实名制信息
                                1    用户填写过实名制信息，且大于18岁
                                2    用户填写过实名制信息，但是小于18岁)*/

        int _union_id;

#ifdef QQ_PLAT
        int _is_qq_yellow;            //是否qq黄钻
        int _qq_yellow_level;        //qq黄钻等级
        int _is_qq_year_yellow;    //是否qq年费黄钻
        std::string _feedid;                //传播id
        std::string _iopenid;                //推荐好友id
        std::string _login_str1;    //登陆信息字段
        std::string _login_str2;    //登陆信息字段
#endif

        std::string _extra1;
        std::string _extra2;
    private:
        boost::weak_ptr<session> session_;
    };

    class chatmessage
    {
    public:
        chatmessage();
        chatmessage(const chatmessage& msg);
        void setsession(const session_ptr& _session);
        void getsession(session_ptr& _session);
        chatmessage& operator =(chatmessage &msg);
        int channel_type_;
        int chat_type_;
        std::string towho_;
        std::string msg_;
    private:
        boost::weak_ptr<session> session_;
    };

    class actionmessage
    {
    public:
        actionmessage();
        actionmessage(json_spirit::mObject& robj, int from);
        json_spirit::mObject& getRecvObj() {return recvObj;};
        actionmessage& operator =(actionmessage &msg);
        void setsession(const session_ptr& _session);
        void getsession(session_ptr& _session);
        std::string cmd;
        int from() {return m_from;};

        uint64_t _recv_time;
        uint64_t _get_time;

    private:
        boost::weak_ptr<session> session_;
        json_spirit::mObject recvObj;
        int m_from;
    };

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
    class session
        : public boost::enable_shared_from_this<session>
    {
    public:
        session(boost::asio::io_service& io_service, jobqueue<actionmessage>& jobwork, jobqueue<loginmessage>& jobwork2);
        ~session();

        tcp::socket& socket();
        bool is_using();
        void is_using(bool b);

        void start();
        void send(const std::string& msg, bool pack = true);
        int retry()
        {
            return m_retry;
        }
        void retry(int retry)
        {
            m_retry = retry;
        }
        int state()
        {
            return m_state;
        }
        void state(int state)
        {
            m_state = state;
        }
        void set_port(int port)
        {
            m_port = port;
        }
        boost::shared_ptr<OnlineUser> user()
        {
            return m_ponlineuser;
        }
        void user(boost::shared_ptr<OnlineUser>& puser)
        {
            if (NULL == puser)
            {
                m_ponlineuser.reset();
            }
            else
            {
                m_ponlineuser.reset();
                m_ponlineuser = puser;
            }
            //std::cout<<"session user count "<<puser.use_count()<<std::endl;
        }
        time_t connect_time()
        {
            return m_connect_time;
        }
        time_t last_act_time()
        {
            return m_last_action_time;
        }
        time_t last_beat_time()
        {
            return m_last_beat_time;
        }
        std::string remote_ip() {return _remote_ip;}
        void _closeconnect(bool kickoff = false);     //是否被踢后主动断开连接
        void closeconnect(bool kickoff = false);     //是否被踢后主动断开连接
        void handle_read_login_or_policy(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
        void try_read();
        int process_msg(json_spirit::mValue& value);
        int read_http_request();
        void handle_read_http_request(const boost::system::error_code& error, size_t bytes_transferred);

        bool is_debug_;
        static uint64_t refs();
    private:
        static volatile uint64_t _refs;
        void _send(const std::string& msg, bool pack = true);
        void do_send(const std::string& buff, bool pack = true);
        int m_state;    //会话状态
        int m_retry;    //重试次数
        int m_port;

        int m_log_cmds;
        int m_action_cmds;
        bool m_too_many_cmds;

        time_t m_last_log_time;
        time_t m_last_action_time;
        time_t m_last_beat_time;
        time_t m_connect_time;

        char tempbuff[NET_MAX_BUFF_SIZE/2];
        recvbuff recv;
        httpRequest _httpRequst;
        char send_data[NET_HEAD_SIZE+50*NET_MAX_BUFF_SIZE + 1000];
        unsigned short slen;

        tcp::socket socket_;
        std::string _remote_ip;
        boost::shared_ptr<OnlineUser> m_ponlineuser;

        jobqueue<actionmessage>& actionwork_;
        jobqueue<loginmessage>& loginwork_;

        std::list<std::string> m_send_que;
        boost::mutex session_mutex_;
        bool using_;
        bool wait_close_;
        bool is_sending_;
        bool is_reading_;
        boost::asio::io_service& io_service_;
    #ifdef USE_STRAND
        boost::asio::io_service::strand strand_;
    #endif // USE_STRAND
    };

//////////////////////////////////////////////////////////////////////////
    class server
    {
    public:
        server(short port, jobqueue<actionmessage>& actionwork, jobqueue<loginmessage>& loginwork, std::size_t io_service_pool_size = 4, short port2 = 0);

        void run();
        void stop();
        void handle_accept(session_ptr new_session, tcp::acceptor* acp,
            const boost::system::error_code& error);
#ifdef USE_POOL    
        void check_sessions();
#endif

    private:
        jobqueue<actionmessage>& actionwork_;
        jobqueue<loginmessage>& loginwork_;

        io_service_pool io_service_pool_;
        tcp::acceptor acceptor_;
        tcp::acceptor acceptor2_;
        std::vector<session_ptr> session_pool_;
    };

    class write_handler
    {
    public:
        write_handler(boost::shared_ptr<session> psn)
        {
            pthis = psn;
        }
        boost::shared_ptr<session> pthis;
        void operator()(const boost::system::error_code& error, size_t bytes_transferred)
        {
            pthis->handle_write(error, bytes_transferred);
        }
    };

    //标准的flash server的请求：<policy-file-request/>\0，
    //这里需要以\0结尾，一共23个字节。这里的\0是指一个accii码为0的符号，只占用一个字节。
    const int policy_file_request_len = 23;

    struct mySession : public boost::enable_shared_from_this<mySession>
    {
        mySession(boost::asio::io_service& ios)
        :strand_(ios)
        ,m_socket(ios)
        ,state(0)
        {
            memset(tempbuff, 0, policy_file_request_len);
        }
        boost::asio::io_service::strand strand_;
        tcp::socket m_socket;
        int state;    // 0:初始状态  1:等待数据  2:发送中
        char tempbuff[policy_file_request_len];
        void close_session();
        void handle_read_request(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_send_policy(const boost::system::error_code& error, size_t bytes_transferred);
        void start_session();

        static char policy_file_buffer[512];
        static int policy_file_buffer_len;
    };

    class policyServer
    {
    public:
        policyServer();
        ~policyServer();
        void run();
        void stop();
        void start_accept();
        void handle_accept(boost::shared_ptr<mySession> session_,
                            const boost::system::error_code& error);    

        boost::shared_ptr<mySession> get_session();
    private:
        io_service_ptr io_service_;
        work_ptr work_;
        boost::shared_ptr<tcp::acceptor> acceptor_;

        std::list<boost::shared_ptr<mySession> > m_sessions;
    };

}

#endif // NET_H__

