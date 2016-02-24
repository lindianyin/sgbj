
/*
 *
 * Copyright (C) 2009 jack.wgm, microcai.
 * For conditions of distribution and use, see copyright notice 
 * in (http://code.google.com/p/netsever/source/browse/trunk/COPYING)
 *
 * Author: jack.wgm
 * Email:  jack.wgm@gmail.com
 */

#ifndef ROBOTS_H__
#define ROBOTS_H__

#include "config.h"

#include "jobqueue.hpp"
#include "BasePacket.h"
#include "defs.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include "json_spirit_value.h"
#include <string>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

using namespace std;
using namespace json_spirit;

using boost::asio::ip::tcp;

enum {
    STATE_CONNECTED = 0,
    STATE_AUTHED = 1,       //�Ѿ���֤
    STATE_DISCONNECT = 2
};

#define NET_HEAD_SIZE       16
#define NET_MAX_BUFF_SIZE 2048

namespace net 
{
    class server;
    class session;
    class io_service_pool;

    typedef boost::shared_ptr<session> session_ptr;

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
        char state;     // 0 δ�յ��̶��ֽ�  1 �յ��̶��ֽ�  2 �յ���ͷ 3 �յ�����
        char* point1;
        char* point2;
        recvbuff();
        void clear();
        int check(session& se);     // 0 ���� 1 ������ 2 �Ͽ�
        void adddata(void* ad, size_t len);
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
    class robot;
    class session
        : public boost::enable_shared_from_this<session>
    {
    public:
        session(boost::asio::io_service& io_service);
        ~session();

        tcp::socket& socket();
        boost::asio::io_service& io_service();
    
#ifdef USE_STRAND
        boost::asio::io_service::strand& strand()
        {
            return strand_;
        }
#endif // USE_STRAND
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
        boost::shared_ptr<robot> user()
        {
            return m_ponlineuser;
        }
        void user(boost::shared_ptr<robot>& puser)
        {
            if (NULL == puser.get())
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
        std::string remote_ip() {return _remote_ip;}
        void _closeconnect(bool kickoff = false);     //�Ƿ��ߺ������Ͽ�����
        void closeconnect(bool kickoff = false);     //�Ƿ��ߺ������Ͽ�����
        void handle_read_login_or_policy(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
        void try_read();
        int process_msg(json_spirit::mValue& value);
        int read_http_request();
        void handle_read_http_request(const boost::system::error_code& error, size_t bytes_transferred);

        void handle_connect(const boost::system::error_code& error);

        size_t msg_count();
        actionmessage get_job();
        
        bool is_debug_;
        static uint64_t refs();
    private:
        static volatile uint64_t _refs;
        void _send(const std::string& msg, bool pack = true);
        void do_send(const std::string& buff, bool pack = true);
        int m_state;    //�Ự״̬
        int m_retry;    //���Դ���
        int m_port;

        int m_log_cmds;
        int m_action_cmds;

        time_t m_last_log_time;
        time_t m_last_action_time;
        time_t m_connect_time;

        char tempbuff[NET_MAX_BUFF_SIZE/2];
        recvbuff recv;
        httpRequest _httpRequst;
        char send_data[NET_HEAD_SIZE+50*NET_MAX_BUFF_SIZE + 1000];
        unsigned short slen;

        tcp::socket socket_;
        std::string _remote_ip;
        boost::shared_ptr<robot> m_ponlineuser;

        jobqueue<actionmessage> actionwork_;

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

    class robotMgr;

    enum robot_state
    {
        robot_not_connect = 0,
        robot_try_connect = 1,
        robot_connected = 2,
        robot_try_connect2,
        robot_try_connected2,
        robot_try_auth,
        robot_authed,
        robot_try_login,
        robot_logined1,
        robot_try_login2,
        robot_logined2,
    };

    
    enum char_action
    {
        action_idle = 0,
        action_attack_stronghold,
        action_in_boss,
        action_in_camp_race,
        action_in_group_copy,
        action_in_sweep,
    };
    //��ɫ��¼��Ϣ
    struct CharactorInfo
    {
        int m_cid;
        int m_level;
        int m_spic;
        int m_state;    // 0 ���� 1 ��ɾ��
        time_t m_deleteTime;
        time_t m_lastlogin;
        std::string m_name;
    };

    struct str_stronghold
    {
        int _id;
        int _level;
        int _state;
    };

    typedef void (*pFuncProcessCmds)(net::session_ptr& psession, json_spirit::mObject& o);
    typedef void (*pFuncInternalProcessCmds)(json_spirit::mObject& o);

    class robot: public boost::enable_shared_from_this<robot>
    {
    public:
        robot(robotMgr& h);
        int doSomething();

        int queryCharData();
        int queryTreasure(int tid);

        int get_cid()
        {
            return m_cid;
        }
        void handle_connect(const boost::system::error_code& error);
        
        int try_connect();
        void do_connect();
        bool work(actionmessage& task);

        int queryActionInfo();    //��ѯ���Ϣ
        int queryTradeInfo();     //��ѯͨ����Ϣ
        int queryRaceInfo();        //��ѯ������Ϣ
        //enterBossScene
        int enterBoss(int id);
        //enterCampRace
        int enterCampRace(int type);
        //startCampRace
        int startCampRace();
        //getCampRace
        int getCampRace();
        //join Camp
        int JoinCamp(int type);
        int queryBossCD(int id);    //��ѯboss����CD
        //{"id":1,"cmd":"attackBoss"}
        int attackBoss(int id); //����boss
        //{"id":1,"cmd":"getBossInfo"}
        int queryBossInfo(int id); //��ѯboss��Ϣ
        int queryOfficalInfo();    //��ѯ��ְ

        void resetActionState();

        static void init_cmd_map();

        net::session_ptr _session;

        volatile int m_state;    //����״̬
        //��ɫid
        int m_cid;
        //��ɫ�ȼ�
        int m_level;

        //�ؿ�id
        int m_stronghold;
        //��ͼid
        int m_mapid;

        int m_stage;

        int m_cur_mapid;
        int m_cur_stage;

        //����
        int m_task_state;
        int m_task_id;
        int m_task_need1;
        int m_task_need2;

        //��Ӫ
        int m_camp;
        //����
        int m_corps;

        //����ʣ�����
        int m_hubiao_left;
        int m_rob_left;
        time_t m_hubiao_cool;
        time_t m_rob_cool;

        //����ʣ�����
        int m_race_left;
        time_t m_race_cool;

        //��Һ�����
        int m_gold;
        int m_silver;
        int m_ling;
        int m_vip;
        
        time_t m_boss_cool;
        time_t m_camp_race_cool;

        
        int m_boss_id;
        int m_boss_state[10];    //boss����״̬
        
        int    m_trade_state;
        int m_race_state;
        int m_camp_race_state;
        int m_my_action;
        str_stronghold _strongholds[25];    //�ؿ�

        //����
        std::map<int, int> m_treasures;

        //װ��

        //��ʯ

        //��ɫ��
        std::string m_name;
        //�ʺ�
        std::string _account;

        robotMgr& m_handle;

        //��������
        std::string m_requst_cmd;
        //���󷢳�ʱ��
        time_t m_requst_time;
        //��Ӧʱ��
        time_t m_response_time;

        static boost::unordered_map<std::string, pFuncProcessCmds> m_cmds_process_map;
        static boost::unordered_map<std::string, pFuncInternalProcessCmds> m_internal_cmds_process_map;

        std::list<CharactorInfo> _charList;

    };

    class robotMgr
    {
    public:
        robotMgr(std::string& host, short port = 80, std::size_t io_service_pool_size = 4);
        int workLoop();
        void run();
        void stop();
        void load();
        int checkConnect();
        session_ptr get_session();

        std::string get_host()
        {
            return m_host;
        }
        int get_port()
        {
            return m_port;
        }

    private:
        //��Ӫս�Ƿ�����
        int m_camp_race_open;
        //bossս�Ƿ�����
        int m_boss_open;
        //���˸����Ƿ�����
        int m_group_copy_open;

        //��ǰ����
        int m_seaon;
        //��ǰ���
        int m_year;

        //��ǰʱ��Сʱ������
        int m_hour;
        int m_min;

        //����
        //��Ӫ1��������
        int m_camp1_online;
        //��Ӫ2��������
        int m_camp2_online;

        //���ߵĻ������б�
        std::map<int, boost::shared_ptr<robot> > m_robots;

        //socket����
        net::io_service_pool io_service_pool_;
        std::vector<net::session_ptr> session_pool_;

        std::string m_host;
        int m_port;
    };

}

#endif

