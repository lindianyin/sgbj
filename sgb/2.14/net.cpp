
#include "stdafx.h"

#ifndef _WINDOWS
#include <execinfo.h>
#endif

#include "inc.h"
#include "net.h"
#include <boost/asio/buffer.hpp>
#include "json_spirit.h"
#include "utils_all.h"
#include "data.h"
#include <syslog.h>

//#define INFO(x) cout<<x

#define INFO(x)

#if 0
#define DUMP(x, len) for (size_t i = 0; i < len; ++i)\
{\
    char* p = static_cast<char*>(x + i);\
    cout<<"0x"<<hex<<(int)(*p)<<",";\
}\
cout<<endl
#else
#define DUMP(x,len)
#endif

#define NET_PACK_START_STRING "WUDONG!!"

#define MAX_CONNECT_EVERY_IP 200
#define MAX_SESSITONS 6000

static std::map<std::string,int> g_ip_connects;
static std::map<std::string,int> g_free_connects;
static std::map<std::string,int> g_filter_ip;

extern volatile int g_print_debug_info;

int add_ip_connect(const std::string& ip)
{
    if (g_free_connects.find(ip) != g_free_connects.end()
        || g_ip_connects[ip] < MAX_CONNECT_EVERY_IP)
    {
        g_ip_connects[ip] = g_ip_connects[ip] + 1;
        return g_ip_connects[ip];
    }
    else
    {
        if (g_filter_ip.find(ip) != g_filter_ip.end())
        {
            ++g_filter_ip[ip];
        }
        else
        {
            g_filter_ip[ip] = 1;
            //syslog
            syslog (LOG_ERR, "ip %s too mush connects.", ip.c_str());
        }
        return -1;
    }
}

void sub_ip_connect(const std::string& ip)
{
    if (g_ip_connects[ip] > 0)
    {
        g_ip_connects[ip] = g_ip_connects[ip] - 1;
    }
}

namespace net 
{
//////////////////////////////////////////////////////////////////////////
    recvbuff::recvbuff()
    {
        clear();
    };
    void recvbuff::clear()
    {
        body_length_ = 0;
        point1 = data_;
        point2 = data_;
        state = 0;
        encryption = 0;
    };

    void recvbuff::adddata(void* ad, size_t len)
    {
        #if 0
        char* pc = static_cast<char*>(ad);
        INFO("recv buff add "<<len<<endl);
        DUMP(pc, len);
        #endif
        //剩余buff不够了
        if (len > size_t(data_ + NET_HEAD_SIZE + NET_MAX_BUFF_SIZE - point2))
        {
            INFO("**************move buff*********************"<<endl);
            size_t len_now = size_t(point2 - point1);
            //内存可能重叠
            memmove(data_, point1, len_now);
            point1 = data_;
            point2 = data_ + len_now;
            memset(point2, 0, data_ + NET_HEAD_SIZE + NET_MAX_BUFF_SIZE - point2);
            //空间还是不够
            if (len > size_t(data_ + NET_HEAD_SIZE + NET_MAX_BUFF_SIZE - point2))
            {
                syslog(LOG_ERR, "recv buff full.%d,%d", len_now, len);
                int old = encryption;
                clear();
                encryption = old;
            }
        }
        memcpy(point2, ad, len);
        point2 += len;
    }
    // 0 继续 1 继续收 -1 断开
    int recvbuff::check(session& se)
    {
        switch (state)
        {
            case 0:     //还未收到固定开始
            {
                    char* ps = strstr(point1, NET_PACK_START_STRING);
                    if (NULL == ps || ps >= point2)
                    {
                        //找不到开始，把前面的全扔了，剩下8个字节

                        //如果超过8个
                        if (point2 - point1 > 8)
                        {
                            memmove(data_, point2-8, 8);
                            point1 = data_;
                            point2 = data_ + 8;
                            return 1;
                        }
                        else
                        {
                            //没超过8个就不用处理
                            return 1;
                        }
                    }
                    else
                    {
                        //找到了修改状态
                        state = 1;
                        point1 = ps;
                        //INFO("find start!"<<endl);
                        return 0;
                    }
                break;
            }
            case 1: //已经收到固定开始，正在收包头
            {
                //收到包头了
                size_t len = point2 - point1;
                if (len >= (NET_HEAD_SIZE + 8))
                {
                    #if 0
                    INFO("find head!");
                    DUMP(point1+8,2);
                    #endif
                    body_length_ = ntohs(*((unsigned short*)(point1+8)));
                    if (encryption)
                    {                        
                        body_length_ /= encryption;
                    }
                    //INFO(",len="<<dec<<body_length_<<endl);
                    if (body_length_ > NET_MAX_BUFF_SIZE)
                    {
                        state = 0;
                        point1 = data_;
                        point2 = data_;
                        return -1;
                    }
                    state = 2;
                    return 0;    //继续处理
                }
                else
                {
                    //INFO("<head len"<<endl);
                    //不足长度，等着收包继续处理
                    return 1;
                }
                break;
            }
            case 2:
                //收到整个包了
                if (size_t(point2 - point1) >= (NET_HEAD_SIZE + 8 + body_length_))
                {
                    std::string s(point1 + NET_HEAD_SIZE + 8);
                    point1 += (NET_HEAD_SIZE + 8 + body_length_);
                    json_spirit::mValue value;
                    json_spirit::read(s, value);
                    state = 0;
                    INFO("recv a pack! from "<<se.remote_ip()<<endl<<s<<endl<<"pack end!"<<endl);
                    //处理消息
                    return se.process_msg(value);
                }
                else
                {
                    //INFO("< bodylen"<<body_length_<<endl);
                    return 1;
                }
                break;
        }
        return -1;
    }

//////////////////////////////////////////////////////////////////////////
    loginmessage::loginmessage() 
    {
        username = "";
        password = "";
        _qid = "";
        _qname = "";
        _server_id = "";
        _time = 0;
        _sign = "";
        _isAdult = 0;
        _union_id = 0;
        _extra1 = "";
        _extra2 = "";
#ifdef QQ_PLAT
        _is_qq_year_yellow = 0;
        _qq_yellow_level = 0;
        _is_qq_yellow = 0;
        _iopenid = "";
        _feedid = "0";
        _login_str1 = "";
        _login_str2 = "";
#endif
    }
    loginmessage::loginmessage(const loginmessage& msg)
    {
         session_ = msg.session_;
         username = msg.username;
         password = msg.password;

         _qid = msg._qid;
         _qname = msg._qname;
         _server_id = msg._server_id;
         _time = msg._time;
         _sign = msg._sign;
         _isAdult = msg._isAdult;
         _union_id = msg._union_id;
         _extra1 = msg._extra1;
         _extra2 = msg._extra2;
#ifdef QQ_PLAT
         _is_qq_year_yellow = msg._is_qq_year_yellow;
         _is_qq_yellow = msg._is_qq_yellow;
         _qq_yellow_level = msg._qq_yellow_level;
         _iopenid = msg._iopenid;
         _feedid = msg._feedid;
         _login_str1 = msg._login_str1;
         _login_str2 = msg._login_str2;
#endif
    }
    void loginmessage::setsession(const session_ptr& _session)
    {
        session_ = _session;
    }
     void loginmessage::getsession(session_ptr& _session)
    {
        _session = session_.lock();
    }

    loginmessage& loginmessage::operator =(loginmessage &msg)
    {
        session_ = msg.session_;
        username = msg.username;
        password = msg.password;

        _qid = msg._qid;
        _qname = msg._qname;
        _server_id = msg._server_id;
        _time = msg._time;
        _sign = msg._sign;
        _isAdult = msg._isAdult;
        _union_id = msg._union_id;
        _extra1 = msg._extra1;
        _extra2 = msg._extra2;
#ifdef QQ_PLAT
        _is_qq_year_yellow = msg._is_qq_year_yellow;
         _is_qq_yellow = msg._is_qq_yellow;
         _qq_yellow_level = msg._qq_yellow_level;
         _iopenid = msg._iopenid;
         _feedid = msg._feedid;
         _login_str1 = msg._login_str1;
         _login_str2 = msg._login_str2;
#endif
        return (*this);
    }
//////////////////////////////////////////////////////////////////////////
    actionmessage::actionmessage() 
    {
        m_from = 0;
        cmd = "";
#ifdef DEBUG_PER
        _recv_time = splsTimeStamp();
#endif
    }

    actionmessage::actionmessage(json_spirit::mObject& robj, int from = 0) 
    :recvObj(robj)
    {
        cmd = "";
        m_from = from;
        READ_STR_FROM_MOBJ(cmd, robj, "cmd");
#ifdef DEBUG_PER
        _recv_time = splsTimeStamp();
#endif
    }
    void actionmessage::setsession(const session_ptr& _session)
    {
        session_ = _session;
    }
    void actionmessage::getsession(session_ptr& _session)
    {
        _session = session_.lock();
    }
    actionmessage& actionmessage::operator =(actionmessage &msg)
    {
        session_ = msg.session_;
        cmd = msg.cmd;
        recvObj = msg.recvObj;
        m_from = msg.m_from;
        return (*this);
    }

    //////////////////////////////////////////////////////////////////////////
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
    volatile uint64_t session::_refs;
//////////////////////////////////////////////////////////////////////////
    session::session(boost::asio::io_service& io_service, jobqueue<actionmessage>& actionwork, jobqueue<loginmessage>& loginwork)
        : socket_(io_service)
        , actionwork_(actionwork)
        , loginwork_(loginwork)
        , using_(false)
        , wait_close_(false)
        , is_sending_(false)
        , is_reading_(false)
        , io_service_(io_service)
    #ifdef USE_STRAND
        , strand_(io_service)
    #endif // USE_STRAND
    {
        ++session::_refs;
        m_port = 0;
        slen = 0;
        memcpy(send_data, NET_PACK_START_STRING, 8);
        send_data[11] = 1;
        send_data[12] = 1;
        m_too_many_cmds = false;
    }

    session::~session()
    {
        --session::_refs;
    }

    uint64_t session::refs()
    {
        return session::_refs;
    }

    tcp::socket& session::socket()
    {
        return socket_;
    }

    bool session::is_using() 
    {
        // boost::mutex::scoped_lock lock(using_mutex_);
        return using_;
    };

    void session::is_using(bool b)
    {
        // boost::mutex::scoped_lock lock(using_mutex_);
        if (!b)
        {
            if (_remote_ip != "")
            {
                sub_ip_connect(_remote_ip);
            }
        }
        using_ = b;
    }

    void session::start()
    {
        m_log_cmds = 0;
        m_action_cmds = 0;
        m_last_log_time = 0;
        m_last_action_time = 0;
        m_connect_time = time(NULL);
        m_too_many_cmds = false;
        is_using(true);
        is_sending_ = false;
        m_state = STATE_CONNECTED;
        wait_close_ = false;
        is_debug_ = false;
        recv.clear();
        slen = 0;
        _remote_ip = "0.0.0.0";
        // 必须读满一个数据头才返回.
        try
        {
            _remote_ip = socket_.remote_endpoint().address().to_string();
            //cout<<"session::start(),remote ip:"<<_remote_ip<<endl;
            if (g_print_debug_info)
            {
                cout<<"session::start(),remote ip:"<<_remote_ip<<endl;
            }
            if (add_ip_connect(_remote_ip) < 0)
            {
                using_ = false;
                boost::system::error_code ignored_ec;
                socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
                socket().close(ignored_ec);
                return;
            }
            is_reading_ = true;

            int ver = my_random(100, 10000);
            json_spirit::Object robj;
            robj.push_back( Pair("cmd", "login"));
            robj.push_back( Pair("s", 200));
            robj.push_back( Pair("login", 1));
            robj.push_back( Pair("ver", ver));
            do_send(json_spirit::write(robj));

            recv.encryption = ver % 3;
            if (recv.encryption)
            {
                ++recv.encryption;
            }
            // 必须读满一个数据头才返回.
            try_read();

            boost::asio::async_read(socket_, boost::asio::buffer(tempbuff, NET_MAX_BUFF_SIZE/2),
                boost::asio::transfer_at_least(8),
#ifdef USE_STRAND
                strand_.wrap(
#endif // USE_STRAND
                boost::bind(&session::handle_read_login_or_policy, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
#ifdef USE_STRAND
                )
#endif // USE_STRAND
            );

        }
        catch (boost::system::system_error& e)
        {
            cout<<"start session fail!"<<e.what()<<endl;
            is_reading_ = false;
            _closeconnect();
        }
    }

    void session::_send(const std::string& buff, bool pack)
    {
        try
        {
            INFO("send data to "<<_remote_ip<<endl);
            if (pack)
            {
                memcpy(send_data, NET_PACK_START_STRING, 8);
                send_data[11] = 1;
                send_data[12] = 1;
                slen = buff.length() + NET_HEAD_SIZE + 8;
                //设置内容长度
                *(unsigned short*)(send_data + 8) = htons(buff.length());
                memcpy(send_data + NET_HEAD_SIZE + 8, buff.data(), slen);
                INFO(buff<<endl);
                INFO("org.len="<<buff.length()<<".send len="<<slen<<endl);
                DUMP(send_data, slen);
            }
            else
            {
                slen = buff.length();
                memcpy(send_data, buff.data(), slen);
            }
            net::write_handler w_handler(shared_from_this());
            boost::asio::async_write(socket_, boost::asio::buffer(send_data, slen),
                    #ifdef USE_STRAND  
                        strand_.wrap(
                    #endif // USE_STRAND
                       w_handler
                    #ifdef USE_STRAND
                        )
                    #endif // USE_STRAND
                    );
        }
        catch (boost::system::system_error& e)
        {
            INFO("_send exception:"<<e.what()<<endl);
            boost::mutex::scoped_lock lock(session_mutex_);
            is_sending_ = false;
            wait_close_ = true;
            m_send_que.clear();
            boost::system::error_code ignored_ec;
            socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            socket().close(ignored_ec);
            if (!is_reading_)
            {
                is_using(false);
                if (m_ponlineuser != NULL)
                {
                    accountOffline(m_ponlineuser->m_account);
                    m_ponlineuser.reset();                    
                }
            }
        }
    }

    void session::send(const std::string& buff, bool pack)
    {
        if (buff.size() >= (sizeof(send_data) - 100))
        {
            INFO("packet to send is to large!"<<endl);
            return;
        }
        io_service_.post(boost::bind(&session::do_send, shared_from_this(), buff, pack));
    }

    void session::do_send(const std::string& buff, bool pack)
    {
        INFO("session::send"<<endl);
        if (!wait_close_)
        {
            try
            {
                boost::mutex::scoped_lock lock(session_mutex_);
                m_send_que.push_back(buff);
                if (is_sending_)
                {
                    INFO("is_sending_!"<<endl);
                }
                else
                {
                    INFO("call _send()"<<endl);
                    is_sending_ = true;
                    io_service_.post(boost::bind(&session::_send,
                        shared_from_this(),
                        buff, pack));
                }
            }
            catch (boost::system::system_error& e)
            {
                INFO("send exception:"<<e.what()<<endl);
                boost::mutex::scoped_lock lock(session_mutex_);
                is_sending_ = false;
                wait_close_ = true;
                m_send_que.clear();
                boost::system::error_code ignored_ec;
                socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
                socket().close(ignored_ec);
                if (!is_reading_)
                {
                    is_using(false);
                    if (m_ponlineuser.get())
                    {
                        accountOffline(m_ponlineuser->m_account);
                        m_ponlineuser.reset();
                    }
                }
            }
        }
        else
        {
            INFO("wait to close!not send"<<endl);
        }
    }

    void session::handle_read_login_or_policy(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error)
        {
            if (g_print_debug_info)
            {
                cout<<"handle_read_login_or_policy->"<<tempbuff<<"<-"<<endl;
            }
            using namespace std;
            if (!strncmp(tempbuff, "login!!!", 8))
            {
                INFO("recv login!!!"<<remote_ip()<<endl);
                using namespace json_spirit;
                Object robj;
                robj.push_back( Pair("cmd", "login"));
                robj.push_back( Pair("s", 200));
                robj.push_back( Pair("login", 1));
                do_send(write(robj));
                // 必须读满一个数据头才返回.
                if (bytes_transferred > 8)
                {
                    recv.adddata(tempbuff + 8, bytes_transferred - 8);
                    int ret = recv.check(*this);
                    while (0 == ret)
                    {
                        ret = recv.check(*this);
                    }
                    if (1 == ret)
                    {
                        // 继续读
                        try_read();
                    }
                    else
                    {
                        is_reading_ = false;
                        _closeconnect();
                    }
                }
            }
            else if (!strncmp(tempbuff, "<policy-", 8))
            {
                is_reading_ = false;
               INFO("recv <policy> "<<remote_ip()<<endl);
                do_send("<?xml version=\"1.0\"?><!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\"><cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\" /><allow-http-request-headers-from domain=\"*\" headers=\"*\" /></cross-domain-policy>", false);
                closeconnect();
            }
            else
            {
                int total_recv = bytes_transferred;
                if (bytes_transferred >= 34 && !strncmp(tempbuff,"tgw_l7_forward", 14))
                {
                    bool needClose = true;
                    if (tempbuff[14] == 0x0d && tempbuff[15] == 0x0a)
                    {
                        if (tempbuff[bytes_transferred-9] == 0x0a
                            && tempbuff[bytes_transferred-10] == 0x0d
                            && tempbuff[bytes_transferred-11] == 0x0a
                            && tempbuff[bytes_transferred-12] == 0x0d)
                        {
                            tempbuff[bytes_transferred-9] = 0;
                            if (!strncmp(tempbuff+16, "Host: ", 6)
                                && !strncmp(tempbuff + bytes_transferred - 8, "login!!!", 8))
                            {
                                needClose = false;
                                INFO("recv login!!!"<<remote_ip()<<endl);
                                using namespace json_spirit;
                                Object robj;
                                robj.push_back( Pair("cmd", "login"));
                                robj.push_back( Pair("s", 200));
                                robj.push_back( Pair("login", 1));
                                do_send(write(robj));
                                // 必须读满一个数据头才返回.
                                try_read();
                            }
                        }
                    }
                    if (needClose)
                    {
                        is_reading_ = false;
                        _closeconnect();
                    }
                }
                else
                {
                    recv.adddata(tempbuff, bytes_transferred);
                    int ret = recv.check(*this);
                    while (0 == ret)
                    {
                        ret = recv.check(*this);
                    }
                    if (1 == ret)
                    {
                        // 继续读
                        try_read();
                    }
                    else
                    {
                        is_reading_ = false;
                        _closeconnect();
                    }
                }
#if 0
                if (!strncmp(tempbuff, "GET ", 4))
                {
                    _httpRequst._bComplete = false;
                    _httpRequst._state = sw_start;
                    _httpRequst._type = 1;
                    memcpy(_httpRequst._req_url, tempbuff, 8);
                    _httpRequst._req_url[8] = 0;
                    _httpRequst._pos = 8;
                    read_http_request();
                }
                else if (!strncmp(tempbuff, "POST ", 5))
                {
                    _httpRequst._bComplete = false;
                    _httpRequst._state = sw_start;
                    _httpRequst._type = 2;
                    memcpy(_httpRequst._req_url, tempbuff, 8);
                    _httpRequst._req_url[8] = 0;
                    _httpRequst._pos = 8;
                    read_http_request();
                }
                else
                
                is_reading_ = false;
                {
                    _closeconnect();
                }
#endif
            }
        }
        else if (error != boost::asio::error::operation_aborted)
        {
            is_reading_ = false;
            boost::system::error_code ignored_ec;
            socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            socket().close(ignored_ec);
            is_using(false);
        }
        else
        {
            is_reading_ = false;
            m_ponlineuser.reset();
            is_using(false);
        }
    }  

    void session::try_read()
    {
        // 必须读满一个数据头才返回.
        is_reading_ = true;
        try
        {
            INFO("try read!!!"<<endl);
            boost::asio::async_read(socket_, boost::asio::buffer(tempbuff, NET_MAX_BUFF_SIZE/2),
                boost::asio::transfer_at_least(1),
            #ifdef USE_STRAND
                strand_.wrap(
            #endif // USE_STRAND
                boost::bind(&session::handle_read, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
            #ifdef USE_STRAND
                )
            #endif // USE_STRAND
            );
        }
        catch (boost::system::system_error& e)
        {
            is_reading_ = false;
            INFO(e.what()<<endl);
            _closeconnect();
        }
    }
    void session::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
    {
        //有错误，或者需要关闭socket
        if (error && error != boost::asio::error::operation_aborted)
        {
            is_reading_ = false;
            _closeconnect();
            return;
        }
        //读成功
        if (!error)
        {
            recv.adddata(tempbuff, bytes_transferred);
            int ret = recv.check(*this);
            while (0 == ret)
            {
                ret = recv.check(*this);
            }
            if (1 == ret)
            {
                // 继续读
                try_read();
            }
            else
            {
                is_reading_ = false;
                _closeconnect();
            }
        }
        if (error == boost::asio::error::operation_aborted)
        {
            is_reading_ = false;
            m_ponlineuser.reset();
            is_using(false);
        }
    }

    int session::process_msg(json_spirit::mValue& value)
    {
        using namespace std;
        using namespace boost;
        using namespace json_spirit;
        Object robj;
        if (value.type() != obj_type)
        {
            if (m_retry < 3)
            {
                ++m_retry;
                INFO("*******************************recv none obj!!!!!"<<endl);
            }
            else
            {
                Object robj;
                robj.push_back( Pair("cmd", "bye"));
                robj.push_back( Pair("s", 402));
                send(write(robj));
                return -1;
            }
        }
        else
        {
            mObject& mobj = value.get_obj();
            std::string cmd = "";
            READ_STR_FROM_MOBJ(cmd, mobj, "cmd");
            if (cmd == "")
            {
                if (m_retry < 3)
                {
                    ++m_retry;
                    return 0;
                }
                else
                {
                    Object robj;
                    robj.push_back( Pair("cmd", "bye"));
                    robj.push_back( Pair("s", 402));
                    do_send(write(robj));
                    return -1;
                }
            }
            
            if (cmd == "beat")
            {
                robj.clear();
                robj.push_back( Pair("cmd", "beat"));
                robj.push_back( Pair("time_now", time(NULL)));
                robj.push_back( Pair("s", 200));
                do_send(write(robj));
            }
            else if (cmd == "login")
            {
                if ((time(NULL) - m_last_log_time) >= 2)
                {
                    m_log_cmds = 0;
                    m_last_log_time = time(NULL);
                }
                ++m_log_cmds;
                //忽略過多命令
                if (m_log_cmds > 6)
                {
                    return 0;
                }
                loginmessage msg;
                try
                {
                    READ_STR_FROM_MOBJ(msg.username, mobj, "user");
                    READ_STR_FROM_MOBJ(msg.password, mobj, "password");

                    READ_STR_FROM_MOBJ(msg._qid, mobj, "qid");
                    READ_STR_FROM_MOBJ(msg._qname, mobj, "qname");
                    READ_STR_FROM_MOBJ(msg._server_id, mobj, "server_id");
                    READ_STR_FROM_MOBJ(msg._sign, mobj, "sign");
                    READ_STR_FROM_MOBJ(msg._extra1, mobj, "extra1");
                    READ_STR_FROM_MOBJ(msg._extra2, mobj, "extra2");

                    READ_INT_FROM_MOBJ(msg._isAdult, mobj, "isAdult");
                    READ_INT_FROM_MOBJ(msg._union_id, mobj, "union_id");
                    READ_INT_FROM_MOBJ(msg._time, mobj, "time");

#ifdef QQ_PLAT
                    READ_INT_FROM_MOBJ(msg._is_qq_yellow, mobj, "vip_is_yellow");
                    READ_INT_FROM_MOBJ(msg._is_qq_year_yellow, mobj, "vip_is_year_yellow");
                    READ_INT_FROM_MOBJ(msg._qq_yellow_level, mobj, "vip_yellow_level");
                    READ_STR_FROM_MOBJ(msg._login_str1, mobj, "str1");
                    READ_STR_FROM_MOBJ(msg._login_str2, mobj, "str2");
                    READ_STR_FROM_MOBJ(msg._iopenid, mobj, "iopenid");
                    READ_STR_FROM_MOBJ(msg._feedid, mobj, "feed_id");
#endif
                    msg.setsession(shared_from_this());
                }
                catch (std::exception& e)
                {
                    cout<<e.what()<<endl;
                }
                // 将数据提交到工作队列.
                loginwork_.submitjob(msg);
            }
            #if 0
            else if (cmd == "chat")
            {
                m_retry = 0;
                chatmessage msg;
                try
                {
                    READ_STR_FROM_MOBJ(msg.towho_, mobj, "to");
                    READ_INT_FROM_MOBJ(msg.channel_type_, mobj, "ctype");
                    READ_INT_FROM_MOBJ(msg.chat_type_, mobj, "type");
                    READ_STR_FROM_MOBJ(msg.msg_, mobj, "m");
                }
                catch (std::exception& e)
                {
                    cout<<e.what()<<endl;
                }

                msg.setsession(shared_from_this());
                // 将数据提交到工作队列.
                chatwork_.submitjob(msg);
            }
            #endif
            else
            {
                if ((time(NULL) - m_last_action_time) >= 5)
                {
                    m_action_cmds = 0;
                    m_last_action_time = time(NULL);
                }
                ++m_action_cmds;
                //忽略過多命令
                if (m_action_cmds > 100)
                {
                    if (!m_too_many_cmds)
                    {
                        m_too_many_cmds = true;
                        robj.clear();
                        robj.push_back( Pair("cmd", "tooManyCmds"));
                        robj.push_back( Pair("s", 200));
                        do_send(write(robj));
                        if (m_ponlineuser.get() && m_ponlineuser->m_onlineCharactor.get() && m_ponlineuser->m_onlineCharactor->m_charactor.get())
                        {
                            cout<<"!!!!!!!!!!!!!! too many cmds, cid "<<m_ponlineuser->m_onlineCharactor->m_charactor->m_id<<endl;
                        }
                    }
                    return 0;
                }
                m_too_many_cmds = false;
                actionmessage act_msg(mobj);
                act_msg.setsession(shared_from_this());
                actionwork_.submitjob(act_msg);
            }
        }
        return 0;
    }
    
    void session::handle_write(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error)
        {
            if (slen > bytes_transferred)
            {
                INFO("***********handle_write************"<<slen<<"!="<<bytes_transferred<<endl);
                memmove(send_data, send_data+bytes_transferred, slen - bytes_transferred);
                slen = slen - bytes_transferred;
                try
                {
                    net::write_handler w_handler(shared_from_this());
                    boost::asio::async_write(socket_, boost::asio::buffer(send_data, slen),
                    #ifdef USE_STRAND  
                        strand_.wrap(
                    #endif // USE_STRAND
                       w_handler
                    #ifdef USE_STRAND
                        )
                    #endif // USE_STRAND
                    );
                }
                catch(boost::system::system_error& e)
                {
                    boost::mutex::scoped_lock lock(session_mutex_);
                    is_sending_ = false;
                    wait_close_ = true;
                    m_send_que.clear();
                    boost::system::error_code ignored_ec;
                    socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
                    socket().close(ignored_ec);
                    if (!is_reading_)
                    {
                        is_using(false);
                        if (m_ponlineuser.get())
                        {
                            INFO(e.what()<<endl);
                            accountOffline(m_ponlineuser->m_account);
                            m_ponlineuser.reset();
                        }
                    }
                }
            }
            //如果发完了
            else
            {
                INFO("***********handle_write************ complete "<<bytes_transferred<<endl);
                //清空发送缓冲
                slen = 0;
                boost::mutex::scoped_lock lock(session_mutex_);
                //移除发送完的包
                if (m_send_que.size() > 0)
                {
                    m_send_que.pop_front();
                } 
                //找到下一个要发送的包
                if (m_send_que.size() > 0)
                {
                    std::string buff = m_send_que.front();
                    return _send(buff);
                }
                else
                {
                    is_sending_ = false;
                    if (wait_close_)
                    {
                        boost::system::error_code ignored_ec;
                        socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
                        socket().close(ignored_ec);
                        if (!is_reading_)
                        {
                            is_using(false);
                            if (m_ponlineuser != NULL)
                            {
                                INFO(error<<endl);
                                accountOffline(m_ponlineuser->m_account);
                                m_ponlineuser.reset();
                            }
                        }
                    }
                    return;
                }
            }            
        }
        else if ((error && error != boost::asio::error::operation_aborted))
        {
            boost::mutex::scoped_lock lock(session_mutex_);
            is_sending_ = false;
            wait_close_ = true;
            m_send_que.clear();
            boost::system::error_code ignored_ec;
            socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            socket().close(ignored_ec);
            if (!is_reading_)
            {
                is_using(false);
                if (m_ponlineuser != NULL)
                {
                    INFO(error<<endl);
                    accountOffline(m_ponlineuser->m_account);
                    m_ponlineuser.reset();
                }
            }
        }
    }

    void session::closeconnect(bool kickoff)
    {
        io_service_.post(boost::bind(&session::_closeconnect, shared_from_this(), kickoff));
    }

    void session::_closeconnect(bool kickoff)
    {
        boost::mutex::scoped_lock lock(session_mutex_);
        wait_close_ = true;
        if (!is_sending_ && !is_reading_)
        {
            INFO("session::closeconnect()"<<endl);
            if (m_ponlineuser != NULL)
            {
                if (!kickoff)
                {
                    accountOffline(m_ponlineuser->m_account);
                }
                m_ponlineuser.reset();
            }
            wait_close_ = false;
            m_send_que.clear();
            boost::system::error_code ignored_ec;
            socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            socket().close(ignored_ec);
            is_using(false);
        }
        return;
    }

    void session::handle_read_http_request(const boost::system::error_code& error, size_t bytes_transferred)
    {
        //有错误，或者需要关闭socket
        if (error && error != boost::asio::error::operation_aborted)
        {
            is_reading_ = false;
            _closeconnect();
            return;
        }
        //读成功
        if (!error)
        {
            // state 
            for (size_t i = 0; i < bytes_transferred; ++i)
            {
                switch (_httpRequst._state)
                {
                    case sw_start:
                        if (tempbuff[i] == 0x0d)
                        {
                            _httpRequst._state = sw_req_url_CR;
                            _httpRequst._req_url[_httpRequst._pos] = tempbuff[i];
                            ++_httpRequst._pos;
                        }
                        else
                        {
                            if (_httpRequst._pos > 253)
                            {
                                goto close_connect;
                            }
                            _httpRequst._req_url[_httpRequst._pos] = tempbuff[i];
                            ++_httpRequst._pos;
                        }
                        break;
                    case sw_req_url_CR:
                        if (tempbuff[i] == 0x0a && _httpRequst._pos >= 10 && !strncmp(_httpRequst._req_url + _httpRequst._pos - 10, " HTTP", 5))
                        {
                            _httpRequst._state = sw_req_url_LF;
                            _httpRequst._req_url[_httpRequst._pos] = tempbuff[i];
                            _httpRequst._req_url[_httpRequst._pos - 10] = 0;
                            _httpRequst._pos = _httpRequst._pos - 10;
                        }
                        else
                        {
                            goto close_connect;
                        }
                        break;
                    case sw_req_url_LF:
                        if (tempbuff[i] == 0x0d)
                        {
                            _httpRequst._state = sw_end_CR1;
                        }
                        break;
                    case sw_end_CR1:
                        if (tempbuff[i] == 0x0a)
                        {
                            _httpRequst._state = sw_end_LF1;
                        }
                        else
                        {
                            _httpRequst._state = sw_req_url_LF;
                        }
                        break;
                    case sw_end_LF1:
                        if (tempbuff[i] == 0x0d)
                        {
                            _httpRequst._state = sw_end_CR2;
                        }
                        else
                        {
                            _httpRequst._state = sw_req_url_LF;
                        }
                        break;
                    case sw_end_CR2:
                        if (tempbuff[i] == 0x0a)
                        {
                            _httpRequst._state = sw_end_LF2;
                            //成功接受到完整http request
                        }
                        else
                        {
                            _httpRequst._state = sw_req_url_LF;
                        }
                        break;
                }
                
                if (sw_end_LF2 == _httpRequst._state)
                {
                    break;
                }
            }
            if (sw_end_LF2 == _httpRequst._state)
            {
                INFO("SUCCESS!"<<endl);
                INFO("["<<endl);
                INFO(_httpRequst._req_url<<endl);
                INFO(endl<<"]"<<endl);

                bool bFindStart = false;
                int iStartPos = 0;
                for (int i = 0; i < _httpRequst._pos - 1; ++i)
                {
                    if (_httpRequst._req_url[i] == '/' && _httpRequst._req_url[i+1] == '?')
                    {
                        bFindStart = true;
                        iStartPos = i + 2;
                        break;
                    }
                }
                if (bFindStart)
                {
                    json_spirit::mObject mobj;
                    mobj["cmd"] = "recharge";
                     
                     std::string strInput(_httpRequst._req_url + iStartPos);
                    INFO("recv recharge cmd ...["<<strInput<<"]"<<endl);
                    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
                    boost::char_separator<char> sep("&");
                    boost::char_separator<char> sep2("=");

                    tokenizer tok(strInput, sep);
                    tokenizer::iterator it = tok.begin();
                    while (it != tok.end())
                    {
                        INFO("pair:"<<*it<<endl);
                        tokenizer tok2(*it, sep2);
                        tokenizer::iterator it_key = tok2.begin();
                        if (it_key == tok2.end())
                        {
                            ERR();
                            break;
                        }
                        std::string key = *it_key;
                        ++it_key;
                        if (it_key == tok2.end())
                        {
                            ERR();
                            break;
                        }
                        INFO(key<<"="<<*it_key<<endl);
                        if (key != "cmd")
                        {
                            mobj[key] = *it_key;
                        }
                        ++it;
                    }
                    actionmessage act_msg(mobj, 1);
                    act_msg.setsession(shared_from_this());
                    actionwork_.submitjob(act_msg);
                    return;
                }
            }
            else
            {
                INFO("continue read..."<<endl);
                read_http_request();
                return;
            }
            close_connect:
            is_reading_ = false;
            boost::system::error_code ignored_ec;
            socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            socket().close(ignored_ec);
            is_using(false);
        }
    }

    int session::read_http_request()
    {
        is_reading_ = true;
        try
        {
            boost::asio::async_read(socket_, boost::asio::buffer(tempbuff, NET_MAX_BUFF_SIZE/2),
                boost::asio::transfer_at_least(1),
            #ifdef USE_STRAND
                strand_.wrap(
            #endif // USE_STRAND
                boost::bind(&session::handle_read_http_request, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
            #ifdef USE_STRAND
                )
            #endif // USE_STRAND
            );
        }
        catch(boost::system::system_error& e)
        {
            is_reading_ = false;
            INFO(e.what()<<endl);
            _closeconnect();
        }
        return 0;
    }

//////////////////////////////////////////////////////////////////////////
    server::server(short port, jobqueue<actionmessage>& actionwork, jobqueue<loginmessage>& loginwork, std::size_t io_service_pool_size/* = 4*/, short port2/* = 0 */)
        : actionwork_(actionwork)
        , loginwork_(loginwork)
        , io_service_pool_(io_service_pool_size)
        , acceptor_(io_service_pool_.get_io_service(), tcp::endpoint(tcp::v4(), port))
        , acceptor2_(io_service_pool_.get_io_service(), tcp::endpoint(tcp::v4(), port2))
    {
        session_ptr new_session(new session(io_service_pool_.get_io_service(), actionwork_, loginwork_));
    #ifdef USE_POOL
        session_pool_.push_back(new_session);
    #endif // USE_POOL   
        cout<<"accept on port1 "<<port<<endl;
        try
        {
            acceptor_.async_accept(new_session->socket(),
                boost::bind(&server::handle_accept, this, new_session, &acceptor_,
                boost::asio::placeholders::error));
            if (port2 > 0)
            {
                cout<<"accept on port2 "<<port2<<endl;
                //session_ptr new_session2(new session(io_service_pool_.get_io_service(), actionwork_, loginwork_, chatwork_));
                //#ifdef USE_POOL
                //    session_pool_.push_back(new_session2);
                //#endif // USE_POOL        
                acceptor2_.async_accept(new_session->socket(),
                    boost::bind(&server::handle_accept, this, new_session, &acceptor2_,
                    boost::asio::placeholders::error));
            }
        }
        catch (boost::system::system_error & e)
        {
            ERR();
            std::cout << e.what() << std::endl;
        }
        catch (std::exception& e)
        {
            std::cerr << "Net server,Exception: " << e.what() << "\n";
#ifndef _WINDOWS
            void * array[25];
            int nSize = backtrace(array, 25);
            char ** symbols = backtrace_symbols(array, nSize);
            for (int i = 0; i < nSize; i++)
            {
                cout << symbols[i] << endl;
            }
            free(symbols);
#endif
        }
        catch (...)
        {
            ERR();
            std::cout<< "Unknow excption!" <<std::endl;
        }
        g_free_connects["60.191.14.40"] = 1;
        g_free_connects["60.12.226.109"] = 1;
        g_free_connects["localhost"] = 1;
        g_free_connects["127.0.0.1"] = 1;
        g_free_connects["183.129.166.234"] = 1;
    }

    void server::run()
    {
        io_service_pool_.run();
    }

    void server::stop()
    {
        io_service_pool_.stop();
    }

#ifdef USE_POOL    
void server::check_sessions()
{
    std::vector<session_ptr>::iterator i = session_pool_.begin();
    while (i != session_pool_.end())
    {
        if ((*i)->is_using() && (*i)->state() == STATE_CONNECTED && ((*i)->connect_time() + 3) < time(NULL))
        {
            (*i)->_closeconnect();
            if (session_pool_.size() > MAX_SESSITONS)
            {
                i = session_pool_.erase(i);
                continue;
            }
        }
        ++i;
    }
    if (g_print_debug_info)
    {
        int unused = 0;
        i = session_pool_.begin();
        while (i != session_pool_.end())
        {
            if (!(*i)->is_using())
            {
                ++unused;
            }
            ++i;
        }
        cout<<"session_pool_.size = "<<session_pool_.size()<<",unused "<<unused<<endl;
    }
}
#endif // USE_POOL

    void server::handle_accept(session_ptr new_session, tcp::acceptor* acp,
        const boost::system::error_code& error)
    {
        if (!error)
        {
            try
            {
                if (acp != NULL)
                {
                    boost::asio::ip::tcp::endpoint endpoint = acp->local_endpoint();
                    new_session->set_port(endpoint.port());
                }
                new_session->start();
                //cout<<"server::handle_accept()"<<endl;

                std::vector<session_ptr>::iterator i;
                // 尝试在session池寻找一个没有使用的session.
                for(i = session_pool_.begin(); i != session_pool_.end(); i++)
                {
                    if (!(*i)->is_using())
                    {
                        new_session = *i;
                        if (g_print_debug_info)
                        {
                            cout<<"server::handle_accept() find a unused session, session_pool.size()="<<session_pool_.size()<<endl;
                        }
                        break;
                    }
                }
                // 没找到将插入一个新的session到池中.
                if (i == session_pool_.end())
                {
                #ifdef USE_POOL
                    if (session_pool_.size() < MAX_SESSITONS)
                       {
                           if (g_print_debug_info)
                           {
                            cout<<"server::handle_accept() new a session, session_pool.size()="<<session_pool_.size()<<endl;
                           }
                        check_sessions();
                           new_session.reset(new session(io_service_pool_.get_io_service(), actionwork_, loginwork_));
                        session_pool_.push_back(new_session);
                    }
                    else
                    {
                        if (g_print_debug_info)
                        {
                            cout<<"server::handle_accept() new a session and try to close a session, session_pool.size()="<<session_pool_.size()<<endl;
                        }
                        new_session->_closeconnect();
                        check_sessions();
                        new_session.reset(new session(io_service_pool_.get_io_service(), actionwork_, loginwork_));
                        session_pool_.push_back(new_session);
                    }
                #else
                    new_session.reset(new session(io_service_pool_.get_io_service(), actionwork_, loginwork_));
                #endif // USE_POOL
                }
                acceptor_.async_accept(new_session->socket(),
                        boost::bind(&server::handle_accept, this, new_session, &acceptor_,
                        boost::asio::placeholders::error));
                if (acceptor2_.local_endpoint().port()>0)
                {
                    acceptor2_.async_accept(new_session->socket(),
                        boost::bind(&server::handle_accept, this, new_session, &acceptor2_,
                        boost::asio::placeholders::error));
                }
            }
            catch (boost::system::system_error & e)
            {
                ERR();
                std::cout << e.what() << std::endl;
            }
            catch (std::exception& e)
            {
                std::cerr << "Net server,Exception: " << e.what() << "\n";
#ifndef _WINDOWS
                void * array[25];
                int nSize = backtrace(array, 25);
                char ** symbols = backtrace_symbols(array, nSize);
                for (int i = 0; i < nSize; i++)
                {
                    cout << symbols[i] << endl;
                }
                free(symbols);
#endif
            }
            catch (...)
            {
                ERR();
                std::cout<< "Unknow excption!" <<std::endl;
            }
        }
    }


    char mySession::policy_file_buffer[512];
    int mySession::policy_file_buffer_len;

    void mySession::start_session()
    {
        assert (state == 0);
        state = 1;
        //发送策略文件
        boost::asio::async_read(m_socket, boost::asio::buffer(tempbuff, policy_file_request_len),
            boost::asio::transfer_at_least(policy_file_request_len),
            strand_.wrap(
            boost::bind(&mySession::handle_read_request, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)) );
    }
    void mySession::close_session()
    {
        if (state != 0)
        {
            boost::system::error_code ignored_ec;
            m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            m_socket.close(ignored_ec);
            state = 0;
        }
    }
    void mySession::handle_read_request(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (error || bytes_transferred != policy_file_request_len)
        {
            //cout<<"policyServer, receive a unknow request. error:"<<error<<",size:"<<bytes_transferred<<endl;
            close_session();
        }
        else
        {
            //cout<<"policyServer, receive a policy request."<<endl;
            state = 2;
            //发送策略文件
            boost::asio::async_write(m_socket, boost::asio::buffer(mySession::policy_file_buffer, mySession::policy_file_buffer_len),
                strand_.wrap(
                boost::bind(&mySession::handle_send_policy, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)) );
        }
    }

    void mySession::handle_send_policy(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error)
        {
            //cout<<"policyServer, send policy file."<<endl;
        }
        else
        {
            cout<<"policyServer, fail to send policy file. error "<<error<<endl;
        }
        close_session();
    }

    policyServer::policyServer()
    {        
        io_service_.reset(new boost::asio::io_service);
        work_.reset(new boost::asio::io_service::work(*io_service_));
    }

    policyServer::~policyServer()
    {
        
    }
    void policyServer::run()
    {
        mySession::policy_file_buffer_len = sizeof("<?xml version=\"1.0\"?><!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\"><cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\" /><allow-http-request-headers-from domain=\"*\" headers=\"*\" /></cross-domain-policy>");
        strcpy(mySession::policy_file_buffer, "<?xml version=\"1.0\"?><!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\"><cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\" /><allow-http-request-headers-from domain=\"*\" headers=\"*\" /></cross-domain-policy>");
        cout<<"************************* policyServer::run(), run ************************* "<<endl;
        try
        {
            boost::shared_ptr<boost::thread> thread(new boost::thread(
                boost::bind(&boost::asio::io_service::run, io_service_)));
            start_accept();
            thread->join();
        }
        catch(boost::system::system_error& e)
        {
            cout<<"catch a exception, "<<e.what()<<endl;
        }
        //io_services_[i].print_use_count();
        cout<<"************************* policyServer::run(), exit ************************* "<<endl;
    }
    void policyServer::stop()
    {
        INFO("policyServer::stop()..."<<endl);
        io_service_->stop();
        boost::system::error_code ignored_ec;
        acceptor_->cancel(ignored_ec);
        if (ignored_ec)
        {
            INFO("policyServer acceptor_->cancel() return "<<ignored_ec<<endl);
        }
        acceptor_->close(ignored_ec);
        if (ignored_ec)
        {
            INFO("policyServer acceptor_->close() return "<<ignored_ec<<endl);
        }
        acceptor_.reset();
        INFO("policyServer::stop(),done"<<endl);
    }
    boost::shared_ptr<mySession> policyServer::get_session()
    {
        for (std::list<boost::shared_ptr<mySession> >::iterator it = m_sessions.begin(); it != m_sessions.end(); ++it)
        {
            if ((*it)->state == 0 && (*it)->m_socket.is_open() == false)
            {
                //cout<<"policyServer, find a old session. total "<<m_sessions.size()<<endl;
                return *it;
            }
        }
        boost::shared_ptr<mySession> new_session(new mySession(*io_service_));
        m_sessions.push_back(new_session);
        //cout<<"policyServer, new a socket, total "<<m_sessions.size()<<endl;
        return new_session;
    }
    void policyServer::start_accept()
    {
        boost::shared_ptr<mySession> mysession = get_session();
        if (acceptor_.get() == NULL)
        {
            acceptor_.reset(new tcp::acceptor(*io_service_, tcp::endpoint(tcp::v4(), 843)));
            cout<<"policyServer accept on port 843"<<endl;
        }
        if (acceptor_.get())
        {
            acceptor_->async_accept(mysession->m_socket,
                    boost::bind(&policyServer::handle_accept, this, mysession,
                    boost::asio::placeholders::error));
        }
    }
    void policyServer::handle_accept(boost::shared_ptr<mySession> session_,
        const boost::system::error_code& error)
    {
        if (session_.get())
        {
            session_->start_session();
            //cout<<"policyServer, receive a accept."<<endl;
        }
        start_accept();
    }
} // namespace net

