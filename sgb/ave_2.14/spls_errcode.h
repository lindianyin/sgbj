#pragma once

#define HC_SUCCESS              200            //�ɹ�

#define HC_SUCCESS_NO_RET        201                //�ɹ��������ظ��ͻ���
#define HC_ERROR_NO_RET        202                //�Ѿ���Ӵ��������Ϣ

#define HC_ERROR                401            //����
#define HC_ERROR_WRONG_ACCOUNT  402          //�˺Ų�����
#define HC_ERROR_WRONG_PASSWORD 403          //�������
#define HC_ERROR_LOGIN_FIRST    404             //δ��¼
#define HC_ERROR_LOGOUT_FIRST   405              //�Ѿ���¼
#define HC_ERROR_CHAR_EXIST     406           //��ɫ���Ѿ�����
#define HC_ERROR_CHAR_NOT_ONLINE    407      //��ɫ������
#define HC_ERROR_NAME_TOO_LONG      408          //��ɫ��̫��
#define HC_ERROR_FORBIDEN_CHAT      409          //������
#define HC_ERROR_NOT_JOIN_JT        410          //��û�м������

#define HC_ERROR_NOT_ENOUGH_SILVER     411     //������Ӳ�����
#define HC_ERROR_NOT_ENOUGH_GOLD     412     //��Ľ�Ҳ�����

#define HC_ERROR_MORE_VIP_LEVEL 413            //vip�ȼ�����
#define HC_ERROR_SKILL_QUEUE_MAX    414            //�����о����в�����������

#define HC_ERROR_NO_ATTACK_TIMES    415            //û��ʣ��Ĺ�������

#define HC_ERROR_NOT_ENOUGH_LING    416            //��ľ������

#define HC_ERROR_SKILL_LEVEL_MAX    417            //��ļ��ܴﵽ��ǰ���

#define HC_ERROR_HAS_GENERAL_ALREADY 418        //�Ѿ��и��佫
#define HC_ERROR_GENERALTREASURE_MAX 419        //�佫��������

#define HC_ERROR_IN_COOLTIME 420        //��ȴʱ��δ����
#define HC_ERROR_TARGET_IS_BUSY    421    //Ŀ����æ

#define HC_ERROR_WAIT_FOR_FARM_FINISH    422    //�Ⱥ�ȫ���������
#define HC_ERROR_NEED_MORE_RIPE    423    //����Ȳ���
#define HC_ERROR_WRONG_SEASON 424    //���ڲ���

#define HC_ERROR_BACKPACK_FULL_NO_UNEQIPT 425    //�ֿ�����������ж��װ��
#define HC_ERROR_BACKPACK_FULL_GET_EQUIPT 426    //�ֿ��������뵽�ع��л�ȡ��Ʒ
#define HC_ERROR_BACKPACK_FULL_BUYBACK 427        //�ֿ����������ܻع�
#define HC_ERROR_NEED_MORE_MATURITY 428            //�������Ȳ���10%��������ȡ
#define HC_ERROR_GENERAL_LEVEL_MAX    429            //Ӣ�۵ȼ��ﵽ��ǰ���

#define HC_ERROR_ALREADY_IN_A_CORPS      430      //���Ѿ��������
#define HC_ERROR_CORPS_NAME_EXIST          431        //���������Ѿ�����
#define HC_ERROR_CORPS_ALREADY_APPLY     432        //���Ѿ��ύ������
#define HC_ERROR_CORPS_MAX_APPLY         433        //�����ֻ���ύ��������
#define HC_ERROR_CORPS_NEED_MORE_LEV        434        //���ĵȼ�����19�������ܴ�����������
#define HC_ERROR_QUEUE_FULL    435        //��������
#define HC_ERROR_CORPS_OFFICAL_LIMIT        436        //��û��Ȩ��

#define HC_ERROR_NOT_ENOUGH_STONE    437    //ǿ��ʯ��������
#define HC_ERROR_UPGRADE_MAX_LEVEL    438    //װ���Ѿ�ǿ������߼�
#define HC_ERROR_MAP_NOT_OPEN            439    //��ͼ��δ����
#define HC_ERROR_SKILL_RESEARCHING    440    //�ü����Ѿ����о�������

#define HC_ERROR_CORPS_MAX_ASSISTANT     441//���ŵĸ����ų�ְλ�����Ѵ�����
#define HC_ERROR_CORPS_MAX_MEMBERS     442//���ų�Ա�����Ѵ�����
#define HC_ERROR_CORPS_NOT_SAME_CAMP     443//��ֻ�ܼ���ͬһ��Ӫ�ľ���

#define HC_ERROR_WEAPON_NOT_ENOUGH_LEVEL 444//���ĵȼ���������ʱ���ܹ����������

#define HC_ERROR_SEND_MAIL_INVALID_DEST 445    //����ʧ�ܣ������˲�����
#define HC_ERROR_BOSS_EVENT_END 446        //bossս�Ѿ�����
#define HC_ERROR_BOSS_WAITING 447            //BOSSս�������������Ժ�
#define HC_ERROR_BOSS_NOT_OPEN 448        //BOSSս��δ����
#define HC_ERROR_BOSS_INSPIRE_FAIL 449    //�ܿ�ϧ������ʧ����
#define HC_ERROR_BOSS_INSPIRE_MAX 450    //�Ѿ����赽���
#define HC_ERROR_BOSS_NOT_ENTER 451        //���Ƚ��븱��

#define HC_ERROR_ALREADY_GET_PACKS 452    //���Ѿ���ȡ�������

#define HC_ERROR_AUTH_EXPIRED    453            //��֤��Ϣ�Ѿ�����

#define HC_ERROR_NOT_ENOUGH_ORE    454        //��ʯ��������
#define HC_ERROR_BOSS_NOT_ENOUGH_LEVEL    455    //���ĵȼ�����31�����޷�����bossս

#define HC_ERROR_CAMP_RACE_NO_CAMP    456    //û����Ӫ�޷�������Ӫս
#define HC_ERROR_CAMP_RACE_NOT_IN    457    //��û������Ӫս��
#define HC_ERROR_CAMP_RACE_NOT_OPEN    458    //��Ӫս��δ����

#define HC_ERROR_CORPS_NOT_IN_PARTY    459    //ֻ�����Ĳμ��߲��ܷ����ټ�
#define HC_ERROR_CORPS_PARYTY_LEVEL    460    //���ŵȼ�����3�����ٰܾ����
#define HC_ERROR_CORPS_ALREADY_JISI    461    //�������Ѿ��������
#define HC_ERROR_CORPS_ALREADY_IN_PARTY    462    //���Ѿ����������

#define HC_ERROR_GUARD_NOT_ENOUGH_LEVEL 463 //�����ȼ����㣬�޷��μӡ���Ҫ�����ȼ�52
#define HC_ERROR_GUARD_NO_ROB_TIMES 464 //������ѱ��ع��˻������Ľظٴ���Ϊ0���޷���ȡ
#define HC_ERROR_GUARD_ROB_IS_COLD 465    //�ٸ���ȴ��
#define HC_ERROR_GUARD_ROB_ALREADY 466    //���Ѿ��ٹ��˸�

#define HC_ERROR_NO_GROUP_COPY_TEAM    467        //���ȼ���һ������
#define HC_ERROR_INVALID_GROUP_COPY_ID 468    //������������˸���
#define HC_ERROR_NO_GROUP_COPY_ATTACK_TIME 469    //�����첻���ٹ������������
#define HC_ERROR_INVALID_GROUP_COPY_TEAM 470    //���鲻���ڣ���ˢ�½���
#define HC_ERROR_GROUP_COPY_NEED_MORE_MEMBER 471    //�������㣬�޷���������
#define HC_ERROR_GROUP_COPY_NOT_LEADER 472        //�����Ƕӳ�
#define HC_ERROR_GROUP_COPY_CAN_NOT_ATTACK 473    //��ͼδͨ�أ��޷������ø���
#define HC_ERROR_GROUP_COPY_ENTER_FIRST 474    //���Ƚ�����˸���

#define HC_ERROR_TRADE_ALREADY 475    //���Ѿ���ͨ����
#define HC_ERROR_TRADE_POS_ERROR 476    //��λ��״̬����
#define HC_ERROR_TRADE_NO_TIMES 477    //��������ľ�
#define HC_ERROR_TRADE_NOT 478    //�㲻��ͨ����
#define HC_ERROR_TRADE_BE_PROTECT 479    //ͨ��λ�ñ�����

#define HC_ERROR_RESEARCH_TOO_TIRED    480        //̫ƣ���ˣ�������������ѵ����
#define HC_ERROR_CODE_IS_USED    481                //�ü������Ѿ���ʹ��
#define HC_ERROR_CODE_IS_INVALID    482            //��Ч�ļ�����
#define HC_ERROR_SMELT_FULL 483        //ұ����������

#define HC_ERROR_NAME_ILLEGAL    484    //���ֲ��Ϸ�

#define HC_ERROR_GROUP_COPY_NOT_OPEN    485    //���˸�����û�п���
#define HC_ERROR_ACCOUNT_BE_FREEZED    486    //�ʺű�����

#define HC_ERROR_NEED_MORE_PRESTIGE    487    //��Ҫ��������
#define HC_ERROR_CORPS_NEED_12H    488    //������Ų���12Сʱ����ʹ�øù���

#define HC_ERROR_NOT_ENOUGH_TIME 490    //û���㹻����
#define HC_ERROR_SERVANT_CD 491    //�Ҷ���ȴ��
#define HC_ERROR_SERVANT_TOO_MORE 492    //�Ҷ���������
#define HC_ERROR_SERVANT_LEVEL 493    //�ȼ�������

#define HC_ERROR_NOT_ENOUGH_YUSHI    489    //��û���㹻����ʯ
#define HC_ERROR_BAOSHI_MAX_LEVEL    494    //��ʯ�Ѿ������Ʒ��
#define HC_ERROR_BAOSHI_NOS_POS        495    //û�ж���ı�ʯ��

#define HC_ERROR_NEED_MORE_LEVEL    496        //�����ȼ�����
#define HC_ERROR_REVOLT_CD            497        //������ȴ��
#define HC_ERROR_REVOLT_CORPS        498        //������Ҫ���뿪����
#define HC_ERROR_GRAFT_GENIUS_NUM    499        //��ֲ�츳��Ҫ�����츳��ƥ��
#define HC_ERROR_GRAFT_GENIUS_COLOR    500        //��ֲ��ɫ�츳��Ҫ����º�
#define HC_ERROR_GRAFT_GENIUS        501        //��ָ����ͬӢ����ֲ�츳

#define HC_ERROR_NOT_ENOUGH_SUPPLY    502    //��������
#define HC_ERROR_NOT_ENOUGH_GONGXUN    503    //��ѫ����

#define HC_ERROR_BAG_FULL    504    //��������

#define HC_ERROR_XIANGQIAN_SAME_TYPE    505 //�Ѿ���Ƕ��ͬ�����͵ı�ʯ
#define HC_ERROR_TOO_MUCH_GENERALS    506    //�佫�����ﵽ����

#define HC_ERROR_SERVANT_RESCUE 507    //�Ҷ���ȶ�ͬĿ��һ��һ��

#define HC_ERROR_NOT_ENOUGH_GENERAL_LEVEL    508    //�佫�ȼ����㣬�޷��������������佫�ȼ�

#define HC_ERROR_SERVANT    509    //û�з������ץ����Ŀ��

#define HC_ERROR_XIANGQIAN_FULL    510 //��ʯ��Ƕ���������������ȼ�������Ƕ��

#define HC_ERROR_FRIEND_ERROR    511 //�ý�ɫ������
#define HC_ERROR_FRIEND_YET    512 //�������������������б���

#define HC_ERROR_ZHEN_NEED_ONE    513 //�������ٱ���һ���佫
#define HC_ERROR_ZHEN    514             //�޷�����佫�����������ٱ���һ���佫
#define HC_ERROR_UPZHEN_FULL 515        //�����佫��������
#define HC_ERROR_IN_COOLTIME_RACE 516    //������CD��
#define HC_ERROR_ENHANCE_CD 517    //ǿ��װ����ȴ��
#define HC_ERROR_NOT_ENOUGH_REBORN_POINT 518    //�����㲻�㣬��ѵ�����佫

#define HC_ERROR_NOT_ENOUGH_RACESCORE 519    //��������������

#define HC_ERROR_NOT_ENOUGH_CHUANCHENG 520    //���е�����
#define HC_ERROR_SAME_GENERAL 521    //�佫��ͬ

#define HC_ERROR_NOT_ENOUGH_BAG_SIZE 522    //�ֿⲻ��

#define HC_ERROR_NO_CONGRATULATION_TIMES 523        //ף�ش�������
#define HC_ERROR_NO_BE_CONGRATULATIONED_TIMES 524    //��ף�ش�������

#define HC_ERROR_FARM_WATER_TIMES 525    //���ս��ཱ����������30��
#define HC_ERROR_NO_FARM_WATER_FRIEND 526    //�޿ɹ�Ⱥ���

#define HC_ERROR_IN_TRADE    527            //ó�׽�����
#define HC_ERROR_IN_TRADE_CD    528        //ó�׳�ȡ��ȴ��

#define HC_ERROR_CORPS_CON        529        //���Ź��ײ���

#define HC_ERROR_CORPS_EXPLORE_NO_TIMES    530    //����̽����������

#define HC_ERROR_CORPS_YMSJ_GET_AWARD_FIRST 531    //����ȡ�������������ñ����������ȡǰ������ʧ��
#define HC_ERROR_DEFAULT_ZHEN    532             //Ĭ��������������һ���佫

#define HC_ERROR_NAME    533    //�ý�ɫ��������

#define HC_ERROR_XIANGQIAN_WULI_ERR    534//�ñ�ʯֻ�Բ��Թ����佫��Ч
#define HC_ERROR_XIANGQIAN_CELUE_ERR    535    //�ñ�ʯֻ���������佫��Ч

#define HC_ERROR_USE_VIP_CARD 536    //ʹ�ô˵������ӵ�vip�����޷��������޵�80%

#define HC_ERROR_SERVANT_YUSHIMAX    537    //���հ�����ʯ�Ѵ����ޣ����������ȼ��Լ�����������������������
#define HC_ERROR_NEED_FRIEND         538    //�㵱ǰ�����ѿ����룬�����������
#define HC_ERROR_SUPPLY_MAX         539    //��ǰ�����Ѵ�����

#define HC_ERROR_JOIN_CORPS_TO_USE 540      //����Ҫ������Ų��ܿ����˹���
#define HC_ERROR_JOIN_CORPS2_TO_USE 541     //����Ҫ������ţ��Ҿ�������2�����ܿ����˹���
#define HC_ERROR_HORSE_NOT_OPEN     542     //����Ҫ����16�����ܿ����˹���(ս������)
#define HC_ERROR_LEVY_NOT_OPEN      543     //����Ҫ����8�����ܿ����˹���(����)
#define HC_ERROR_SOUL_NOT_OPEN      544     //����Ҫ����34�����ܿ����˹���(�ݱ�)
#define HC_ERROR_TRADE_NOT_OPEN     545     //����Ҫ����32�����ܿ����˹���(ó��)
#define HC_ERROR_TRAIN_NOT_OPEN     546     //����Ҫ����11�����ܿ����˹���(ѵ��)

#define HC_ERROR_CORPS_FIGHTING_NOT_JOIN 547    //���ľ���û�вμӾ���ս
#define HC_ERROR_CORPS_FIGHTING_LOSE 548        //���ź��������ھ�����ʧ�ܡ�

#define HC_ERROR_CORPS_BOSS_NOT_END 549     //�������޽�����ſ����á�


