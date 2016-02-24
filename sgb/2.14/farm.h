
#ifndef _FARM_H_
#define _FARM_H_

#include <vector>
#include <list>
#include <map>
#include <boost/cstdint.hpp>

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "spls_const.h"

//���״̬
enum field_state
{
    field_farm = 1,
    field_finish,
    field_free,
    field_tired,
    field_lock
};

//���
struct field
{
    field()
    {
        m_nourish_time = 0;
        m_nourish_num = 0;
    };
    int m_cid;
    int m_pos;
    int m_state;//���״̬1������2���3����4ƣ��5δ����
    int m_need_level;//������Ҫ�ȼ�
    int m_type;//0δ����1������
    int m_supply;//����
    int m_reward_num;//��ǰ����ȡ����
    int m_left_num;//ʣ����Ҫ��ɵĴ���

    time_t m_start_time;//����ʼʱ��
    time_t m_end_time;//�������ʱ��

    time_t m_nourish_time;//������ʼʱ��
    int m_nourish_num;//��������

    int start();
    int done();
    int save();
    int get_nourish();
    void start_nourish();
    void end_nourish();
    void clear_nourish();

    boost::uuids::uuid _uuid;    //��ʱ��Ψһid
};

typedef std::vector<boost::shared_ptr<field> > fieldlist;
typedef std::map<int,int> waterlist;

class farmMgr
{
public:
    int reload();
    int resetAll();
    static farmMgr* getInstance();
    int FieldNum(int cid);
    int StartFarmed(int cid, int tid, json_spirit::Object& robj);
    int FarmDone(int cid, int pos);
    int SetFarmed(int cid, int pos, json_spirit::Object& robj);
    int speedFarm(int cid, int pos);
    int updateAll(int cid);
    int UpFarmField(int cid, json_spirit::Object& robj);
    int WaterFarmField(int cid, json_spirit::Object& robj);
    bool getWaterState(int cid, int friend_id);
    void setWaterState(int cid, int friend_id);
    int WaterFarmFriendField(int cid, int friend_id, json_spirit::Object& robj);
    int WaterFarmFriendFieldAll(int cid, json_spirit::Object& robj);
    boost::shared_ptr<fieldlist> GetCharFieldList(int cid);
    boost::shared_ptr<waterlist> GetCharWaterList(int cid);
    boost::shared_ptr<fieldlist> open(int cid);
    int getNourishReward(int cid);
    void clearNourishReward(int cid);
    int rewardNourish(int cid, json_spirit::Object& robj);

    //ɾ����ɫ
    int deleteChar(int cid);
    int getCoolTime(int cid, int& state);
    int getRewardTimes(int cid);
    
private:
    static farmMgr* m_handle;
    std::map<int, boost::shared_ptr<fieldlist> > m_char_field_list;//�������б�
    std::map<int, boost::shared_ptr<waterlist> > m_char_water_list;//��ҹ���б�
};

#endif

