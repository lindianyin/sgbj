#ifndef _GENIUS_H_
#define _GENIUS_H_

#include <vector>
#include <list>
#include <map>
#include <boost/cstdint.hpp>

#include <string>
#include "json_spirit.h"

//天赋开放等级
const int iGeniusOpenLevel = 20;
//开放的天赋数量上限
const int iGeniusMaxNum = 5;
//开启天赋所需VIP
const int iGeniusOpenVIP[iGeniusMaxNum] = {0, 0, 0, 5, 7};
//开启天赋所需金币
const int iGeniusOpenGold[iGeniusMaxNum] = {20, 20, 20, 20, 20};
//洗天赋所需金币
const int iGeniusWashGold = 20;
//锁定天赋所需金币
const int iGeniusLockGold[6] = {1,2,5,10,20,30};
//移植天赋所需金币
const int iGeniusGraftGold[6] = {5,20,50,100,200,300};

//武将天赋列表
struct base_genius
{
    int id;                // id
    std::string    cname;    // 名称
    std::string colorName;//带颜色名字
    std::string memo;    // 说明
    int color;            // 品质
    std::vector<int>m_preupidList;        // 进阶前的天赋id，可能会出现多个的情况
    int nextupid;        // 下一进阶特长id
    double    upgrade_per;    // 进阶天赋概率
    double    init_per;        // 开启天赋概率
    double    wash_per;        // 洗天赋概率
    int genius_type;    //天赋类型
    int genius_per;        //天赋触发概率
    int genius_val;        //天赋效果数值
    base_genius()
    {
        id = 0;
        cname = "";
        memo = "";
        color = 0;
        m_preupidList.clear();
        nextupid = 0;
        upgrade_per = 0.0;
        init_per = 0.0;
        wash_per = 0.0;
        genius_type = 0;
        genius_per = 0;
        genius_val = 0;
    }
};

// 保存天赋锁定洗信息
struct general_genius_records
{
    int genius_id;        // id
    int    nums;            // 锁定洗次数
    int    updateTag;        // 更新数据库标识    1：不需更新    0：插入；2：更新；3：删除
    general_genius_records()
    {
        genius_id = 0;
        nums = 0;
        updateTag = 0;
    }
    void SetUpdateFlag()
    {
        if (updateTag == 1)
            updateTag = 2;
    }
    void SetDeleteFlag()
    {
        updateTag = 3;
    }
};

class geniusMgr
{
public:
    int reload();
    void LinkData();
    static geniusMgr* getInstance();
    const base_genius& GetGenius(int nGenius_id);
    void UpdateDB(uint64_t gid);
    int GetGeniusUpgradeList(uint64_t cid, uint64_t gid, json_spirit::Object& retObj);
    int OpenGenius(uint64_t cid, uint64_t gid, json_spirit::Object& robj);
    bool CheckGeniusInList(int genius_id, const std::vector<int>& list);
    bool CheckGeniusCanUp(uint64_t cid, uint64_t gid, int genius_id);
    int CleanGenius(uint64_t cid, uint64_t gid, const std::vector<int>& lock_list, json_spirit::Object& robj);
    bool WashGeniusUnlock(const std::vector<int>& already_list, int& genius_id, std::string& broad_msg);
    bool WashGeniusLock(uint64_t gid, std::vector<int>& try_list, const std::vector<int>& lock_list, int& genius_id);
    int GraftGenius(uint64_t cid, uint64_t gid, uint64_t gid2, json_spirit::Object& robj);
    int setGenius(int cid, int gid, int genius_id, int pos);
    
private:
    static geniusMgr* m_handle;
    std::map<int, base_genius> m_base_genius;// 天赋列表
    std::map<int, std::vector<general_genius_records*>*>m_general_geniusrecord_list;// 洗天赋次数
};

#endif

