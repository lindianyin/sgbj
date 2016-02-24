#pragma once

#include <time.h>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include <map>
#include <list>
#include "json_spirit.h"
#include "spls_const.h"
#include <boost/uuid/uuid.hpp>
#include "net.h"

using boost::uint64_t;
using namespace std;
using namespace json_spirit;
using namespace boost;

struct corpsLottery;

enum corps_event_enum
{
    corps_event_add_exp = 1,
    corps_event_new_leader,
    corps_event_appointment,
    corps_event_donate_gold,
    corps_event_join,
    corps_event_fire,
    corps_event_leave,
    corps_event_explore,

    corps_jisi_1 = 100,
    corps_jisi_2,
    corps_jisi_3
};

enum corps_action_enum
{
    corps_action_jisi = 1,
    corps_action_sheji,
    corps_action_tansuo,
    corps_action_yanhui,
    corps_action_lottery,
    corps_action_fighting,
    corps_action_jt_boss
};

class CharData;

/********** 军团  **************/

struct corps_member
{
    int cid;                    //角色id
    int corps;                    //军团id
    int offical;                //职位 1 军团长  2 副军团长 0 普通成员
    int contribution;            //累计贡献
    int contribution_day;        //日贡献
    time_t join_time;            //加入时间
    time_t recruit_time;        //招人广播时间
    int ymsj_can_get;            //辕门射戟能获得的功勋
    boost::shared_ptr<CharData> cdata;

    corps_member()
    {
        cid = 0;
        corps = 0;
        offical = 0;
        contribution = 0;
        contribution_day = 0;
        join_time = 0;
        recruit_time = 0;
        ymsj_can_get = 0;
    }
    int save();
};

struct corps_application
{
    int cid;        //角色id
    int corps;        //申请军团id
    time_t app_time;        //申请时间
    std::string message;    //申请信息
    boost::shared_ptr<CharData> cdata;
};

struct corpsEvent
{
    time_t inputTime;
    int corps;
    int cid;
    int etype;            //有打算修改 char_corps_events表的结构，有时间优化
    std::string name;
    std::string msg;
};

struct corpsYanhuiPos
{
    int _id;    //参加的角色id
    int _type;    //类别 1 正常 2 协助
    int _spic;    //角色头像
    std::string _name;    //角色名
};

struct base_corps_action
{
    int _id;                    //活动id
    int _needlevel;                //活动需要军团等级
    std::string _name;            //活动名称
    std::string _award_memo;    //活动奖励描述
    std::string _memo;            //活动描述
};

struct splsCorps;

//辕门射戟
struct corps_ymsj
{
    //int m_id;
    boost::shared_ptr<corps_member> m_char[2];
    int m_choice[2];
    time_t m_join_time[2];

    splsCorps& m_corps;
    
    int join(int pos, boost::shared_ptr<corps_member> pc, json_spirit::Object& robj);
    int leave(int cid);
    int choose(int cid, int c);

    void _Done();
    int Done();

    int timeout();

    void refresh();

    time_t next_time;    //下次开启时间

    boost::uuids::uuid m_choose_timer[2];

    boost::uuids::uuid m_done_timer;

    void load(const std::string& data);

    void choose_timer(int pos);

    void win_timer(int pos);

    void Save();

    corps_ymsj(splsCorps& cp);
};

struct splsCorps
{
public:
    splsCorps(int id, int camp, int level, int exp, int exp_week, int memberLimit, int assistantLimit, const std::string& name, const std::string& flag,
                const std::string& bulletin, const std::string& introduction, int createrId, const std::string& qqGroup, const std::string& ymsj_data);
    int load();
    corps_member* addMember(int cid);
    boost::shared_ptr<corps_member> getMember(int cid);
    int setOffical(int cid, int offical);
    int setNewLeader(int cid);
    int cancelApplication(int cid);
    bool haveApplication(int cid);
    int removeMember(int cid, int type = 0, const std::string& who = "");
    void addEvent(int cid, const std::string& name, const std::string& name2, int type);
    void save();
    void sort();
    int jiSi(CharData* pc, int type);        //成员进行祭祀
    int JoinYanhui(CharData* pc, int pos);    //参加宴会
    int queryYanhuiPerson();                //查询宴会人数
    int inviteSomeone(CharData* pc, int pos);    //邀请路人
    int checkYanhuiSuccess();

    //每日重置本日貢獻
    void dailyReset();
    friend bool compare_corps(boost::shared_ptr<splsCorps> a, boost::shared_ptr<splsCorps> b);
    friend class corpsMgr;
//private:
    int _id;            //唯一id
    int _level;            //军团等级
    int _camp;            //军团阵营
    int _rank;            //排行
    int _maxExp;        //经验上限
    int _exp;            //当前经验
    int _expWeek;        //本周经验
    int _creater;        //创建者
    int _memberNums;    //成员数
    int _memberLimit;    //成员上限
    int _assistantLimit;//副团人数上限
    std::string _qqGroup;//qq群号
    std::string _name;    //军团名
    std::string _flag;    //旗号

    std::string _strBulletin;        //公告
    std::string _strIntroduction;    //对外介绍

    time_t _createTime; //创建时间
    int m_auto_recruit_msg; //自动发布招募信息次数
    time_t m_auto_recruit_msg_time;

    boost::shared_ptr<corps_member> _leader;    //军团长

    std::list<boost::shared_ptr<corps_member> > _assistant;    //副军团长

    std::map<int, boost::shared_ptr<corps_member> > _members;    //成员
    std::list<boost::shared_ptr<corps_member> > _members_list;    //成员

    std::map<int, boost::shared_ptr<corps_application> > _applications;        //申请列表

    std::list<boost::shared_ptr<corpsEvent> > _event_list;    //事件列表
    std::list<boost::shared_ptr<corpsEvent> > _js_event_list;    //祭祀事件列表

    corpsYanhuiPos _corps_yanhui[iCorpsYanhuiMax];    //军团宴会

    boost::shared_ptr<corpsLottery> m_corpsLottery;

    std::string m_ymsj_data;
    corps_ymsj m_ymsj;    //军团辕门射戟
};

struct baseCorps
{
    int _maxExp;
    int _memberLimit;
    int _assistantLimit;
};

class corpsMgr
{
public:
    //初始化
    int init();
    //军团活动按钮
    void getAction(CharData* pc, json_spirit::Array& blist);
    //创建军团
    int createCorps(int cid, const std::string& name, json_spirit::Object& robj);
    //查看审核信息
    int getApplications(int cid, int corps, int page, int nums_per_page, json_spirit::Object& robj);
    //通过审核
    int acceptApplication(int cid, int corps, int tcid, json_spirit::Object& robj);
    //拒绝审核
    int rejectApplication(int cid, int corps, int tcid, json_spirit::Object& robj);
    //拒绝所有审核
    int rejectAllApplication(int cid, int corps, json_spirit::Object& robj);
    //获取军团名字
    std::string getCorpsName(int corps);
    //获取军团等级
    int getCorpsLevel(int corps);

    //查询军团信息
    int getCorpsInfo(int cid, int corps, json_spirit::Object& robj);
    //查询本军团信息
    int getCorpsDetail(int cid, int corps, json_spirit::Object& robj);
    //查询军团成员
    int getCorpsMembers(int cid, int corps, json_spirit::Object& robj);
    //查询军团日志
    int getCorpsEvents(int cid, int corps, json_spirit::Object& robj);
    //退出军团
    int quitCorps(int cid, int corps, json_spirit::Object& robj);
    //提交申请
    int submitApplication(int cid, int corps, json_spirit::Object& robj);
    //取消申请
    int cancelApplication(int cid, int corps, json_spirit::Object& robj);
    //任命
    int appointment(int cid, int corps, int tcid, int offical, json_spirit::Object& robj);
    //申请改变军团长
    int changeLeader(int cid, int corps, json_spirit::Object& robj);
    //发布招人公告
    int recruitMember(int cid, int corps, json_spirit::Object& robj);
    //开除成员
    int fireMember(int cid, int corps, int tcid, json_spirit::Object& robj);
    //解散军团
    int dissolve(int cid, int corps, json_spirit::Object& robj);
    //查询副军团长信息
    int getAssistants(int cid, int corps, json_spirit::Object& robj);
    //查询军团列表
    int getCorpsList(int cid, int pageNo, int numsPerPage, json_spirit::Object& robj);
    //搜索军团
    int searchCorps(int cid, const std::string& leader, const std::string& corpsName, json_spirit::Object& robj);
    //查询已申请军团
    int getApplicationCorps(int cid, json_spirit::Object& robj);
    //更新排名
    int updateRank();
    //增加军团事件
    int addEvent(CharData* pc,int type,int param1,int param2);
    //军团等级对应的经验/人数上限
    int getBaseCorps(int level, int& exp, int& numsLimit, int& assistLimit);
    //设置军团信息
    int setCorpsInfo(int cid, int corps, const std::string& memo,
                const std::string& intro, const std::string& qq, json_spirit::Object& robj);

    int getCorpsActionLevel(int id);
    int getCorpsActionList(CharData* pc, json_spirit::Object& robj);

    //删除角色
    int deleteChar(int cid);

    //查询祭祀信息
    int getJisiInfo(CharData* pc, json_spirit::Object& robj);

    //祭祀
    int jiSi(CharData* pc, int type);

    //查看宴会信息
    int getYanhui(CharData* pc, json_spirit::Object& robj);

    //召集
    int zhaoJi(CharData* pc);
    //加入宴会
    int JoinYanhui(CharData* pc, int pos);
    //邀请路人加入宴会
    int inviteSomeone(CharData* pc, int pos);

    //是否加入宴会，返回位置
    int inYanhui(CharData* pc);
    std::string zhaojiMsg() {return m_strZhaoMsg;}

    //查找军团
    splsCorps* findCorps(int corps);

    //设置军团等级/经验
    void setCorpsExp(int corps, int level, int exp, int weekExp);
    //设置玩家贡献
    void setCharContribution(CharData& cdata, int contribute);

    //每周重置本周获得经验
    void weekReset();
    //每日重置本日貢獻
    void dailyReset();

    //为了修改老数据，更新军团人数上限
    void updateCorpsMemberLimit();
    
    void openJisiEvent(int fac, int last_mins);
    int getJisiFac();

    //检查机器人军团
    void checkRobotCorps();
    //建立机器军团
    void createRobotCorps();
    //设置机器军团团长
    void setRobotCorpsLeader(int id, int cid);
    //开启关闭机器人军团 1开启，0关闭
    void openRobotCorps(int state);

    std::string randomCorpsName();
    std::string randomCorpsMemo();
    
    static corpsMgr* getInstance();
    
private:
    static corpsMgr* m_handle;
    std::map<int, boost::shared_ptr<splsCorps> > m_corps_maps;

    std::map<const std::string, boost::shared_ptr<splsCorps> > m_corps_maps2;

    std::list<boost::shared_ptr<splsCorps> > m_corps_list;
    int max_corps;

    std::vector<baseCorps> m_base_corps;
    std::string m_strZhaoMsg;
    std::vector<base_corps_action> m_base_corps_action;
    int max_corps_action;

    int m_jisi_event_endtime;
    int m_jisi_event_fac;

    int m_total_robot_corps;
    bool m_open_robot_corps;

    std::vector<std::string> m_robot_corps_name;
    std::vector<std::string> m_robot_corps_memo;
};

//辕门射戟加入
int ProcessCorpsYmsjJoin(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//辕门射戟选择
int ProcessCorpsYmsjChoose(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//辕门射戟信息
int ProcessCorpsYmsjInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//辕门射戟领取奖励
int ProcessCorpsYmsjAward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//辕门射戟重置
int ProcessCorpsYmsjRefresh(json_spirit::mObject& o);

//辕门射戟超时胜利
int ProcessCorpsYmsjWin(json_spirit::mObject& o);

//辕门射戟选择
int ProcessCorpsYmsjChoose(json_spirit::mObject& o );

//检查机器人军团
int ProcessRobotCorps(json_spirit::mObject& o);

