
#pragma once

#include <string>
#include <map>
#include <list>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"

#include "base_item.h"

using namespace json_spirit;

//礼包头文件

class CharData;

//具体的礼包
struct instancePacks
{
    int _id;            //具体礼包id
    int _bid;            //基础礼包id

    int _vip;            //需要的vip等级
    int _prestige;        //需要的声望
    int _level;            //需要的角色等级
    bool _needCode;    //是否需要激活码


    bool _bAlwaysOpen;    //始终可以领取

    int _openYear;        //开启年份
    int _openMonth;    //开启月份
    int _openDay;        //开启日
    int _openHour;        //开启小时
    int _openMinute;    //开启分钟

    int _closeYear;    //关闭年份
    int _closeMonth;    //关闭月份
    int _closeDay;        //关闭日
    int _closeHour;    //关闭小时
    int _closeMinute;    //关闭分钟

    time_t _start_time;
    time_t _end_time;

    std::string _name;    //名字
    std::string _memo;    //说明
    std::string _content;//内容

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

//礼包管理模块
class packsMgr
{    
public:
    
    int reload();
    //领取礼包
    int getPacks(CharData * pc, const std::string& content, json_spirit::Object& robj);
    //领取礼包
    int getPacks(CharData* pc, int packsId, json_spirit::Object& robj);
    //领取礼包
    //int getPacks(CharData* pc, const std::string& code, json_spirit::Object& robj);
    //获取可领礼包数
    int queryUnGetGifts(CharData * pc, json_spirit::Object &robj);

    //显示礼包列表
    int showPacks(CharData* pc, json_spirit::Object& robj);

    //删除角色
    int deleteChar(int cid);

    //领取记录导出
    void export_opened(int cid, json_spirit::Array& a);

    static packsMgr* getInstance();
private:
    boost::shared_ptr<instancePacks> _findPacks(int packId);
    boost::shared_ptr<CharOpenedPacksRecords> _findOpenedPacks(int cid);
    boost::shared_ptr<instancePacks> loadPacks(int packId);
    boost::shared_ptr<CharOpenedPacksRecords> loadOpenedPacks(int cid);

    //角色是否领过某个礼包
    bool haveOpenedPack(int cid, int packid);
    bool haveOpenedPack(int cid, int packid, int seqNo);
    
    //增加一条领取礼包记录
    void addOpenRecord(int cid, int packsId);
    //增加一条领取礼包记录
    void addOpenRecord(int cid, int packsId, const char* szCode, int seqNo);

    //使用激活码
    int useCode(int cid, const std::string& szCode, int& packId, int& seqNo);

    static packsMgr* m_handle;
    //礼包列表
    std::map<int, boost::shared_ptr<instancePacks> > m_packs_maps;
    //领取记录列表
    std::map<int, boost::shared_ptr<CharOpenedPacksRecords> > m_packsOpen_maps;
    //已经使用的激活码记录
    std::map<std::string, int> m_used_code_maps;
};

