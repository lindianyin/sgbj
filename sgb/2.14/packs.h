
#pragma once

#include <string>
#include <map>
#include <list>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"

#include "base_item.h"

using namespace json_spirit;

//���ͷ�ļ�

class CharData;

//��������
struct instancePacks
{
    int _id;            //�������id
    int _bid;            //�������id

    int _vip;            //��Ҫ��vip�ȼ�
    int _prestige;        //��Ҫ������
    int _level;            //��Ҫ�Ľ�ɫ�ȼ�
    bool _needCode;    //�Ƿ���Ҫ������


    bool _bAlwaysOpen;    //ʼ�տ�����ȡ

    int _openYear;        //�������
    int _openMonth;    //�����·�
    int _openDay;        //������
    int _openHour;        //����Сʱ
    int _openMinute;    //��������

    int _closeYear;    //�ر����
    int _closeMonth;    //�ر��·�
    int _closeDay;        //�ر���
    int _closeHour;    //�ر�Сʱ
    int _closeMinute;    //�رշ���

    time_t _start_time;
    time_t _end_time;

    std::string _name;    //����
    std::string _memo;    //˵��
    std::string _content;//����

    std::list<Item> _items;

};

struct _openRecords
{
    int _cid;
    int _packsId;
    int _seqNo;
    time_t _openTime;
    char _szCode[33];
};

struct CharOpenedPacksRecords
{
    int _cid;
    std::list<_openRecords> _openList;
};

//�������ģ��
class packsMgr
{    
public:
    
    int reload();
    //��ȡ���
    int getPacks(CharData * pc, const std::string& content, json_spirit::Object& robj);
    //��ȡ���
    int getPacks(CharData* pc, int packsId, json_spirit::Object& robj);
    //��ȡ���
    //int getPacks(CharData* pc, const std::string& code, json_spirit::Object& robj);
    //��ȡ���������
    int queryUnGetGifts(CharData * pc, json_spirit::Object &robj);

    //��ʾ����б�
    int showPacks(CharData* pc, json_spirit::Object& robj);

    //ɾ����ɫ
    int deleteChar(int cid);

    //��ȡ��¼����
    void export_opened(int cid, json_spirit::Array& a);

    static packsMgr* getInstance();
private:
    boost::shared_ptr<instancePacks> _findPacks(int packId);
    boost::shared_ptr<CharOpenedPacksRecords> _findOpenedPacks(int cid);
    boost::shared_ptr<instancePacks> loadPacks(int packId);
    boost::shared_ptr<CharOpenedPacksRecords> loadOpenedPacks(int cid);

    //��ɫ�Ƿ����ĳ�����
    bool haveOpenedPack(int cid, int packid);
    bool haveOpenedPack(int cid, int packid, int seqNo);
    
    //����һ����ȡ�����¼
    void addOpenRecord(int cid, int packsId);
    //����һ����ȡ�����¼
    void addOpenRecord(int cid, int packsId, const char* szCode, int seqNo);

    //ʹ�ü�����
    int useCode(int cid, const std::string& szCode, int& packId, int& seqNo);

    static packsMgr* m_handle;
    //����б�
    std::map<int, boost::shared_ptr<instancePacks> > m_packs_maps;
    //��ȡ��¼�б�
    std::map<int, boost::shared_ptr<CharOpenedPacksRecords> > m_packsOpen_maps;
    //�Ѿ�ʹ�õļ������¼
    std::map<std::string, int> m_used_code_maps;
};

