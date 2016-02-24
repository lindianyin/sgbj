#pragma once

#include <string>
#include <map>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "combat_def.h"
#include "json_spirit.h"
#include "spls_const.h"

using namespace json_spirit;

class CharData;

struct baseState
{
    int id;                //״̬Ψһid
    int type;            //״̬���: 1����ר�� 2/����ר�� 3ͨ��
    int effect_type;    //״̬Ч�� �μ�  special_state
    int effect_gailv;    //״̬����
    int effect_value;    //״̬��ֵ
    std::string name;    //״̬��
    std::string memo;    //״̬����
};

//��״̬
struct newCharStates
{
    CharData& cdata;
    bool _enable;        //�Ƿ�����
    int _star_level;    //��ͼ�ȼ�
    int _stars[8];        //�ƾ������������꣬������»�棬���ţ�̰�ǣ���ޱ
    int _effects[8];    //״̬Ч��

    int _cur_state[3];    //��ǰ״̬ 1-8

    int load();            //�����ݿ����
    int init();            //�״ο�����ʼ��
    void save();        //���浽���ݿ�
    int levelup();        //�������߶�ת����
    
    int refresh(int type, int pos = 1);        //���ˢ��״̬
    int refresh();        //ϵͳ�Զ�ˢ��״̬    

    //����7����Ϣ
    int getStarInfo(json_spirit::Object& robj);

    //״̬ˢ����Ϣ
    int getStateInfo(json_spirit::Array&);

    //״̬ˢ����Ϣ
    int getCostInfo(json_spirit::Object&);

    newCharStates(CharData& cd);
};

struct npcStrongholdStates
{
    int _cid;
    int _stronghold;
    int _level;            //״̬�ȼ�
    int _state_num;    //״̬����
    int _cur_state[3];    //��ǰ״̬ 1-8
    int _effects[3];
    bool m_need_refresh;
    int load();

    int refresh();
    //״̬��Ϣ
    int getStateInfo(json_spirit::Array&);

    npcStrongholdStates(int cid, int stronghold, int level, int num);
};

class baseStateMgr
{
public:
    baseStateMgr();
    ~baseStateMgr();
    int load();
    //��û���״̬
    boost::shared_ptr<baseState> GetBaseState(int id);
    static baseStateMgr* getInstance();
    //�������ˢ��״̬��ָ��ˢ�Ǹ�
    int refresh(int vip_level, int hero_level, boost::shared_ptr<baseState>* states, int idx /* 1,2,3 */, int type = 1);
    //ս���������Զ�ˢ�����״̬
    int refresh(int vip_level, int hero_lv, boost::shared_ptr<baseState>*, int type = 1);
    //ˢ�¹ؿ�״̬
    int refreshNpcState(int level, int state_nums, boost::shared_ptr<baseState>* states);

    json_spirit::Object getStateObj(int idx);
    int getAllStates(int hero_level, boost::shared_ptr<baseState>* curStates, json_spirit::Array& allStates);

private:
    static baseStateMgr* m_handle;
    std::map<int, boost::shared_ptr<baseState> > m_base_states;        //����״̬����

    std::vector<int> m_state_sets[3];        //����״̬���ϣ��������״̬

    json_spirit::Object m_state_obj[max_refresh_state_types];        //���Ÿ��û���״̬obj����ѯ����״̬ʱʹ��
};

