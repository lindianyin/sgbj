#pragma once

#include <string>
#include <time.h>
#include <map>
#include <list>
#include <vector>

#include "json_spirit.h"

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

#include "const_def.h"
#include "net.h"
#include "item.h"
#include "multi_combat.hpp"
#include "data.h"

using namespace net;

const int iPKMaxRoom = 200;
const int iPKMaxSet = 8;

struct PkRankRewards
{
    int rank;
    std::list<Item> reward;//����
};

struct CharData;

enum enum_pk_state
{
    PK_STATE_INIT = 0,
    PK_STATE_ROOM = 1,
    PK_STATE_END,
};

struct CharPkData
{
private:
    boost::shared_ptr<CharData> m_charactor;    //��Ӧ�Ľ�ɫ
public:
    CharPkData(int cid);
    boost::shared_ptr<CharData> getCharData();
    void toObj(json_spirit::Object& robj);
    void toSimpleObj(json_spirit::Object& robj);
    int sendInvite(int toid);
    int acceptInvite(int roomid);
    void load();
    void save();
    bool canChallenge();

    int m_cid;
    int m_total_get;//��Ӯȡ����
    int m_bet;//������趨
    int m_state;//pk״̬
    int m_roomid;
    int m_pos;
};

struct CharPkGet
{
    int m_cid;
    int m_rank;
    int m_total_get;
};

//������λ
struct PkSet
{
    int m_pos;//���
    int m_state;//״̬(-1�ر�0����1�����)
    bool m_ready;//׼��״̬
    boost::shared_ptr<CharPkData> m_playes;//���pk��Ϣ

    void reset()
    {
        m_state = 0;
        m_ready = false;
        m_playes.reset();
    }
};

//���볡����
struct PkRoom
{
    int m_id;//������
    std::string m_name;//��������
    std::string m_password;//����
    int m_cur;//��ǰ�������
    int m_max;//���˷�
    int m_own_cid;//����id
    int m_state;//״̬(0���з���1�Ⱥ����2������Ϸ)
    int m_combat_id;//ս��id
    int m_bet;
    PkSet m_sets[iPKMaxSet];//������λ����Ϣ
    boost::shared_ptr<ChatChannel> _channel;//����Ƶ��
    void toObj(json_spirit::Object& obj);
    void broadInfo();
};

class PkMgr
{
public:
    PkMgr();
    boost::shared_ptr<CharPkData> getPkData(int cid);
    boost::shared_ptr<CharPkData> getPkDataIndex(int index);
    boost::shared_ptr<CharPkData> addCharactor(int cid);
    boost::shared_ptr<CharPkGet> getPkGet(int cid);
    void updateRank();
    void removeCharactor(int cid);
    int querySelfInfo(int cid, json_spirit::Object& robj);
    int querySysList(int cid, json_spirit::mObject& o, json_spirit::Object& robj);
    int queryRooms(json_spirit::mObject& o, json_spirit::Object& robj);
    int queryRoomInfo(int cid, json_spirit::Object& robj);
    //�������
    PkRoom* getRoom(int roomid)
    {
        if (roomid > 0 && roomid <= iPKMaxRoom)
        {
            return &m_rooms[roomid-1];
        }
        return NULL;
    }
    int createRoom(int cid, json_spirit::mObject& o, json_spirit::Object& robj);
    int joinRoom(int cid, int roomid, std::string password, bool invite = false);
    int leaveRoom(int cid);
    int kickPlayer(int cid, json_spirit::mObject& o);
    int changeSet(int cid, json_spirit::mObject& o);
    int openSet(int cid, json_spirit::mObject& o);
    int closeSet(int cid, json_spirit::mObject& o);
    int setPassword(int cid, json_spirit::mObject& o);
    int setBet(int cid, json_spirit::mObject& o);
    int setReady(int cid, json_spirit::mObject& o);
    int startCombat(int cid);
    int invite(int cid);
    void broadInfo(int roomid);
    //��������
    int queryTopList(int cid, json_spirit::Object& robj);
    int queryTopCid();
    //��������
    int queryRankRewards(int rank, json_spirit::Array& reward_list);
    int seasonAwards();
    int combatAllResult(multi_combat* pCombat);
    int combatResult(multi_combat* pCombat);
    boost::shared_ptr<ChatChannel> GetRoomChannel(int roomid);

private:
    boost::unordered_map<int, boost::shared_ptr<CharPkData> > m_pk_maps;//���߽�ɫpk��Ϣ
    std::list<boost::shared_ptr<CharPkGet> > m_pk_get;//���pk���
    std::vector<PkRankRewards> m_rewards;//��������
    int m_last_rank;//����н�����
    std::vector<int> m_last_top;//�ϴ������гƺŵ����
    PkRoom m_rooms[iPKMaxRoom];//����
};

//�����ս�б�
int ProcessQueryPKList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ�Լ�����ս��Ϣ
int ProcessQueryPKInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ����ǰ3
int ProcessQueryPKTop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//pk����
int ProcessDealPK(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��÷����б�
int ProcessQueryPKRooms(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ������Ϣ
int ProcessQueryRoomInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��������
int ProcessPKCreateRoom(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//���뷿��
int ProcessPKJoinRoom(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�뿪����
int ProcessPKLeaveRoom(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�߳����
int ProcessPKKickPlayer(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��λ��
int ProcessPKChangeSet(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��λ��
int ProcessPKOpenSet(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��λ��
int ProcessPKCloseSet(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�趨����
int ProcessPKSetPassword(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�趨����
int ProcessPKSetBet(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����׼��
int ProcessPKSetReady(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʼս��
int ProcessPKStart(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����
int ProcessPKInvite(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

