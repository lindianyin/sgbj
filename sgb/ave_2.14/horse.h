#pragma once

#include <string>
#include <map>
#include <list>
#include <boost/cstdint.hpp>

#include "boost/smart_ptr/shared_ptr.hpp"

#include <vector>
#include "json_spirit.h"
#include "spls_const.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>


const int iMaxHorse = iHorseStarsMax * (1 + iHorseTurnsMax) + 1;
const int iMaxAction = 2;//战马活动显示给玩家的战马数量
struct CharData;

struct baseHorse
{
    int id;            //战马唯一id
    int level;        //战马等级
    int quality;    //战马品质
    int star;        //战马星级
    int turn;        //战马转生次数
    int spic;        //战马图片
    int pugong;        //战马加成
    int pufang;        //战马加成
    int cegong;        //战马加成
    int cefang;        //战马加成
    int bingli;        //战马加成
    int need_exp;    //升星所需经验
    int add_exp_silver;    //银币培养增加的经验
    int add_exp_gold;        //金币培养增加的经验
    int need_prestige;//转生所需声望
    int fruit[3];    //战马对应的三个果子
    std::string name;    //名字
};

struct baseHorseFruit
{
    int id;            //果子唯一id
    int type;        //果子类型
    int pugong;        //果子加成
    int pufang;        //果子加成
    int cegong;        //果子加成
    int cefang;        //果子加成
    int bingli;        //果子加成
    int get_horseid;//可获取
    int eat_horseid;//可食用
    std::string name;    //名字
};

struct CharHorseFruit
{
    CharHorseFruit(int c_id)
    {
        cid = c_id;
        state = 0;
        start_time = 0;
        end_time = 0;
    }
    int cid;
    int state;//0未领取，1已经领取但不能服用，2已经领取并且可服用，3已经服用，4过期
    time_t start_time;
    time_t end_time;
    boost::shared_ptr<baseHorseFruit> fruit;
    int start();
    int stop();

    boost::uuids::uuid _uuid;    //定时器唯一id
};

struct CharHorseFruitAction
{
    CharHorseFruitAction(int c_id, int h_id)
    {
        cid = c_id;
        horse_id = h_id;
        horse_star = 0;
        horse_name = "";
    }
    int cid;
    int horse_id;
    int horse_star;
    std::string horse_name;
    std::vector<CharHorseFruit> fruits_list;
};

struct CharHorse
{
    CharHorse(int c_id)
    {
        cid = c_id;
        horseid = 0;
        exp = 0;
        horse = NULL;
        start_time = 0;
        end_time = 0;
        pugong = 0;
        pufang = 0;
        cegong = 0;
        cefang = 0;
        bingli = 0;
        _score = 0;
        _power = 0;
    }
    int cid;    //角色id
    int horseid;    //战马id
    int exp;    //当前经验
    time_t start_time;//果子活动开启时间
    time_t end_time;//果子活动结束时间
    int pugong;        //果子加成
    int pufang;        //果子加成
    int cegong;        //果子加成
    int cefang;        //果子加成
    int bingli;        //果子加成
    baseHorse* horse;
    std::vector<CharHorseFruitAction> action_list;//果子活动信息
    int _score;
    int _power;
    void updateNewAttack();
    int getNewScore(){return _score;}
    int getNewPower(){return _power;}
    int checkFruitState();
    int updateActionFruit();
    int save();
    int save_action();
};


class horseMgr
{
public:
    static horseMgr* getInstance();
    baseHorse* getHorse(int id);
    int getHorseInfo(CharData& cData, json_spirit::Object& robj);
    std::string NameToLink(int cid, std::string name, int quality);
    int trainHorse(CharData& cData, int type, int& cri_type, int& get_exp, int& dj_use);
    int turnHorse(CharData& cData, json_spirit::Object& robj);
    //设置战马等级
    int setHorse(CharData& cData, int horseId, int exp);
    //果子过期
    int fruitDone(int cid, int id);
    //战马果子操作
    int dealHorseFruit(CharData& cData, int type, int id);
    //获取下一个有果子的战马
    int getNextActionHorse(int horseid);
    int getHorseFruitsList(CharData& cData, json_spirit::Object& robj);
    boost::shared_ptr<baseHorseFruit> getBaseHorseFruit(int fruit_id);

    void setHorseTrainTimes(int total, int gold_times);
private:
    static horseMgr* m_handle;
    int reload();
    baseHorse base_horses[iMaxHorse];//转身次数*星级
    std::list<int> HorseFruitAction;//战马活动相关的战马id表
    std::map<int,boost::shared_ptr<baseHorseFruit> > base_fruits_list;
};


