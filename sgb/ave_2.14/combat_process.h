
#include "net.h"
#include "combat.h"
#include "jobqueue.hpp"
#include "utils_all.h"
#include "worker.hpp"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "singleton.h"
#include "combatRecord.h"

using namespace net;


extern int InsertInternalActionWork(json_spirit::mObject& obj);
extern Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

struct combatCmd
{
    Combat* _pCombat;
    combatCmd()
    {
        _pCombat = NULL;
    }
};

class CombatProces : public worker<combatCmd>
{
public:
    CombatProces(jobqueue<combatCmd>& _jobqueue, std::size_t _maxthreads = 4) :
      worker<combatCmd>("combat",_jobqueue, _maxthreads)
    {
        
    }

    virtual bool work(combatCmd &Cmd)       // 在些完成实际任务.
    {
        try
        {
            Combat* combat = Cmd._pCombat;
            if (combat == NULL)
            {
                std::cout<<"CombatProcess receive a empty combat...break."<<endl;
                return false;
            }
            combat->CalcResult();
            json_spirit::mObject obj;
            obj["cmd"] = "combatResult";
            obj["point"] = (boost::uint64_t)combat;
            obj["type"] = combat->type();
            obj["id"] = combat->combat_id();
            if (0 != InsertInternalActionWork(obj))
            {
                ERR();
            }
            return true;
        }
        catch (std::exception& e)
        {
            syslog(LOG_ERR, "combat work , Exception: %s", e.what());
            void * array[25];
            int nSize = backtrace(array, 25);
            char ** symbols = backtrace_symbols(array, nSize);
            for (int i = 0; i < nSize; i++)
            {
                syslog(LOG_ERR, symbols[i]);
            }
            free(symbols);
        }
        return true;
    }
};

class SaveCombatWorker : public worker<combatCmd>
{
public:
    SaveCombatWorker(jobqueue<combatCmd>& _jobqueue, std::size_t _maxthreads = 4) :
      worker<combatCmd>("combat record",_jobqueue, _maxthreads)
    {
        
    }

    virtual bool work(combatCmd &cmd)       // 在些完成实际任务.
    {
        try
        {
            Combat* pCombat = cmd._pCombat;
            if (pCombat == NULL)
            {
                std::cout<<"Combat record Process receive a empty combat...break."<<endl;
                return false;
            }
#ifdef debug_combat_result    
            uint64_t stamp1 = splsTimeStamp();
#endif
            if (pCombat->m_result_text == "")
            {
                pCombat->m_result_text = json_spirit::write(pCombat->m_result_array);
            }
            std::string final_result = "{\"cmd\":\"battle\",\"s\":200,\"cmdlist\":" + pCombat->m_result_text + "}";
#ifdef debug_combat_result    
            uint64_t stamp2 = splsTimeStamp();
            cout<<"Combat record Process 1 cost:"<<(stamp2-stamp1)<<endl;
#endif
            if (combat_servant == pCombat->m_type && 11 == pCombat->m_extra_data[0])//家丁求救战斗信息发给求救者
            {
                //这个线程里面不能调用 GetCharData !!!
                //boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_extra_data[1]);
                //if (pcd.get())
                {
                    //boost::shared_ptr<OnlineCharactor> pchar = GeneralDataMgr::getInstance()->GetOnlineCharactor(pcd->m_name);
                    boost::shared_ptr<OnlineCharactor> pchar = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_extra_viewer);
                    if (pchar.get())
                    {
                        pchar->Send(final_result);
                        //cout<<"send combat result to soser"<<endl;
                    }
                }
            }
            //军团战不用发战报给双方
            else if (combat_corps_fighting != pCombat->m_type)
            {
                boost::shared_ptr<OnlineCharactor> pchar = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_attacker->Name());
                if (pchar.get())
                {
                    pchar->Send(final_result);
                    //cout<<"send combat result to attacker"<<endl;
                }
            }
            if (combat_camp_race == pCombat->m_type)
            {
                boost::shared_ptr<OnlineCharactor> pchar = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_defender->Name());
                if (pchar.get())
                {
                    pchar->Send(final_result);
                    //cout<<"send combat result to defender"<<endl;
                }
            }
#ifdef debug_combat_result    
            uint64_t stamp3 = splsTimeStamp();
            cout<<"Combat record Process 2 cost:"<<(stamp3-stamp2)<<endl;
#endif
            //if (combat_stronghold == pCombat->m_type || combat_race == pCombat->m_type)
            {
                if (pCombat->m_combat_info_text == "")
                {
                    pCombat->setCombatInfo();
                }
                //战斗结果存入数据库
                std::string strRecord = "{\"cmd\":\"getBattleList\",\"s\":200,\"getBattleList\":" + pCombat->m_combat_info_text
                        + ",\"cmdlist\":" + pCombat->m_result_text + "}";

                Singleton<combatRecordMgr>::Instance().addCombatRecord(pCombat->m_combat_id, strRecord);

                InsertSaveDb("insert into battle_records (id,attacker,type,defender,attackerName,defenderName,input,record,_date,result,aLevel,dLevel,archive,aAttack,extra1,extra2,hurt) values ("
                                + LEX_CAST_STR(pCombat->m_combat_id)
                                + "," + LEX_CAST_STR(pCombat->m_attacker->getCharId())
                                + "," + LEX_CAST_STR(pCombat->m_type)
                                + "," + LEX_CAST_STR(pCombat->m_defender->getCharId())
                                + ",'" + GetDb().safestr(pCombat->m_attacker->Name())
                                + "','" + GetDb().safestr(pCombat->m_defender->Name())
                                + "',unix_timestamp(),'" + GetDb().safestr(strRecord)
                                + "',curdate()," + LEX_CAST_STR(pCombat->m_state)
                                + "," + LEX_CAST_STR(pCombat->m_attacker->level())
                                + "," + LEX_CAST_STR(pCombat->m_defender->level())
                                + "," + LEX_CAST_STR(pCombat->m_archive_report)
                                + "," + LEX_CAST_STR(pCombat->m_attacker->attack_value())
                                + "," + LEX_CAST_STR(pCombat->m_extra_data[0])
                                + "," + LEX_CAST_STR(pCombat->m_extra_data[1])
                                + "," + LEX_CAST_STR(pCombat->m_attacker->DieHp())
                                +")");
            }
#ifdef debug_combat_result    
            uint64_t stamp4 = splsTimeStamp();
            cout<<"Combat record Process 3 cost:"<<(stamp4-stamp3)<<endl;
#endif

            if (pCombat->m_mail_to > 0)
            {
                //发邮件                
                sendSystemMail(pCombat->m_mail_to_name, pCombat->m_mail_to, pCombat->m_mail_title, pCombat->m_mail_content,"", pCombat->m_combat_id);
            }

            //释放内存
            if (pCombat->m_attacker->_army_data)
            {
                delete (pCombat->m_attacker->_army_data);
            }
            if (pCombat->m_defender->_army_data)
            {
                delete (pCombat->m_defender->_army_data);
            }
            delete pCombat->m_attacker;
            delete pCombat->m_defender;
            delete pCombat;

            return true;
        }
        catch (std::exception& e)
        {
            syslog(LOG_ERR, "combat record work , Exception: %s", e.what());
            void * array[25];
            int nSize = backtrace(array, 25);
            char ** symbols = backtrace_symbols(array, nSize);
            for (int i = 0; i < nSize; i++)
            {
                syslog(LOG_ERR, symbols[i]);
            }
            free(symbols);
        }
        return true;
    }
};

