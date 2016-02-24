
#pragma once

#include <map>
#include <list>
#include <utility>
#include "json_spirit.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "item.h"
#include "net.h"

enum libao_type
{
    libao_type_level = 1,
};

struct baseLibao
{
    baseLibao()
    {
        m_libao_id = 0;
        m_spic = 0;
        m_quality = 0;
        m_type = 0;
        need_slot_num = 0;
        m_need_extra = 0;
        m_name = "";
        m_memo = "";
    }
    const json_spirit::Object& getObj() const;
    void getObj(json_spirit::Object& obj);
    const json_spirit::Array& getArray() const;
    void getArray(CharData& cdata, json_spirit::Array& list);
    void updateObj();
    
    json_spirit::Object m_obj;
    json_spirit::Array m_item_list;
    int m_libao_id;        //礼包id
    int m_spic;            //礼包图片
    int m_quality;        //礼包品质
    int m_type;         //礼包类型
    int m_notify_msg;   //礼包提示
    int need_slot_num;    //礼包占用仓库数量
    int m_need_extra;   //礼包条件参数
    std::string m_name;    //礼包名字
    std::string m_memo;    //礼包描述
    std::list<Item> m_list;        //礼包奖励内容
};

class libao : public item_base
{
public:
    libao(int id, baseLibao& base);
    virtual std::string name() const;
    virtual std::string memo() const;
    virtual void Save();
    uint16_t getSpic() const;
    int getQuality() const;
    int m_id;
    baseLibao& m_base;        //基础礼包
};

class libaoMgr
{
public:
    static libaoMgr* getInstance();
    baseLibao* getBaselibao(int id);
    std::string getlibaoMemo(int sid);
    int addLibao(CharData* pc, int libao_id);
    int openLibao(CharData* pc, int slot, json_spirit::Object& robj);
    int rewardLibao(CharData* pc, int libao_id, json_spirit::Object& robj);
    int getLibaoInfo(int libao_id, json_spirit::Object& robj);
    int isChengzhangLibao(int id);
    int getChengzhangLibao(int pos);
    int newLibaoId() {return ++m_libao_id;}
    
#ifdef QQ_PLAT
    baseLibao* getQQLevelLibao(int level);
    baseLibao* getQQDailyLibao(int yellow_level);
    baseLibao* getQQYearDailyLibao();
    baseLibao* getQQNewbieLibao();
    baseLibao* getQQSpecialLibao();
    void getCharQQLevelLibao(CharData& cdata, json_spirit::Array& list);
    bool allGetQQLevelLibao(CharData* pc);
    const json_spirit::Array& getQQDailyList() const
    {
        return m_qq_daily_list;
    }
#endif

private:
    void load();
    static libaoMgr* m_handle;
    std::map<int, baseLibao* > m_base_libaos;
    //成长礼包
    std::vector<int> m_chengzhang_Libaos;
    volatile int m_libao_id;
#ifdef QQ_PLAT
    //黄钻成长礼包
    std::map<int, boost::shared_ptr<baseLibao> > m_qq_levelLibaos;
    //黄钻新手礼包
    boost::shared_ptr<baseLibao> m_qq_newbie_libao;
    //黄钻专属礼包(武将)
    boost::shared_ptr<baseLibao> m_qq_special_libao;
    //黄钻每日礼包
    std::map<int,boost::shared_ptr<baseLibao> > m_qq_daily_libao;
    //黄钻年费每日礼包
    boost::shared_ptr<baseLibao> m_qq_year_daily_libao;

    json_spirit::Array m_qq_daily_list;
#endif
};

#ifdef QQ_PLAT
//查询黄钻界面：cmd:getYellowEvent
int ProcessQueryQQYellowEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询黄钻新手礼包 cmd：queryQQnewbieLibao
int ProcessQueryQQNewbieLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询黄钻每日礼包 cmd：queryQQDailyLibao
int ProcessQueryQQDailyLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询黄钻成长礼包 cmd：queryQQLevelLibao
int ProcessQueryQQLevelLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
#endif

