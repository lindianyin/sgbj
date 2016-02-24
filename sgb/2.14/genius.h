#ifndef _GENIUS_H_
#define _GENIUS_H_

#include <vector>
#include <list>
#include <map>
#include <boost/cstdint.hpp>

#include <string>
#include "json_spirit.h"

//�츳���ŵȼ�
const int iGeniusOpenLevel = 20;
//���ŵ��츳��������
const int iGeniusMaxNum = 5;
//�����츳����VIP
const int iGeniusOpenVIP[iGeniusMaxNum] = {0, 0, 0, 5, 7};
//�����츳������
const int iGeniusOpenGold[iGeniusMaxNum] = {20, 20, 20, 20, 20};
//ϴ�츳������
const int iGeniusWashGold = 20;
//�����츳������
const int iGeniusLockGold[6] = {1,2,5,10,20,30};
//��ֲ�츳������
const int iGeniusGraftGold[6] = {5,20,50,100,200,300};

//�佫�츳�б�
struct base_genius
{
    int id;                // id
    std::string    cname;    // ����
    std::string colorName;//����ɫ����
    std::string memo;    // ˵��
    int color;            // Ʒ��
    std::vector<int>m_preupidList;        // ����ǰ���츳id�����ܻ���ֶ�������
    int nextupid;        // ��һ�����س�id
    double    upgrade_per;    // �����츳����
    double    init_per;        // �����츳����
    double    wash_per;        // ϴ�츳����
    int genius_type;    //�츳����
    int genius_per;        //�츳��������
    int genius_val;        //�츳Ч����ֵ
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

// �����츳����ϴ��Ϣ
struct general_genius_records
{
    int genius_id;        // id
    int    nums;            // ����ϴ����
    int    updateTag;        // �������ݿ��ʶ    1���������    0�����룻2�����£�3��ɾ��
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
    std::map<int, base_genius> m_base_genius;// �츳�б�
    std::map<int, std::vector<general_genius_records*>*>m_general_geniusrecord_list;// ϴ�츳����
};

#endif

