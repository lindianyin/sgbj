
#pragma once

#include <time.h>
#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"

#ifdef JP_SERVER
const int supply_goods_max = 18;
const int supply_goods_data[supply_goods_max][2] =
{
    {1000, 25000},
    {1200, 25000},
    {1500, 25000},
    {1800, 25000},
    {2000, 30000},
    {2200, 30000},
    {2500, 30000},
    {2800, 30000},
    {3000, 40000},
    {3200, 40000},
    {3500, 40000},
    {3800, 40000},
    {4000, 50000},
    {4200, 50000},
    {4500, 50000},
    {4800, 50000},
    {5000, 60000},
    {10000, 60000}
};
const int supply_goods_gailv[supply_goods_max] =
{
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,2,2
};
#endif

struct CharData;
//�����̵���Ʒ
struct baseShopGoods
{
    baseShopGoods()
    {
        type = 0;
        id = 0;
        num = 0;
        spic = 0;
        org_price = 0;
        price = 0;
        quality = 0;
        up_quality = 0;
        use = 0;
        need_notify = false;
        name = "";
        memo = "";
    }
    int type;// 1 ��Դ(��ҡ�����)2����3Ӣ�۱���4����,6��ʯ 7װ��
    int id;
    int num;
    int spic;
    int price;
    int org_price;
    int quality;
    int up_quality;
    int use;
    bool need_notify;
    std::string name;
    std::string memo;
};

struct charShopGoods
{
    bool canbuy;
    boost::shared_ptr<baseShopGoods> baseGoods;
};

struct charShop
{
    CharData& m_charData;
    time_t m_refresh_time;            //��Ʒˢ��ʱ��
    charShopGoods m_goods[6];        //��Ʒ�б�

    charShop(CharData& c)
    :m_charData(c)
    {
    }
    int buy(int pos);                //������Ʒ
    int refresh(int type);            //ˢ����Ʒ
    //��ѯ��Ʒ�б�
    int getList(json_spirit::Array& glist);
    //������Ʒ
    void Load();
    
    void Save(int pos);
};

class shopMgr
{
public:
    shopMgr();
    //��ѯ�����¼
    int getList(json_spirit::Array& glist);
    //ˢ���̵�
    int refresh(charShop& shop);
    //��ʼ����Ʒ
    int initGoogs(int type, int id, bool cb, charShopGoods& goods);
    //��ӹ����¼
    int addShopRecord(const std::string& who, int type, const std::string& what, int count, int quality);

    void updateShopPrice(int discount);
    void getRandomList(int level, std::vector<boost::shared_ptr<baseShopGoods> >& list, int min_q=0, int max_q=0);

private:
    std::map<int,int> m_gem_type_map;
    int m_mat_start_id;
    int m_card_start_id;
    int m_equipt_start_id;
    std::vector<boost::shared_ptr<baseShopGoods> > m_mat_list;        //ȫ�������б�
    std::vector<boost::shared_ptr<baseShopGoods> > m_card_list;    //�����б�
    std::vector<boost::shared_ptr<baseShopGoods> > m_equip_list;    //װ���б�
    std::vector<boost::shared_ptr<baseShopGoods> > m_baoshi_list;    //��ʯ�б�
    boost::shared_ptr<baseShopGoods> m_supply_goods;    //����
    boost::shared_ptr<baseShopGoods> m_matitie_goods;    //������
    #ifdef JP_SERVER
    std::vector<boost::shared_ptr<baseShopGoods> > m_supply_list;    //�����б�
    #endif

    json_spirit::Value m_notices_value;
};

