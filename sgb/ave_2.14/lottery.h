#pragma once

#include "base_item.h"

#include "net.h"

#include "singleton.h"

using namespace net;

//ȫ����������
const int iLotteryNoticeNum = 20;

//���˼�¼����
const int iLotteryRecordNum = 5;

//÷���ה��_�ŵȼ�
const int iLotteryOpenLevel = 41;

//÷���ה����Ľ���
const int iLotteryCostGold = 10;

//10��÷���ה���Ҫ��vip�ȼ�
const int iLottery_10_vip_level = 3;

//50��÷���ה���Ҫ��vip�ȼ�
const int iLottery_50_vip_level = 6;

//÷��������ȡ��Ʒ
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
    //��ѯ��ɫ÷��������¼
    boost::shared_ptr<char_lottery_records> getCharRecords(int cid);
    //���ӽ�ɫ÷��������¼
    void addLotteryRecord(int cid, int count, const std::list<Item>& getlist);
    //����ȫ������
    void addLotteryNotice(const std::string& name, const std::string& what);
    //��ѯȫ������
    int queryLotteryNotice(json_spirit::Object& robj);
    //��ѯ���˹���
    int queryLotteryRecord(int cid, json_spirit::Object& robj);

    //����÷����������
    int setLotteryScore(int cid, int score, int total_score);

    //static lotteryMgr* getInstance();

private:
    //static lotteryMgr* m_handle;
    
    void save_notice();    //����ȫ������
    std::map<int, boost::shared_ptr<char_lottery_records> > m_char_lottery_records;

    json_spirit::Value m_notices_value;
    std::list<lottery_notice> m_notice_list;

    std::string m_noices_string;

};

