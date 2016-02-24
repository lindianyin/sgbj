#pragma once

//阵营战头文件
#include <string>
#include <map>
#include <list>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <time.h>
#include "json_spirit.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "utils_all.h"
#include "spls_const.h"
#include "data.h"

class ChatChannel;
class CharData;
struct CharGeneralData;
class Combat;
struct OnlineCharactor;

#ifdef CAMP_RACE_TWO_QUEUE
const int iCampRaceHighQueSize = (iMaxCharLevel - 60)/10 + 1;
#else
const int iCampRaceQueSize = iMaxCharLevel/10;
#endif

enum campRaceState
{
    camp_race_state_idle = 1,
    camp_race_state_wait = 2,
    camp_race_state_fighting = 3,
    camp_race_state_leave = 4
};

enum campRaceReportType
{
    camp_race_report_win = 1,
    camp_race_report_end,
    camp_race_report_skip,
    camp_race_report_fail,
};

struct campRaceReport
{
    int _type;    //类型 1打败，2终结 3 战败 4 轮空
    int _id1;
    int _id2;
    int _camp1;
    int _camp2;
    std::string _name1;
    std::string _name2;
    int _silver;
    int _prestige;
    int _win_times;
};

struct campRaceReportList
{
    std::list<campRaceReport> _report_list;

    void addReport(int type, int id1, int id2, const std::string& name1, const std::string& name2, int silver, int prestige, int win_times, int, int);
    int getReport(json_spirit::Array& reportArray, int cid, bool self);
};

struct charCampRace
{
    int _cid;    //角色id

    int _wins;    //胜利次数
    int _loses;    //失败次数
    int _skips;//跳过次数
    int _max_continue_wins;    //最高连杀
    int _cur_continue_wins;    //当前连杀

    int _prestige;    //累计获得声望
    int _silver;    //累计获得银币

    int _state;            // 1 空闲 2 等待  4 离开
    int _in_combat;    //玩家关闭战报重新打开阵营界面了 0 ，如果是1 表示还在看战斗

    time_t _idle_time;    //空闲时间

    boost::shared_ptr<CharData> _cdata;
    boost::shared_ptr<OnlineCharactor> _onlineChar;

    int _zhen_type;
    boost::shared_ptr<CharGeneralData> m_generals[9];      //各位置的武将
    int m_generals_hp[9]; //各位置的武将的受伤血量 -1表示死亡

    //campRaceReportList _reports;    //个人战报
};

struct campRaceWaitQue
{
#ifdef CAMP_RACE_TWO_QUEUE    //是否划分为两个队列
    //高等級陣營戰等待隊列
    std::vector<boost::shared_ptr<charCampRace> > m_camp_wait_que_high[iCampRaceHighQueSize];
    //低等級陣營戰等待隊列
    std::vector<boost::shared_ptr<charCampRace> > m_camp_wait_que_low[4];    // 30-39，40-49，50-59，60
#else
    //陣營戰等待隊列
    std::vector<boost::shared_ptr<charCampRace> > m_camp_wait_que[iCampRaceQueSize];    // 30-39，40-49，50-59，60-69,70-79...
#endif
    //随机选出战的人
    charCampRace* _getRacer(bool high);

#ifdef CAMP_RACE_TWO_QUEUE
    size_t m_total_high;
    size_t m_total_low;
#else
    size_t m_total;
#endif

    void add(boost::shared_ptr<charCampRace> sp);
    void clear();
};

struct campRaceWinsRanking
{
    int cid;
    int wins;
};

const int iCampRaceRankingsPage = 3;        //排行榜3页
const int iCampRaceRankingsPerPage = 8;    //每页8个
const int iCampRaceRankingsCount = iCampRaceRankingsPage*iCampRaceRankingsPerPage;

struct rankings_event;

struct campRaceWinsRankings
{
    int size;
    int min_wins;
    std::map<int, int> winsMap;
    std::list<campRaceWinsRanking> winsList;

    int rankings[iCampRaceRankingsCount];
    std::string strRangkingsPages[iCampRaceRankingsPage];
    json_spirit::Array aRankingsPages[iCampRaceRankingsPage];

    void updateRankings(std::map<int, int>&);
    //更新排行榜活动中的cid字段
    void updateRankingsEvent(rankings_event* pE);

    std::string getRankings(int page = 1);
};

class campRaceMgr
{
public:
    campRaceMgr();
    //加载
    int reload();
    //加入阵营战
    int joinRace(boost::shared_ptr<CharData>, boost::shared_ptr<OnlineCharactor> ou, json_spirit::Object& robj);
    //离开阵营战
    int leaveRace(CharData* pc, boost::shared_ptr<OnlineCharactor>);
    //参战/取消参战
    int setFight(CharData* pc, bool enable, json_spirit::Object& robj);
    //鼓舞
    int inspire(CharData* pc, int type, json_spirit::Object& robj);    //鼓舞 type=1银币鼓舞，type=2 金币鼓舞
    int getInspire(CharData* pc);

    //查询战报 type=1所有战报 2=自己的战报
    int queryHistory(CharData* pc, int type, json_spirit::Object& robj);
    //查询双方参战人员
    int queryCharList(json_spirit::Object& robj);
    //查询积分
    int queryPoints(json_spirit::Object& robj);
    //查询连胜信息
    int queryConsecutiveVictory(json_spirit::Object& robj);
    //查询自己的阵营战信息
    int querySelfInfo(CharData* pc, json_spirit::Object& robj);
    //查询阵营战结束时间
    int queryEndTime(json_spirit::Object& robj);
    //查询鼓舞信息
    int queryInspireInfo(CharData* pc, json_spirit::Object& robj);
    //捉对厮杀
    int Race();
    
    int combatResult(Combat* pCombat);    //阵营战斗结束

    int open(int);    //开启

    int close();//结束
    //结束时间
    int endTime();
    //最大连胜纪录
    void getMaxWin(std::string& name, int& times, int& camp);
    //双方积分
    void getScore(int& score1, int& score2);
    //参加者列表
    int getRacers(int camp, json_spirit::Array& array, json_spirit::Array& array2);
    //活动是否开启
    void getAction(CharData* pc, json_spirit::Array& blist);
    int getActionMemo(std::string& memo);

    campRaceReportList& getReportList1();
    campRaceReportList& getReportList2();

    boost::shared_ptr<charCampRace> getChar(int camp, int cid);
    void broadMsg(const std::string& msg);
    void broadMsg(int level_from, int level_to, const std::string& msg);

    std::string getRankings(int page = 1);
    //更新排行榜活动中的cid字段
    void updateRankingsEvent(rankings_event* pE);

    bool isOpen();

    std::string openTime();

    stand_in_mob m_stand_in_mob;

    void getJoinCount(int camp, int& count1, int& count2);

    static campRaceMgr* getInstance();
private:
    int _join(boost::shared_ptr<CharData> cdata, boost::shared_ptr<OnlineCharactor> ou);
    //两两战斗
    int _Race(charCampRace* a, charCampRace* d, json_spirit::Array&);

    void updateWaitQue(json_spirit::Array&);
    //轮空处理
    void noMatch(charCampRace*);
    //额外奖励并发送邮件
    void giveExtraAndMail(int camp, int type);    // 0 失败没有奖励  1 平局一半奖励 2 胜利全部奖励

    static campRaceMgr* m_handle;

    std::map<int, boost::shared_ptr<charCampRace> > m_data_map[2];

    std::map<int, boost::shared_ptr<charCampRace> > m_total_data_map;
    //官府等待隊列
    campRaceWaitQue m_guanfu_que;
    //綠林等待隊列
    campRaceWaitQue m_lvlin_que;

    std::map<int, int> m_inspire_map;            //鼓舞数据

    std::string _str_will_open_msg;
    std::string _str_have_closed;
    std::string _open_time;

    int _open_state;

    openRules _open_rules;    //开放规则，可以有多个
    openRule* _last_rule;    //上次开放规则

    time_t _start_time;    //开启时间戳
    time_t _end_time;        //结束时间戳
    boost::uuids::uuid _uuid;    //定时器唯一id - 关闭阵营战的定时器
    
    boost::uuids::uuid _uuid_race;    //定时器唯一id - 定时匹配阵营战对手的定时器

    std::string _max_winner;    //最多连胜玩家
    int _max_win_times;        //最多连胜次数
    int _max_win_camp;

    int _score[2];                //双方积分
    
    campRaceReportList _reports;    //低等级全体战报
    campRaceReportList _reports2;    //高等级全体战报

    campRaceWinsRankings m_rankings;    //连胜排行

};

