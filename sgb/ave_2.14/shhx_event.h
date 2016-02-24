
#pragma once

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include <map>
#include <time.h>

struct CharData;

//�챦��
int open_box(CharData* pc, int level);

//ˮ䰺�����ֵ�
class shhx_recharge_event
{
    
};

//��ɫ���ѻ����
struct char_cost_event
{
    int _cid;            //��ɫid
    int _total_cost;    //��ۼ����ѽ��
    int _next;            //��һ�����Եñ���
    int _next_level;    //��һ������ȼ�
    int _can_get;        //������ȡ�ı���
    int _can_gets[5];    //ÿ��������ı���

    boost::shared_ptr<CharData> _cdata;

    std::string open_box();
    void save();
};

//ˮ䰺������ѻ
class shhx_cost_event
{
public:
    void load();        //���ػ
    int query_event(int cid, json_spirit::Object& robj);
    int update_cost_event(int cid, int gold_cost);
    int openBox(int cid, std::string&);
    boost::shared_ptr<char_cost_event> getChar(int cid);

    int leftSecs();

    static int get_next(int& gold);
    static shhx_cost_event* getInstance();
private:
    static shhx_cost_event* m_handle;
    bool m_enable;

    time_t m_start_time;
    time_t m_end_time;

    std::map<int, boost::shared_ptr<char_cost_event> > m_char_datas;
};

//��ɫ�佫���������
struct char_general_upgrade_event
{
    int _cid;            //��ɫid
    int _total_score;    //��ۼƻ���

    int _can_get;        //������ȡ�ı���
    int _geted;            //�Ѿ���ȡ����

    boost::shared_ptr<CharData> _cdata;

    std::string open_box();
    void save();
};

//�佫�����
class shhx_generl_upgrade_event
{
public:
    void load();        //���ػ
    int add_score(int cid, int score);
    int openBox(int cid, std::string&);
    int query_event(int cid, json_spirit::Object& robj);
    int leftSecs();

    boost::shared_ptr<char_general_upgrade_event> getChar(int cid);

    static shhx_generl_upgrade_event* getInstance();
private:
    static shhx_generl_upgrade_event* m_handle;
    bool m_enable;

    time_t m_start_time;
    time_t m_end_time;

    std::map<int, boost::shared_ptr<char_general_upgrade_event> > m_char_datas;
    
};

