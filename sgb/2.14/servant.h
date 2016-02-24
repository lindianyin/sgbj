
#ifndef _SERVANT_H_
#define _SERVANT_H_

#include <vector>
#include <list>
#include <map>
#include <boost/cstdint.hpp>

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "data.h"
#include "combat.h"

enum servant_type
{
    type_free = 0,
    type_master = 1,
    type_servant = 2
};

struct baseInteract
{
    baseInteract()
    {
        m_id = 0;
        m_type = 0;
        m_name = "";
        m_memo = "";
        m_memo2 = "";
    }
    int m_id;//互动id
    int m_type;//互动类别1-主人2-家丁
    std::string m_name;
    std::string m_memo;
    std::string m_memo2;
};

struct charServant
{
    charServant()
    {
        m_cid = 0;
        m_type = 0;
        m_catch_time = 0;
        m_buy_catch_time = 0;
        m_interact_time = 0;
        m_rescue_time = 0;
        m_sos_time = 0;
        m_be_sos_time_f = 0;
        m_be_sos_time_c = 0;
        m_resist_time = 0;
        m_buy_resist_time = 0;
        m_get_num = 0;
        m_interact_cooltime = 0;
        m_master_id = 0;
        m_output = 0;
        m_left = 0;
        m_start_time = 0;
        m_end_time = 0;
        m_fight_with = 0;
    }
    int m_cid;    //玩家id
    int m_type;    //身份类别0-自由身，1-主人，2-家丁
    int m_catch_time;        //抓捕次数
    int m_buy_catch_time;    //今日购买抓捕次数
    int m_interact_time;    //互动次数
    int m_rescue_time;        //解救次数
    int m_sos_time;        //求救次数
    int m_be_sos_time_f;        //被好友求救次数
    int m_be_sos_time_c;        //被军团求救次数
    int m_resist_time;        //反抗次数
    int m_buy_resist_time;//今日购买反抗次数
    int m_get_num;            //今日获得的玉石数量，从家丁获得
    time_t m_interact_cooltime;//互动冷却完成时间

    int m_master_id;        //主人id
    int m_fight_with;        //正和谁战斗

    std::list<int> m_servant_list;//家丁
    std::list<int> m_loser_list;//手下败将
    std::list<int> m_enemy_list;//夺仆之敌
    std::list<std::string> m_event_list;//事件
    std::list<int> m_rescue_list;//解救列表

    int m_output;//劳动产出(当身份是奴隶)
    int m_left;//剩余家丁收入次数
    time_t m_start_time;//被抓捕干活开始时间
    time_t m_end_time;//干活结束时间
    int start();
    int done();
    int stop();

    boost::uuids::uuid _uuid;    //定时器唯一id
};

class servantMgr
{
public:
    int reload();
    int getAction(CharData* cdata, json_spirit::Array& elist);
    int Save_data(int cid);
    int Save_enemy_list(int cid);
    int Save_loser_list(int cid);
    int Save_event_list(int cid);
    int Save_rescue_list(int cid);
    int Done(int cid);
    static servantMgr* getInstance();
    int ServantListInfo(int cid, json_spirit::Object& robj);
    bool CheckServant(int master_id, int servant_id);
    bool CheckRescue(int cid, int rescue_cid);
    bool RemoveServant(int master_id, int servant_id);
    bool CheckCanList(int master_id, int cid);
    bool RemoveFromCanList(int master_id, int cid);
    int addServantEvent(int cid, std::string msg);
    int addServantEnemy(int cid, int eid);
    int addServantLoser(int cid, int eid);
    int getServantType(int cid);
    int getServantInfo(CharData& cData, json_spirit::Object& robj);
    int getServantEventsList(CharData& cData, json_spirit::Object& robj);
    int getServantList(CharData& cData, int type, int purpose, json_spirit::Object& robj);
    int getMax(CharData& cData);
    int DealServantAction(CharData& cData, int type, int extra_type, int id, int aid, json_spirit::Object& robj);
    int getInteractionList(CharData& cData, json_spirit::Object& robj);
    boost::shared_ptr<baseInteract> getInteraction(int id);
    boost::shared_ptr<charServant> GetCharServant(int cid);
    boost::shared_ptr<charServant> open(int cid);
    void deleteChar(int cid);
    void resetAll();
    int combatResult(Combat* pCombat);
    void setFactor(int fac);
    void servantRealReward(int &get);
private:
    static servantMgr* m_handle;
    std::map<uint64_t,boost::shared_ptr<charServant> > m_charServant_list;
    std::list<boost::shared_ptr<baseInteract> > m_base_Interact_list;

    //家丁收益系数
    int m_servant_factor;
};

#endif

