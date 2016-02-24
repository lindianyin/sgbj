#pragma once

#include "base_item.h"

#include "net.h"

#include "singleton.h"

using namespace net;

//全服公告数量
const int iLotteryNoticeNum = 20;

//个人记录数量
const int iLotteryRecordNum = 5;

//梅花易數開放等級
const int iLotteryOpenLevel = 41;

//梅花易數消耗金幣
const int iLotteryCostGold = 10;

//10次梅花易數需要的vip等級
const int iLottery_10_vip_level = 3;

//50次梅花易數需要的vip等級
const int iLottery_50_vip_level = 6;

//梅花易数抽取奖品
int ProcessLottery(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
std::string NameToLink(int cid, int gid, std::string name);

struct char_lottery_records
{
    int cid;
    std::string record_list_string;
    json_spirit::Value record_list;        /* [v0]
                                                v0:{count,getlist:[string1,string2,string3]}
                                                                        */
    void add(int count, const std::list<Item>& getlist);
    void save();
};

struct lottery_notice
{
    std::string name;
    std::string what;
};

class lotteryMgr
{
public:
    lotteryMgr();
    void reload();
    std::string NameToLink(int cid,int gid, std::string name);
    //查询角色梅花易数记录
    boost::shared_ptr<char_lottery_records> getCharRecords(int cid);
    //增加角色梅花易数记录
    void addLotteryRecord(int cid, int count, const std::list<Item>& getlist);
    //增加全服公告
    void addLotteryNotice(const std::string& name, const std::string& what);
    //查询全服公告
    int queryLotteryNotice(json_spirit::Object& robj);
    //查询个人公告
    int queryLotteryRecord(int cid, json_spirit::Object& robj);

    //设置梅花易数积分
    int setLotteryScore(int cid, int score, int total_score);

    //static lotteryMgr* getInstance();

private:
    //static lotteryMgr* m_handle;
    
    void save_notice();    //保存全服公告
    std::map<int, boost::shared_ptr<char_lottery_records> > m_char_lottery_records;

    json_spirit::Value m_notices_value;
    std::list<lottery_notice> m_notice_list;

    std::string m_noices_string;

};

