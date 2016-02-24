#pragma once

#include <string>
#include <map>
#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "combat_def.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "spls_const.h"

//����
struct Book
{
    int id;
    int quality;
    int hours;
    int uplevel;
    std::string name;
    std::string memo;
};

//�佫��Ϣ
struct charGeneral
{
    int cid;
    int gid;
    int pre_level;
    int pre_color;
    int cur_level;
    int cur_color;
};

//�佫ѵ������
struct generalTrainQue
{
    int pos;    //λ��
    int cid;    //��ɫid
    int type;    //�����б� 0 ��ͨ 1 ����
    int speed_time;    //���ٴ���

    boost::shared_ptr<charGeneral> general;    //ѵ�����佫

    time_t start_time;    //��ʼʱ��
    time_t end_time;    //����ʱ��

    int state;    //״̬0δ����1����2�ѱ�ռ��
    int start();

    boost::uuids::uuid _uuid;    //��ʱ��Ψһid

    int save();
    int getSpeedGold();
    int resetSpeedtime();
};

class TrainMgr
{
public:
    int load();
    //��ñ���
    boost::shared_ptr<Book> GetBook(int id);
    static TrainMgr* getInstance();
    //ˢ��ѵ������
    int updateBook(int mapid, boost::shared_ptr<Book>* books);

private:
    static TrainMgr* m_handle;
    std::map<int, boost::shared_ptr<Book> > m_base_books;        //�����б�
    int prop_silver[24];
    int prop_gold[24];
    int prop_best[24];
    int prop_sys[24];
};

