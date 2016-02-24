#pragma once

#include <string>
#include <map>
#include <list>
#include <boost/cstdint.hpp>

#include "boost/smart_ptr/shared_ptr.hpp"

#include <vector>
#include "json_spirit.h"
#include "spls_const.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>


const int iMaxHorse = iHorseStarsMax * (1 + iHorseTurnsMax) + 1;
const int iMaxAction = 2;//ս����ʾ����ҵ�ս������
struct CharData;

struct baseHorse
{
    int id;            //ս��Ψһid
    int level;        //ս��ȼ�
    int quality;    //ս��Ʒ��
    int star;        //ս���Ǽ�
    int turn;        //ս��ת������
    int spic;        //ս��ͼƬ
    int pugong;        //ս��ӳ�
    int pufang;        //ս��ӳ�
    int cegong;        //ս��ӳ�
    int cefang;        //ս��ӳ�
    int bingli;        //ս��ӳ�
    int need_exp;    //�������辭��
    int add_exp_silver;    //�����������ӵľ���
    int add_exp_gold;        //����������ӵľ���
    int need_prestige;//ת����������
    int fruit[3];    //ս���Ӧ����������
    std::string name;    //����
};

struct baseHorseFruit
{
    int id;            //����Ψһid
    int type;        //��������
    int pugong;        //���Ӽӳ�
    int pufang;        //���Ӽӳ�
    int cegong;        //���Ӽӳ�
    int cefang;        //���Ӽӳ�
    int bingli;        //���Ӽӳ�
    int get_horseid;//�ɻ�ȡ
    int eat_horseid;//��ʳ��
    std::string name;    //����
};

struct CharHorseFruit
{
    CharHorseFruit(int c_id)
    {
        cid = c_id;
        state = 0;
        start_time = 0;
        end_time = 0;
    }
    int cid;
    int state;//0δ��ȡ��1�Ѿ���ȡ�����ܷ��ã�2�Ѿ���ȡ���ҿɷ��ã�3�Ѿ����ã�4����
    time_t start_time;
    time_t end_time;
    boost::shared_ptr<baseHorseFruit> fruit;
    int start();
    int stop();

    boost::uuids::uuid _uuid;    //��ʱ��Ψһid
};

struct CharHorseFruitAction
{
    CharHorseFruitAction(int c_id, int h_id)
    {
        cid = c_id;
        horse_id = h_id;
        horse_star = 0;
        horse_name = "";
    }
    int cid;
    int horse_id;
    int horse_star;
    std::string horse_name;
    std::vector<CharHorseFruit> fruits_list;
};

struct CharHorse
{
    CharHorse(int c_id)
    {
        cid = c_id;
        horseid = 0;
        exp = 0;
        horse = NULL;
        start_time = 0;
        end_time = 0;
        pugong = 0;
        pufang = 0;
        cegong = 0;
        cefang = 0;
        bingli = 0;
        _score = 0;
        _power = 0;
    }
    int cid;    //��ɫid
    int horseid;    //ս��id
    int exp;    //��ǰ����
    time_t start_time;//���ӻ����ʱ��
    time_t end_time;//���ӻ����ʱ��
    int pugong;        //���Ӽӳ�
    int pufang;        //���Ӽӳ�
    int cegong;        //���Ӽӳ�
    int cefang;        //���Ӽӳ�
    int bingli;        //���Ӽӳ�
    baseHorse* horse;
    std::vector<CharHorseFruitAction> action_list;//���ӻ��Ϣ
    int _score;
    int _power;
    void updateNewAttack();
    int getNewScore(){return _score;}
    int getNewPower(){return _power;}
    int checkFruitState();
    int updateActionFruit();
    int save();
    int save_action();
};


class horseMgr
{
public:
    static horseMgr* getInstance();
    baseHorse* getHorse(int id);
    int getHorseInfo(CharData& cData, json_spirit::Object& robj);
    std::string NameToLink(int cid, std::string name, int quality);
    int trainHorse(CharData& cData, int type, int& cri_type, int& get_exp, int& dj_use);
    int turnHorse(CharData& cData, json_spirit::Object& robj);
    //����ս��ȼ�
    int setHorse(CharData& cData, int horseId, int exp);
    //���ӹ���
    int fruitDone(int cid, int id);
    //ս����Ӳ���
    int dealHorseFruit(CharData& cData, int type, int id);
    //��ȡ��һ���й��ӵ�ս��
    int getNextActionHorse(int horseid);
    int getHorseFruitsList(CharData& cData, json_spirit::Object& robj);
    boost::shared_ptr<baseHorseFruit> getBaseHorseFruit(int fruit_id);

    void setHorseTrainTimes(int total, int gold_times);
private:
    static horseMgr* m_handle;
    int reload();
    baseHorse base_horses[iMaxHorse];//ת�����*�Ǽ�
    std::list<int> HorseFruitAction;//ս����ص�ս��id��
    std::map<int,boost::shared_ptr<baseHorseFruit> > base_fruits_list;
};


