#pragma once

#include "bag.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include "attribute.h"

#define MAX_BAOSHI_LEVEL 13

#define MAX_BAOSHI_COUNT 99

const int iFreeYushi = 3;

struct baseNewBaoshi
{
    int type;    //��ʯ���
    int atype;    //�������

    int cost;    //������Ҫ����ʯ
    int goldCost;//������Ҫ�Ľ��
    int silverCost;//������Ҫ������

    std::string name;    //��ʯ����
    std::string memo;    //��ʯ����
    int values[MAX_BAOSHI_LEVEL];        //12����ʯ��Ӧ��Ч��

    int qualitys[MAX_BAOSHI_LEVEL];    //12����ʯ��Ӧ����ɫ
    //int exps[MAX_BAOSHI_LEVEL];            //12����ʯ��Ҫ�ľ���

    baseAttribute& baseAttr;

    baseNewBaoshi(baseAttribute& a)
    :baseAttr(a)
    {
    }

    void toObj(int level, json_spirit::Object& obj);
    std::string Name_to_Link(int level);
};

class newBaoshi : public iItem
{
public:
    newBaoshi(int id, baseNewBaoshi& base, int level, int exp);

    int level() const;
    int levelup();
    int value () const;
    bool canLevelup() const;
    virtual std::string memo() const;
    virtual std::string name() const;
    virtual void Save();
    void setLevel(int);
    const json_spirit::Object& getObj() const;
    void updateObj();
    int getQuality() const {return m_quality;}
    int32_t maxCount() const {return MAX_BAOSHI_COUNT;}
    virtual int32_t sellPrice() const;
    
    const baseNewBaoshi& m_base;        //������ʯ    
private:
    int m_level;    //��ʯ�ȼ�
    int m_value;    //����Ч��
    //int m_exp;        //��ʯ��ǰ����
    int m_quality;    //Ʒ��

    json_spirit::Object m_obj;

    std::string m_link_name;    //�����ӵ�����
};

struct baoshi_goods
{
    int type;    //��ʯ���
    int level;    //��ʯ�ȼ�
    int goldCostMax;//ԭ��
    int goldCost;//������Ҫ�Ľ��
    int counts;//��ʯ����
};

class newBaoshiMgr
{
public:
    newBaoshiMgr();
    ~newBaoshiMgr();
    baseNewBaoshi* getBaoshi(int type);
    //�ϲ���ʯ
    int combine(int id1, int id2);
    //����ʯ
    int buyBaoshi(CharData* pc, int& type, json_spirit::Object& robj, int btype=1, int auto_buy = 0);
    //��¡��ʯ
    boost::shared_ptr<iItem> cloneBaoshi(int type, int level = 1, int count = 1);
    //���ٱ�ʯ
    void destroyBaoshi(int id);
    //��ѯ��ʯ
    boost::shared_ptr<iItem> queryBaoshi(int id);
    //���ӱ�ʯ
    int addBaoshi(CharData* pc, int type, int level = 1, int count = 1, int reason = 1);
    //ע�ᱦʯ
    void registerBaoshi(int id, boost::shared_ptr<iItem> bs);

    int newBaoshiId() {return ++m_baoshi_id;}

    //��ʯ�̵�
    void load_shop();
    void refresh_shop();
    void save_shop(int change_pos);
    int shop_baoshi(CharData* pc, int pos, json_spirit::Object& robj);
    int baoshishopInfo(json_spirit::Object& robj);
private:
    std::vector<baseNewBaoshi*> m_baseBaoshis;
    std::map<int, boost::shared_ptr<iItem> > m_baoshiMap;

    volatile int m_baoshi_id;
    
    baoshi_goods m_goods[6];//�޹���ʯ�б�
};

