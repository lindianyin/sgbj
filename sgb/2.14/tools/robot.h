#pragma once

#include "iostream"

using namespace std;

#include "config.h"
#include <boost/asio.hpp>

#include "worker.hpp"
#include "defs.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include "json_spirit_value.h"
#include <string>

using boost::asio::io_service;     
using boost::asio::ip::tcp;

using namespace net;

#define NET_HEAD_SIZE       16
#define NET_MAX_BUFF_SIZE 163840
using boost::asio::ip::tcp;

enum {
    STATE_TRY_CONNECT = -1,
    STATE_CONNECTED = 0,
    STATE_AUTHED = 1,       //�Ѿ���֤
    STATE_DISCONNECT = 2
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

struct testChar;
class tsession;

typedef boost::shared_ptr<tsession> tsession_ptr;

struct trecvbuff
    {
        char data_[NET_HEAD_SIZE + NET_MAX_BUFF_SIZE];
        size_t body_length_;
        char state;     // 0 δ�յ��̶��ֽ�  1 �յ��̶��ֽ�  2 �յ���ͷ 3 �յ�����
        char* point1;
        char* point2;
        trecvbuff();
        void clear();
        int check(tsession& se);     // 0 ���� 1 ������ 2 �Ͽ�
        void adddata(void* ad, size_t len);
    };


class testMessage
{
public:
    testMessage();
    testMessage(json_spirit::mObject& robj, int from);
    json_spirit::mObject& getRecvObj() {return recvObj;};
    testMessage& operator =(testMessage &msg);
    void setsession(const tsession_ptr& _session);
    void getsession(tsession_ptr& _session);
    std::string cmd;
    int from() {return m_from;};

private:
    boost::weak_ptr<tsession> session_;
    json_spirit::mObject recvObj;
    int m_from;
};

struct sendMsg
{
    char _msg[2048];
    bool _pack;
};

//////////////////////////////////////////////////////////////////////////
    class tsession
        : public boost::enable_shared_from_this<tsession>
    {
    public:
        tsession(boost::asio::io_service& io_service, jobqueue<testMessage>& jobwork, tcp::resolver::iterator endpoint_iterator);
        ~tsession();

        tcp::socket& socket();
        bool is_using();
        void is_using(bool b);

        void start();
        void do_start();

        void send(const std::string& msg);
        void write(const sendMsg& msg);
        void do_write(const sendMsg msg);
        void do_close();
        int retry()
        {
            return m_retry;
        };
        void retry(int retry)
        {
            m_retry = retry;
        };
        int state()
        {
            return m_state;
        };
        void state(int state)
        {
            m_state = state;
        };
        void set_port(int port)
        {
            m_port = port;
        };
        boost::shared_ptr<testChar> user()
        {
            return m_char;
        };
        void user(boost::shared_ptr<testChar> puser)
        {
            if (NULL == puser)
            {
                m_char.reset();
            }
            else
            {
                m_char.reset();
                m_char = puser;
            }
            //std::cout<<"session user count "<<puser.use_count()<<std::endl;
        };
        void close();     //�Ƿ��ߺ������Ͽ�����
        void handle_read_login_or_policy(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
        void try_read();
        void read_http_request();
        void handle_read_http_request(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_connect(const boost::system::error_code& error);
        std::string remote_ip() {return _remote_ip;}

        int process_msg(json_spirit::mValue& value);

    private:
        void _send(const std::string& msg, bool pack = true);
        int m_state;    //�Ự״̬
        int m_retry;    //���Դ���
        int m_port;

        char tempbuff[NET_MAX_BUFF_SIZE/2];
        trecvbuff recv;
        char send_data[NET_HEAD_SIZE+50*NET_MAX_BUFF_SIZE + 1000];
        unsigned short slen;

        tcp::socket socket_;
        std::string _remote_ip;
        boost::shared_ptr<testChar> m_char;

        jobqueue<testMessage>& actionwork_;
        tcp::resolver::iterator endpoint_it;

        std::list<sendMsg> m_send_queue;

        bool using_;
        bool wait_close_;
        bool is_sending_;
        bool is_reading_;
        boost::asio::io_service& io_service_;
    #ifdef USE_STRAND
        boost::asio::io_service::strand strand_;
    #endif // USE_STRAND
    };

enum char_state
{
    char_inited = 0,
    char_try_connect,
    char_connected,
    char_login1,
    char_list_return,
    char_login2,
    char_dis = 888
};

struct str_stronghold
{
    int _id;
    int _level;
    int _state;
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

class testChar: public boost::enable_shared_from_this<testChar>
{
public:
    static int _totals;
    char _account[50];
    char _password[50];

    int _cid;        //��ɫid
    int _gid;        //����id
    int _vip;        //vip�ȼ�
    int _level;        //��ɫ�ȼ�
    int _silver;    //��������
    int _gold;        //�������
    int _ling;        //��������
    int _mapid;        //��ͼ����
    int _stage;        //��������
    int _camp;        //��Ӫ��Ϣ

    int _curMapid;    //��ǰ���ڵ�ͼ
    int _curStage;    //��ǰ����
    str_stronghold _strongholds[25];    //�ؿ�

    int _state;

    int _boss_id;
    int _boss_state[10];    //boss����״̬
    int _camp_race_state;    //��Ӫս״̬
    int _trade_state;        //ͨ���Ƿ���
    int _race_state;        //�����Ƿ���
    int _my_trade_state;    //�ҵ�ͨ��״̬
    int _my_camp_race_state;//�ҵ���Ӫս״̬
    int _my_action;        //������ʲô

    char send_data[2048];
    uint64_t _action_time;
    tsession_ptr _session;

    tcp::resolver::iterator _iter;

    std::list<CharactorInfo> _charList;

    int connect();            //����
    int sendAuth();        //��֤
    int queryCharlist();    //��ѯ��ɫ�б�
    int loginChar();
    int chat(int type, const std::string& msg);                //����
    int queryCharInfo();
    int queryMapInfo();
    int queryStageInfo();
    int attackStronghold();    //�����ؿ�
    int queryActionInfo();        //��ѯ���Ϣ
    int queryTradeInfo();        //��ѯͨ����Ϣ
    int queryRaceInfo();        //��ѯ������Ϣ
    int queryBossCD(int id);    //��ѯboss����CD
    int attackBoss(int id);    //����boss
    int queryBossInfo(int id);    //��ѯboss��Ϣ
    int enterBoss(int id);
    int enterCampRace(int type);
    int startCampRace();
    int getCampRace();
    int JoinCamp(int type);
    int queryOfficalInfo();

    int race();
    void randChat();
    void resetActionState();
    void randAction();

    void start();
    
    testChar(boost::asio::io_service& io_service,jobqueue<testMessage>& actionwork, 
      tcp::resolver::iterator endpoint_iterator, const std::string& account, const std::string&, int cid);

    ~testChar();
    void _send(const std::string& buff, bool pack);

      void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred);

    int doSomething();
};

