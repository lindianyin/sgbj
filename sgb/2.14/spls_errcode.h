#pragma once

#define HC_SUCCESS              200            //成功

#define HC_SUCCESS_NO_RET        201                //成功，不返回给客户端
#define HC_ERROR_NO_RET        202                //已经添加错误码和信息

#define HC_ERROR                401            //错误
#define HC_ERROR_WRONG_ACCOUNT  402          //账号不存在
#define HC_ERROR_WRONG_PASSWORD 403          //密码错误
#define HC_ERROR_LOGIN_FIRST    404             //未登录
#define HC_ERROR_LOGOUT_FIRST   405              //已经登录
#define HC_ERROR_CHAR_EXIST     406           //角色名已经存在
#define HC_ERROR_CHAR_NOT_ONLINE    407      //角色不在线
#define HC_ERROR_NAME_TOO_LONG      408          //角色名太长
#define HC_ERROR_FORBIDEN_CHAT      409          //被禁言
#define HC_ERROR_NOT_JOIN_JT        410          //你没有加入军团

#define HC_ERROR_NOT_ENOUGH_SILVER     411     //你的银子不够。
#define HC_ERROR_NOT_ENOUGH_GOLD     412     //你的金币不够。

#define HC_ERROR_MORE_VIP_LEVEL 413            //vip等级不够
#define HC_ERROR_SKILL_QUEUE_MAX    414            //技能研究队列不能再增加了

#define HC_ERROR_NO_ATTACK_TIMES    415            //没有剩余的攻击次数

#define HC_ERROR_NOT_ENOUGH_LING    416            //你的军令不够。

#define HC_ERROR_SKILL_LEVEL_MAX    417            //你的技能达到当前最高

#define HC_ERROR_HAS_GENERAL_ALREADY 418        //已经有该武将
#define HC_ERROR_GENERALTREASURE_MAX 419        //武将宝物满级

#define HC_ERROR_IN_COOLTIME 420        //冷却时间未结束
#define HC_ERROR_TARGET_IS_BUSY    421    //目标正忙

#define HC_ERROR_WAIT_FOR_FARM_FINISH    422    //等候全部屯田结束
#define HC_ERROR_NEED_MORE_RIPE    423    //成熟度不足
#define HC_ERROR_WRONG_SEASON 424    //季节不对

#define HC_ERROR_BACKPACK_FULL_NO_UNEQIPT 425    //仓库已满，不能卸下装备
#define HC_ERROR_BACKPACK_FULL_GET_EQUIPT 426    //仓库已满，请到回购中获取物品
#define HC_ERROR_BACKPACK_FULL_BUYBACK 427        //仓库已满，不能回购
#define HC_ERROR_NEED_MORE_MATURITY 428            //宝箱成熟度不足10%，不能领取
#define HC_ERROR_GENERAL_LEVEL_MAX    429            //英雄等级达到当前最高

#define HC_ERROR_ALREADY_IN_A_CORPS      430      //你已经加入军团
#define HC_ERROR_CORPS_NAME_EXIST          431        //军团名字已经存在
#define HC_ERROR_CORPS_ALREADY_APPLY     432        //你已经提交了申请
#define HC_ERROR_CORPS_MAX_APPLY         433        //你最多只能提交三个申请
#define HC_ERROR_CORPS_NEED_MORE_LEV        434        //您的等级不足19级，不能创建或加入军团
#define HC_ERROR_QUEUE_FULL    435        //队列已满
#define HC_ERROR_CORPS_OFFICAL_LIMIT        436        //您没有权限

#define HC_ERROR_NOT_ENOUGH_STONE    437    //强化石数量不足
#define HC_ERROR_UPGRADE_MAX_LEVEL    438    //装备已经强化到最高级
#define HC_ERROR_MAP_NOT_OPEN            439    //地图暂未开放
#define HC_ERROR_SKILL_RESEARCHING    440    //该技能已经在研究队列中

#define HC_ERROR_CORPS_MAX_ASSISTANT     441//军团的副军团长职位数量已达上限
#define HC_ERROR_CORPS_MAX_MEMBERS     442//军团成员数量已达上限
#define HC_ERROR_CORPS_NOT_SAME_CAMP     443//您只能加入同一阵营的军团

#define HC_ERROR_WEAPON_NOT_ENOUGH_LEVEL 444//您的等级不够，暂时不能购买这个兵器

#define HC_ERROR_SEND_MAIL_INVALID_DEST 445    //发送失败，收信人不存在
#define HC_ERROR_BOSS_EVENT_END 446        //boss战已经结束
#define HC_ERROR_BOSS_WAITING 447            //BOSS战即将开启，请稍候
#define HC_ERROR_BOSS_NOT_OPEN 448        //BOSS战尚未开启
#define HC_ERROR_BOSS_INSPIRE_FAIL 449    //很可惜，鼓舞失败了
#define HC_ERROR_BOSS_INSPIRE_MAX 450    //已经鼓舞到最高
#define HC_ERROR_BOSS_NOT_ENTER 451        //请先进入副本

#define HC_ERROR_ALREADY_GET_PACKS 452    //你已经领取过该礼包

#define HC_ERROR_AUTH_EXPIRED    453            //验证信息已经过期

#define HC_ERROR_NOT_ENOUGH_ORE    454        //矿石数量不足
#define HC_ERROR_BOSS_NOT_ENOUGH_LEVEL    455    //您的等级不足31级，无法加入boss战

#define HC_ERROR_CAMP_RACE_NO_CAMP    456    //没有阵营无法加入阵营战
#define HC_ERROR_CAMP_RACE_NOT_IN    457    //你没有在阵营战中
#define HC_ERROR_CAMP_RACE_NOT_OPEN    458    //阵营战还未开启

#define HC_ERROR_CORPS_NOT_IN_PARTY    459    //只有宴会的参加者才能发起召集
#define HC_ERROR_CORPS_PARYTY_LEVEL    460    //军团等级到达3级才能举办宴会
#define HC_ERROR_CORPS_ALREADY_JISI    461    //您今天已经祭祀过了
#define HC_ERROR_CORPS_ALREADY_IN_PARTY    462    //您已经在宴会中了

#define HC_ERROR_GUARD_NOT_ENOUGH_LEVEL 463 //主将等级不足，无法参加。需要主将等级52
#define HC_ERROR_GUARD_NO_ROB_TIMES 464 //该玩家已被截光了或者您的截纲次数为0，无法截取
#define HC_ERROR_GUARD_ROB_IS_COLD 465    //劫纲冷却中
#define HC_ERROR_GUARD_ROB_ALREADY 466    //你已经劫过此纲

#define HC_ERROR_NO_GROUP_COPY_TEAM    467        //请先加入一个队伍
#define HC_ERROR_INVALID_GROUP_COPY_ID 468    //不存在这个多人副本
#define HC_ERROR_NO_GROUP_COPY_ATTACK_TIME 469    //您今天不能再攻击这个副本了
#define HC_ERROR_INVALID_GROUP_COPY_TEAM 470    //队伍不存在，请刷新界面
#define HC_ERROR_GROUP_COPY_NEED_MORE_MEMBER 471    //人数不足，无法攻击副本
#define HC_ERROR_GROUP_COPY_NOT_LEADER 472        //您不是队长
#define HC_ERROR_GROUP_COPY_CAN_NOT_ATTACK 473    //地图未通关，无法攻击该副本
#define HC_ERROR_GROUP_COPY_ENTER_FIRST 474    //请先进入多人副本

#define HC_ERROR_TRADE_ALREADY 475    //你已经在通商中
#define HC_ERROR_TRADE_POS_ERROR 476    //该位置状态不对
#define HC_ERROR_TRADE_NO_TIMES 477    //今天次数耗尽
#define HC_ERROR_TRADE_NOT 478    //你不在通商中
#define HC_ERROR_TRADE_BE_PROTECT 479    //通商位置被保护

#define HC_ERROR_RESEARCH_TOO_TIRED    480        //太疲劳了，明天再来加速训练吧
#define HC_ERROR_CODE_IS_USED    481                //该激活码已经被使用
#define HC_ERROR_CODE_IS_INVALID    482            //无效的激活码
#define HC_ERROR_SMELT_FULL 483        //冶炼队列已满

#define HC_ERROR_NAME_ILLEGAL    484    //名字不合法

#define HC_ERROR_GROUP_COPY_NOT_OPEN    485    //多人副本还没有开启
#define HC_ERROR_ACCOUNT_BE_FREEZED    486    //帐号被冻结

#define HC_ERROR_NEED_MORE_PRESTIGE    487    //需要更多声望
#define HC_ERROR_CORPS_NEED_12H    488    //加入军团不足12小时不能使用该功能

#define HC_ERROR_NOT_ENOUGH_TIME 490    //没有足够次数
#define HC_ERROR_SERVANT_CD 491    //家丁冷却中
#define HC_ERROR_SERVANT_TOO_MORE 492    //家丁数量已满
#define HC_ERROR_SERVANT_LEVEL 493    //等级差距过大

#define HC_ERROR_NOT_ENOUGH_YUSHI    489    //您没有足够的玉石
#define HC_ERROR_BAOSHI_MAX_LEVEL    494    //宝石已经是最高品质
#define HC_ERROR_BAOSHI_NOS_POS        495    //没有多余的宝石孔

#define HC_ERROR_NEED_MORE_LEVEL    496        //主將等級不夠
#define HC_ERROR_REVOLT_CD            497        //叛逃冷却中
#define HC_ERROR_REVOLT_CORPS        498        //叛逃需要先离开军团
#define HC_ERROR_GRAFT_GENIUS_NUM    499        //移植天赋需要开启天赋数匹配
#define HC_ERROR_GRAFT_GENIUS_COLOR    500        //移植紫色天赋需要激活绰号
#define HC_ERROR_GRAFT_GENIUS        501        //需指定不同英雄移植天赋

#define HC_ERROR_NOT_ENOUGH_SUPPLY    502    //军粮不足
#define HC_ERROR_NOT_ENOUGH_GONGXUN    503    //功勋不足

#define HC_ERROR_BAG_FULL    504    //背包已满

#define HC_ERROR_XIANGQIAN_SAME_TYPE    505 //已经镶嵌了同样类型的宝石
#define HC_ERROR_TOO_MUCH_GENERALS    506    //武将人数达到上限

#define HC_ERROR_SERVANT_RESCUE 507    //家丁解救对同目标一天一次

#define HC_ERROR_NOT_ENOUGH_GENERAL_LEVEL    508    //武将等级不足，无法穿戴，请提升武将等级

#define HC_ERROR_SERVANT    509    //没有符合随机抓捕的目标

#define HC_ERROR_XIANGQIAN_FULL    510 //宝石镶嵌孔已满，请提升等级扩充镶嵌孔

#define HC_ERROR_FRIEND_ERROR    511 //该角色不存在
#define HC_ERROR_FRIEND_YET    512 //该盟友已在您的盟友列表中

#define HC_ERROR_ZHEN_NEED_ONE    513 //阵上至少保留一名武将
#define HC_ERROR_ZHEN    514             //无法解雇武将，阵型中至少保留一名武将
#define HC_ERROR_UPZHEN_FULL 515        //上阵武将人数已满
#define HC_ERROR_IN_COOLTIME_RACE 516    //竞技场CD中
#define HC_ERROR_ENHANCE_CD 517    //强化装备冷却中
#define HC_ERROR_NOT_ENOUGH_REBORN_POINT 518    //重生点不足，请训练该武将

#define HC_ERROR_NOT_ENOUGH_RACESCORE 519    //竞技场点数不足

#define HC_ERROR_NOT_ENOUGH_CHUANCHENG 520    //传承丹不足
#define HC_ERROR_SAME_GENERAL 521    //武将相同

#define HC_ERROR_NOT_ENOUGH_BAG_SIZE 522    //仓库不够

#define HC_ERROR_NO_CONGRATULATION_TIMES 523        //祝贺次数已满
#define HC_ERROR_NO_BE_CONGRATULATIONED_TIMES 524    //被祝贺次数已满

#define HC_ERROR_FARM_WATER_TIMES 525    //今日浇灌奖励次数已满30次
#define HC_ERROR_NO_FARM_WATER_FRIEND 526    //无可灌溉好友

#define HC_ERROR_IN_TRADE    527            //贸易进行中
#define HC_ERROR_IN_TRADE_CD    528        //贸易抽取冷却中

#define HC_ERROR_CORPS_CON        529        //军团贡献不足

#define HC_ERROR_CORPS_EXPLORE_NO_TIMES    530    //军团探索次数已满

#define HC_ERROR_CORPS_YMSJ_GET_AWARD_FIRST 531    //请领取奖励后再来。该宝箱在玩家领取前永不消失。
#define HC_ERROR_DEFAULT_ZHEN    532             //默认阵形最少上阵一名武将

#define HC_ERROR_NAME    533    //该角色名不存在

#define HC_ERROR_XIANGQIAN_WULI_ERR    534//该宝石只对策略攻击武将有效
#define HC_ERROR_XIANGQIAN_CELUE_ERR    535    //该宝石只对物理攻击武将有效

#define HC_ERROR_USE_VIP_CARD 536    //使用此道具增加的vip经验无法超过上限的80%

#define HC_ERROR_SERVANT_YUSHIMAX    537    //今日剥削玉石已达上限，请提高人物等级以及提升竞技场排名增加上限
#define HC_ERROR_NEED_FRIEND         538    //你当前无盟友可邀请，请先添加盟友
#define HC_ERROR_SUPPLY_MAX         539    //当前军粮已达上限

#define HC_ERROR_JOIN_CORPS_TO_USE 540      //您需要加入军团才能开启此功能
#define HC_ERROR_JOIN_CORPS2_TO_USE 541     //您需要加入军团，且军团升到2级才能开启此功能
#define HC_ERROR_HORSE_NOT_OPEN     542     //您需要升到16级才能开启此功能(战马培养)
#define HC_ERROR_LEVY_NOT_OPEN      543     //您需要升到8级才能开启此功能(征收)
#define HC_ERROR_SOUL_NOT_OPEN      544     //您需要升到34级才能开启此功能(演兵)
#define HC_ERROR_TRADE_NOT_OPEN     545     //您需要升到32级才能开启此功能(贸易)
#define HC_ERROR_TRAIN_NOT_OPEN     546     //您需要升到11级才能开启此功能(训练)

#define HC_ERROR_CORPS_FIGHTING_NOT_JOIN 547    //您的军团没有参加军团战
#define HC_ERROR_CORPS_FIGHTING_LOSE 548        //很遗憾，你所在军团已失败。

#define HC_ERROR_CORPS_BOSS_NOT_END 549     //当日神兽结束后才可设置。


