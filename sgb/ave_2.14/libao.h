
#pragma once

#include <map>
#include <list>
#include <utility>
#include "json_spirit.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "base_item.h"
#include "bag.h"
#include "net.h"

struct baseLibao
{
    baseLibao()
    {
        m_libao_id = 0;
        m_spic = 0;
        m_quality = 0;
        m_type = 0;
        m_level = 0;
        m_attack = 0;
        m_stronghold = 0;
        need_slot_num = 0;
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
    int m_libao_id;        //���id
    int m_spic;            //���ͼƬ
    int m_quality;        //���Ʒ��
    int m_type;
    int m_level;
    int m_attack;
    int m_stronghold;
    int need_slot_num;    //���ռ�òֿ�����
    std::string m_name;    //�������
    std::string m_memo;    //�������
    std::list<Item> m_list;        //�����������
};

class libao : public iItem
{
public:
    libao(int id, baseLibao& base);
    virtual std::string name() const;
    virtual std::string memo() const;
    virtual void Save();
    uint16_t getSpic() const;
    int getQuality() const;
    int m_id;
    baseLibao& m_base;        //�������
};

class libao_mgr
{
public:
    static libao_mgr* getInstance();
    baseLibao* getBaselibao(int id);
    std::string getlibaoMemo(int sid);
    int getLevelLibao(int level);
    void getLevelLibaoList(CharData& cdata, json_spirit::Array& rlist);
    int getLevelLibaoState(CharData& cdata);
    int getChengzhangLibao(int pos);
    int isChengzhangLibao(int id);
    int getChengzhangState(CharData& cdata);
    int addLibao(CharData* pc, int libao_id);
    int newLibaoId() {return ++m_libao_id;}

#ifdef QQ_PLAT
    baseLibao* getQQLevelLibao(int level);
    baseLibao* getQQDailyLibao(int yellow_level);
    baseLibao* getQQYearDailyLibao();
    baseLibao* getQQNewbieLibao();
    baseLibao* getQQSpecialLibao();

    void getCharQQLevelLibao(CharData& cdata, json_spirit::Array& list);

    bool allGetQQLevelLibao(CharData* pc);

    void getQQDailyList(CharData&, json_spirit::Array& list, json_spirit::Array& list2);

    const json_spirit::Array& getQQDailyList() const
    {
        return m_qq_daily_list;
    }
#else
    baseLibao* getVipDailyLibao(int vip);
    baseLibao* getVipWeekLibao();
    baseLibao* getVipSpecialLibao();

    const json_spirit::Array& getVipDailyList() const
    {
        return m_vip_daily_list;
    }
    int vipWeekOrgGold() { return m_vip_week_libao_org_gold; }
    int vipWeekGold() { return m_vip_week_libao_gold; }
#endif

private:
    void load();
    static libao_mgr* m_handle;
    //std::map<int, boost::shared_ptr<libao> > m_libaos;
    std::map<int, baseLibao* > m_base_libaos;
    std::map<int, int> m_levelLibaos;
    //�ɳ����
    std::vector<int> m_chengzhang_Libaos;

#ifdef QQ_PLAT

    //����ɳ����
    std::map<int, boost::shared_ptr<baseLibao> > m_qq_levelLibaos;
    //�����������
    boost::shared_ptr<baseLibao> m_qq_newbie_libao;
    //����ר�����(�佫)
    boost::shared_ptr<baseLibao> m_qq_special_libao;
    //����ÿ�����
    std::map<int,boost::shared_ptr<baseLibao> > m_qq_daily_libao;
    //�������ÿ�����
    boost::shared_ptr<baseLibao> m_qq_year_daily_libao;

    json_spirit::Array m_qq_daily_list;
#else
    //V5ר�����(�佫)
    boost::shared_ptr<baseLibao> m_vip_special_libao;
    //VIPÿ�����
    std::map<int,boost::shared_ptr<baseLibao> > m_vip_daily_libao;
    //VIP�����
    boost::shared_ptr<baseLibao> m_vip_week_libao;
    int m_vip_week_libao_org_gold;
    int m_vip_week_libao_gold;

    json_spirit::Array m_vip_daily_list;
#endif

    volatile int m_libao_id;
};

#ifdef QQ_PLAT
//��ѯ������棺cmd:getYellowEvent
int ProcessQueryQQYellowEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ����������� cmd��queryQQnewbieLibao
int ProcessQueryQQNewbieLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ����ÿ����� cmd��queryQQDailyLibao
int ProcessQueryQQDailyLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ����ɳ���� cmd��queryQQLevelLibao
int ProcessQueryQQLevelLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

#else

//��ѯVIPÿ����� cmd��queryVipDailyLibao
int ProcessQueryVipDailyLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

#endif

//��ѯVIP��Ȩ���棺cmd:getVipBenefit
int ProcessQueryVipEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

