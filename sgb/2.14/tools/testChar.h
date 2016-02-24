#pragma once

#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include "json_spirit_value.h"
#include <string>
#include <list>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/lexical_cast.hpp>

struct rwlock {
    int write;
    int read;
};

static inline void
rwlock_init(struct rwlock *lock) {
    lock->write = 0;
    lock->read = 0;
}

static inline void
rwlock_rlock(struct rwlock *lock) {
    for (;;) {
        while(lock->write) {
            __sync_synchronize();
        }
        __sync_add_and_fetch(&lock->read,1);
        if (lock->write) {
            __sync_sub_and_fetch(&lock->read,1);
        } else {
            break;
        }
    }
}

static inline void
rwlock_wlock(struct rwlock *lock) {
    while (__sync_lock_test_and_set(&lock->write,1)) {}
    while(lock->read) {
        __sync_synchronize();
    }
}

static inline void
rwlock_wunlock(struct rwlock *lock) {
    __sync_lock_release(&lock->write);
}

static inline void
rwlock_runlock(struct rwlock *lock) {
    __sync_sub_and_fetch(&lock->read,1);
}


#define ERR() std::cout<<"!!!!!!!!!!!!!!!!!error!!!!!!!!!!!!!!!!:"<<__FILE__<<" at line "<<__LINE__<<std::endl<<std::flush

#define DB_ERROR(q)\
    ERR();\
    std::cout<<"sql:"<<q.GetLastQuery()<<std::endl<<"error:"<<q.GetError()<<",errno:"<<q.GetErrno()<<std::endl

#define CHECK_DB_ERR(q) if (q.GetErrno())\
    {\
        DB_ERROR(q);\
    }

#define DEF_READ_INT_FROM_MOBJ(m, o, f)\
    int m = 0;\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::int_type)\
        {\
            m = o[f].get_int();\
        }\
    }

#define DEF_READ_UINT64_FROM_MOBJ(m, o, f)\
    uint64_t m = 0;\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::int_type)\
        {\
            m = o[f].get_uint64();\
        }\
    }

#define DEF_READ_STR_FROM_MOBJ(m, o, f)\
    std::string m = "";\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::str_type)\
        {\
            m = o[f].get_str();\
        }\
    }

#define READ_INT_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::int_type)\
        {\
            m = o[f].get_int();\
        }\
    }

#define READ_BOOL_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::bool_type)\
        {\
            m = o[f].get_bool();\
        }\
    }


#define READ_UINT64_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::int_type)\
        {\
            m = o[f].get_uint64();\
        }\
    }

#define READ_REAL_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::real_type)\
        {\
            m = o[f].get_real();\
        }\
    }

#define READ_STR_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::str_type)\
        {\
            m = o[f].get_str();\
        }\
    }

#define READ_OBJ_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::obj_type)\
        {\
            m = o[f].get_obj();\
        }\
    }
#define READ_ARRAY_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::array_type)\
        {\
            m = o[f].get_array();\
        }\
    }

#define LEX_CAST_STR(x)\
    boost::lexical_cast<std::string>(x)

using boost::asio::ip::tcp;
using json_spirit::mObject;

enum cmd_enum
{
    CMD_LOGIN = 1001,
    CMD_CHARLIST,
    CMD_ROLE_LOGIN,
    CMD_CREATE_CHAR,
    CMD_ROLE_INFO,
    CMD_TASK_INFO,
    CMD_GENERAL_LIST,
    CMD_GENERAL_EQUIPTS,
    CMD_STORE_LIST,
    CMD_STAGE_INFO,
    CMD_EQUIP,
    CMD_STRONGHOLD_INFO,
    CMD_ATTACK,
    CMD_GET_STAGE_FINISH,
    CMD_GET_AWARD,
    CMD_GET_POST_GENERAL_LIST,
    CMD_DEAL_BUY,
    CMD_GET_FORMATION,
    CMD_SET_FORMAIION,
    CMD_GET_UPDATE_LIST,
    CMD_GET_BOOK_LIST,
    CMD_TRAIN_GENERAL,
};

//[{"id":1,"primary":1,"type":1,"name":"击败白蛇","goal":1,"current":0,"isDone":1,
//"award":"军令×1","mapid":1,"stageid":1,"pos":1,"state":0}],"cmd":"getCurTask","s":200}
struct task_data
{
    int id;
    int primary;
    int type;
    int goal;
    int current;
    int done;
    int mapid;
    int stageid;
    int pos;
    int state;

    void clear()
    {
        memset(this, 0, sizeof(*this));
    }
};

//{"stageid":1,"cmd":"checkStageFinish","mapid":1}
struct stage_finish
{
    int get;
    int mapid;
    int stageid;
    int can_get;
};

struct stronghold_data
{
    int id;
    int level;
    int state;

    void clear()
    {
        memset(this, 0, sizeof(*this));
    }
};

struct my_strongholds
{
    int mapid;
    int stageid;
    stronghold_data strongholds[9];

    void clear()
    {
        memset(this, 0, sizeof(*this));
    }
};

struct equiptment
{
    int id;
    int type;
    int level;
    int slot;

    void clear()
    {
        memset(this, 0, sizeof(*this));
    }
};

struct char_general
{
    int gid;
    int type;
    int level;
    int upzhen;

    int equips_get;
    equiptment equips[6];

    void clear()
    {
        memset(this, 0, sizeof(*this));
    }
};

struct stronghold_info
{
    int get;
    int mapid;
    int stageid;
    int pos;
    int id;
    int state;
    int need_supply;
    void clear()
    {
        memset(this, 0, sizeof(*this));
    }
};

struct canBuyGeneral
{
    int get;
    int type;
    int price;
    void clear()
    {
        memset(this, 0, sizeof(*this));
    }
};

struct zhenGenerals
{
    int get;
    int total;
    int left;
    int gid[9];
    void clear()
    {
        memset(this, 0, sizeof(*this));
    }
};

struct updateListCD
{
    int get;
    int cd[10][2];
    void clear()
    {
        memset(this, 0, sizeof(*this));
    }
};

struct testCharData
{
    int cid;
    char name[32];
    int level;
    int gold;
    int silver;
    int prestige;
    int gongxun;
    int supply;
    int ling;

    int mapid;
    int stageid;

    int stronghold;
    int finish_all_stronghold;

    int strongholds_get;
    my_strongholds strongholds;

    stronghold_info sinfo;

    stage_finish stage_reward;    //通关奖励

    int tasks_get;
    task_data tasks[3];

    int generals_get;
    int generals_count;
    char_general generals[10];

    int equipts_get;
    equiptment equipts[10];

    canBuyGeneral can_buy_general;

    zhenGenerals zhen_generals;

    updateListCD updateList;
};

const int iMaxRecvBufferLen = 10240;

#pragma pack (1)
struct recvMsg
{
    char _head[8];
    uint16_t len;
    char _xxx[14];
    char _data[iMaxRecvBufferLen+1];
    bool _drop;
    uint16_t _left_recv;
};
#pragma pack ()

class robotMgr;

enum CHAR_STATE
{
    //0 未连接 1发起连接 2连接成功 3收到欢迎信息,发送登录， 4验证通过，请求创建角色 5发起创建角色 6登录角色成功 -1连接失败,-2连接关闭    
    CHAR_STATE_UNCONNECT = 0,            //未连接
    CHAR_STATE_TRY_CONNECT = 1,        //发起连接
    CHAR_STATE_CONNECTED = 2,            //连接成功
    CHAR_STATE_TRY_LOGIN = 3,            //收到欢迎信息,发送登录
    CHAR_STATE_LOGINED = 4,            //登录成功，请求角色列表
    CHAR_STATE_TRY_LOGIN_CHAR = 5,    //收到角色列表,登录角色
    CHAR_STATE_TRY_CREATE_CHAR = 6,    //创建角色中
    CHAR_STATE_CHAR_LOGINED = 7,        //角色登入成功
    CHAR_STATE_CONNECT_FAIL = -1,        //连接失败
    CHAR_STATE_CONNECT_CLOSED = -2,    //连接关闭
};

class testRobot: public boost::enable_shared_from_this<testRobot>
{
public:
    testRobot(robotMgr& h, const std::string& account);
    void postStart();
    
    void postHeartbeat(time_t t, int cmd = 0);

    time_t lastHeartbeat() {return m_heart_beat; }

    int getState() { return state_; }

    int getLevel() { return cdata_.level; }

private:
    void tryAuth();
    void tryReadHead();
    void tryReadData();

    void handleReadHead(const boost::system::error_code& error, size_t bytes_transferred);
    void handleReadData(const boost::system::error_code& error, size_t bytes_transferred);

    void handleStart();
    void handleHeartbeat(int cmd);

    void handleWrite(const boost::system::error_code& error, size_t bytes_transferred);
    void handleConnect(const boost::system::error_code& error);

    void doWrite(const std::string& msg);

    void processCharList(json_spirit::mObject& mobj);
    void ProcessCharStageInfo(json_spirit::mObject& o);
    void ProcessStrongholdInfo(json_spirit::mObject& o);

    void ProcessTaskList(json_spirit::mObject& o);
    void ProcessGeneralList(mObject& o);
    void ProcessEquipmentlist(mObject& o);
    void ProcessGeneralEquiptments(mObject& o);
    void ProcessStageFinish(mObject& o);
    void ProcessCanBuyGeneralList(mObject& o);
    void ProcessFormation(mObject& o);
    
    //处理左侧队列信息
    void ProcessUpdateList(mObject& o);
    void ProcessGetBookList(mObject& o);

    int queryStageInfo();

    int attackStronghold();    //攻击关卡
    //是否可以装上装备
    int checkUpEquipment();
    //是否可以强化装备
    int checkEnhanceEquipment();
    //是否可以招募武将
    int checkCanBuyGeneral();
    //是否可以升级秘法
    int checkCanLevelupSkill();
    //是否可以完成扫荡任务
    int checkCanSweep();
    //武将是否可以上阵
    int checkCanUpZhen();

    //武将是否可以训练升级
    int checkCanTrain();

    void closeSocket();

    std::string m_account;
    robotMgr& m_mgr;

    boost::asio::io_service& io_service_;
    boost::asio::io_service::strand strand_;
    tcp::socket socket_;

    int pending_write_;
    int pending_read_;

    time_t m_heart_beat;
    
    char send_buffer_[2048];
    int send_len_;
    recvMsg recv_msg_;
    
    time_t m_last_attack_time;
    int m_idle;

    std::list<std::string> send_queue_;

    int state_;    

    int encryption_;    // 0, 1 ,2 

    testCharData cdata_;
};

