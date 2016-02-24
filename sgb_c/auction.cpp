
#include "auction.h"
#include "errcode_def.h"
#include "const_def.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include "utils_lang.h"
#include <syslog.h>
#include "statistics.h"
#include "singleton.h"
#include "SaveDb.h"

extern Database& GetDb();
extern void InsertSaveDb(const std::string& sql);


// this void creates new auction and adds auction to some auctionhouse
int ProcessAuctionSellItem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int etime = 0, bid = 0, buyout = 0, count = 0, type = 0;
    READ_INT_FROM_MOBJ(etime,o,"etime");
    READ_INT_FROM_MOBJ(bid,o,"bid");
    READ_INT_FROM_MOBJ(buyout,o,"buyout");
    READ_INT_FROM_MOBJ(count,o,"count");
    READ_INT_FROM_MOBJ(type,o,"type");
    if (!bid || !etime)
        return HC_ERROR;
    switch (etime)
    {
        case 1*MIN_AUCTION_TIME:
        case 2*MIN_AUCTION_TIME:
        case 4*MIN_AUCTION_TIME:
            break;
        default:
            return HC_ERROR;
    }
    int deposit = Singleton<AuctionHouseMgr>::Instance().GetAuctionDeposit(etime);
    if (cdata->gold(true) < deposit)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    Item newItem;
    if (type == ITEM_TYPE_CURRENCY)
    {
        int tid = 0;
        READ_INT_FROM_MOBJ(tid,o,"id");
        if (tid != CURRENCY_ID_SILVER)
        {
            return HC_ERROR;
        }
        if (cdata->subSilver(count,silver_cost_auction) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GEM;
        }
        newItem.type = type;
        newItem.id = tid;
        newItem.nums = count;
    }
    else if (type == ITEM_TYPE_GEM)
    {
        int tid = 0;
        READ_INT_FROM_MOBJ(tid,o,"id");
        if (cdata->subGem(tid,count,gem_cost_auction) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GEM;
        }
        newItem.type = type;
        newItem.id = tid;
        newItem.nums = count;
    }
    else if(type == ITEM_TYPE_EQUIPMENT)
    {
        int id = 0;
        READ_INT_FROM_MOBJ(id,o,"id");
        Equipment* equip = cdata->m_bag.getEquipById(id);
        if (!equip)
        {
            return HC_ERROR;
        }
        newItem.type = type;
        newItem.id = equip->getSubType();
        newItem.nums = 1;
        newItem.extra = equip->getQuality();
        newItem.extra2 = equip->getLevel();
        cdata->m_bag.removeItem(equip->getSlot());
    }
    else if(type == ITEM_TYPE_HERO)
    {
        int id = 0;
        READ_INT_FROM_MOBJ(id,o,"id");
        boost::shared_ptr<CharHeroData> p_hero = cdata->m_heros.GetHero(id);
        if (!p_hero.get() || p_hero->isWork())
        {
            return HC_ERROR;
        }
        newItem.type = type;
        newItem.id = p_hero->m_hid;
        newItem.nums = 1;
        newItem.extra = p_hero->m_star;
        newItem.extra2 = p_hero->m_level;
        newItem.d_extra[0] = p_hero->m_add[0];
        newItem.d_extra[1] = p_hero->m_add[1];
        newItem.d_extra[2] = p_hero->m_add[2];
        newItem.d_extra[3] = p_hero->m_add[3];
        cdata->m_heros.Sub(id);
    }
    else
    {
        HC_ERROR;
    }
    cdata->subGold(deposit,gold_cost_auction,true);
    Singleton<AuctionHouseMgr>::Instance().AddAuction(newItem, etime, bid, buyout, deposit, cdata);
    if (newItem.extra >= 5)
    {
        std::string msg = strAuctionMsg;
        str_replace(msg, "$R", newItem.toString(true));
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
    }
    return HC_SUCCESS;
}

// this function is called when client bids or buys out auction
int ProcessAuctionPlaceBid(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int auctionId = 0, price = 0;
    READ_INT_FROM_MOBJ(auctionId,o,"auctionId");
    READ_INT_FROM_MOBJ(price,o,"price");
    if (!price || !auctionId)
        return HC_ERROR;

    AuctionItem *auction = Singleton<AuctionHouseMgr>::Instance().GetAuction(auctionId);
    if (!auction || auction->owner == cdata->m_id)
    {
        return HC_ERROR_AUCTION_BID_OWN;
    }
    // cheating
    if (price < auction->startbid)
        return HC_ERROR_AUCTION_HIGHER_BID;
    if (price <= auction->bid)
    {
        return HC_ERROR_AUCTION_HIGHER_BID;
    }
    // price too low for next bid if not buyout
    if ((price < auction->buyout || auction->buyout == 0) &&
        price < auction->bid + auction->GetAuctionOutBid())
    {
        return HC_ERROR_AUCTION_HIGHER_BID;
    }
    if (cdata->gold() < price)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    auction->UpdateBid(price, cdata);
    return HC_SUCCESS;
}

// this void is called when auction_owner cancels his auction
int ProcessAuctionRemoveItem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int auctionId = 0;
    READ_INT_FROM_MOBJ(auctionId,o,"auctionId");
    if (!auctionId)
        return HC_ERROR;

    AuctionItem *auction = Singleton<AuctionHouseMgr>::Instance().GetAuction(auctionId);
    if (!auction || auction->owner != cdata->m_id)
    {
        return HC_ERROR;
    }
    //已经有竞拍则返还拍卖费
    if (auction->bid)
    {
        int auctionCut = auction->GetAuctionCut();
        if (cdata->gold(true) < auctionCut)
            return HC_ERROR_NOT_ENOUGH_GOLD;
        if (auction->bidder)
            Singleton<AuctionHouseMgr>::Instance().SendAuctionCancelledToBidderMail(auction);
        cdata->subGold(auctionCut,gold_cost_auction,true);
    }
    // Return the item by mail
    std::list<Item> items;
    items.push_back(auction->item);
    std::string attack_info = itemlistToAttach(items);
    std::string content = strAuctionRemoveMailContent;
    str_replace(content, "$R", itemlistToString(items));
    sendSystemMail(cdata->m_name, auction->owner, strAuctionMailTitle, content, attack_info);
    // Now remove the auction
    auction->DeleteFromDB();
    Singleton<AuctionHouseMgr>::Instance().RemoveAuction(auction->id);
    delete auction;
    return HC_SUCCESS;
}

// called when player lists his bids
int ProcessAuctionListBidderItems(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //页面信息
    int page = 1, nums_per_page = 0;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 10;
    }
    json_spirit::Array list;
    int cur_nums = 0;
    Singleton<AuctionHouseMgr>::Instance().BuildListBidderItems(list, cdata, page, nums_per_page, cur_nums);
    robj.push_back( Pair("list", list));
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

// this void sends player info about his auctions
int ProcessAuctionListOwnerItems(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //页面信息
    int page = 1, nums_per_page = 0;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 10;
    }
    json_spirit::Array list;
    int cur_nums = 0;
    Singleton<AuctionHouseMgr>::Instance().BuildListOwnerItems(list, cdata, page, nums_per_page, cur_nums);
    robj.push_back( Pair("list", list));
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

int ProcessAuctionListItems(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //页面信息
    int page = 1, nums_per_page = 0;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 10;
    }
    //筛选条件
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    robj.push_back( Pair("type", type));
    
    
    AuctionHouseMgr::AuctionItemMap const& aucs = Singleton<AuctionHouseMgr>::Instance().GetAuctions();
    std::vector<AuctionItem*> auctions;
    auctions.reserve(aucs.size());

    for (AuctionHouseMgr::AuctionItemMap::const_iterator itr = aucs.begin(); itr != aucs.end(); ++itr)
        auctions.push_back(itr->second);
    //筛选
    json_spirit::Array list;
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    for (std::vector<AuctionItem*>::const_iterator itr = auctions.begin(); itr != auctions.end(); ++itr)
    {
        AuctionItem *AenItm = *itr;
        if (AenItm->moneyDeliveryTime)                      // skip pending sell auctions
            continue;
        if (type != 0 && AenItm->item.type != type)
            continue;
        if (type == ITEM_TYPE_GEM)
        {
            int usage = 0;
            READ_INT_FROM_MOBJ(usage, o, "usage");
            robj.push_back( Pair("usage", usage));
            boost::shared_ptr<baseGem> tr = GeneralDataMgr::getInstance()->GetBaseGem(AenItm->item.id);
            if (!tr.get())
                continue;
            if (usage != 0 && tr->usage != usage)
                continue;
        }
        else if(type == ITEM_TYPE_EQUIPMENT)
        {
            int quality = 0;
            READ_INT_FROM_MOBJ(quality, o, "quality");
            robj.push_back( Pair("quality", quality));
            if (quality != 0 && AenItm->item.extra != quality)
                continue;
        }
        else if(type == ITEM_TYPE_HERO)
        {
            int star = 0;
            READ_INT_FROM_MOBJ(star, o, "star");
            robj.push_back( Pair("star", star));
            if (star != 0 && AenItm->item.extra != star)
                continue;
        }
        ++cur_nums;
        if (cur_nums >= first_nums && cur_nums <= last_nums)
        {
            json_spirit::Object obj;
            if (AenItm->toObj(obj))
            {
                list.push_back(obj);
            }
        }
    }
    robj.push_back( Pair("list", list));
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

AuctionHouseMgr::AuctionHouseMgr()
{
    Load();
}

void AuctionHouseMgr::Update()
{
    time_t curTime = time(NULL);
    for (AuctionItemMap::iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); )
    {
        if (itr->second->moneyDeliveryTime)
        {
            if (curTime > itr->second->moneyDeliveryTime)
            {
                SendAuctionSuccessfulMail(itr->second);
                itr->second->DeleteFromDB();
                delete itr->second;
                AuctionsMap.erase(itr++);
                continue;
            }
        }
        else
        {
            if (curTime > itr->second->expireTime)
            {
                if (itr->second->bid)//最后竞拍获胜
                {
                    itr->second->AuctionBidWinning();
                }
                else//拍卖失败
                {
                    SendAuctionExpiredMail(itr->second);

                    itr->second->DeleteFromDB();
                    delete itr->second;
                    AuctionsMap.erase(itr++);
                    continue;
                }
            }
        }
        ++itr;
    }
}

void AuctionHouseMgr::BuildListBidderItems(json_spirit::Array& list, CharData* player, int page, int nums_per_page, int& cur_nums)
{
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    for (AuctionItemMap::const_iterator itr = AuctionsMap.begin();itr != AuctionsMap.end();++itr)
    {
        AuctionItem *AenItm = itr->second;
        if (AenItm->moneyDeliveryTime)                      // skip pending sell auctions
            continue;
        if (AenItm->bidder == player->m_id)
        {
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                json_spirit::Object obj;
                if (AenItm->toObj(obj))
                {
                    list.push_back(obj);
                }
            }
        }
    }
}

void AuctionHouseMgr::BuildListOwnerItems(json_spirit::Array& list, CharData* player, int page, int nums_per_page, int& cur_nums)
{
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    for (AuctionItemMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionItem *AenItm = itr->second;
        if (AenItm->moneyDeliveryTime)                      // skip pending sell auctions
            continue;
        if (AenItm->owner == player->m_id)
        {
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                json_spirit::Object obj;
                if (AenItm->toObj(obj))
                {
                    list.push_back(obj);
                }
            }
        }
    }
}

void AuctionHouseMgr::BuildListAllItems(json_spirit::Array& list, CharData* player, int& count)
{
    for (AuctionItemMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionItem *AenItm = itr->second;
        if (AenItm->moneyDeliveryTime)                      // skip pending sell auctions
            continue;
        json_spirit::Object obj;
        if (AenItm->toObj(obj))
        {
            ++count;
            list.push_back(obj);
        }
    }
}

AuctionItem* AuctionHouseMgr::AddAuction(Item newItem, int etime, int bid, int buyout, int deposit, CharData* pl /*= NULL*/)
{
    AuctionItem *AH = new AuctionItem;
    AH->id = ++max_aid;
    AH->item = newItem;
    AH->owner = pl ? pl->m_id : 0;
    AH->ownerName = pl ? pl->m_name : "";

    AH->startbid = bid;
    AH->bidder = 0;
    AH->bid = 0;
    AH->buyout = buyout;
    AH->expireTime = time(NULL) + etime;
    AH->moneyDeliveryTime = 0;
    AH->deposit = deposit;

    AddAuction(AH);
    AH->SaveToDB();
    return AH;
}

// does not clear ram
void AuctionHouseMgr::SendAuctionWonMail(AuctionItem *auction)
{
    CharData* bidder = GeneralDataMgr::getInstance()->GetCharData(auction->bidder).get();
    if (bidder)
    {
        std::list<Item> items;
        items.push_back(auction->item);
        std::string attack_info = itemlistToAttach(items);
        std::string content = strAuctionWonMailContent;
        str_replace(content, "$G", LEX_CAST_STR(auction->bid));
        str_replace(content, "$N", MakeCharNameLink(auction->ownerName));
        str_replace(content, "$R", itemlistToString(items));
        sendSystemMail(bidder->m_name, auction->bidder, strAuctionMailTitle, content, attack_info, 0, 0, loot_auction);
    }
}

// call this method to send mail to auction owner, when auction is successful, it does not clear ram
void AuctionHouseMgr::SendAuctionSuccessfulMail(AuctionItem * auction)
{
    CharData* auction_owner = GeneralDataMgr::getInstance()->GetCharData(auction->owner).get();
    CharData* bidder = GeneralDataMgr::getInstance()->GetCharData(auction->bidder).get();
    if (auction_owner && bidder)
    {
        std::list<Item> items;
        int auctionCut = auction->GetAuctionCut();
        int profit = auction->bid + auction->deposit - auctionCut;
        Item tmp(ITEM_TYPE_CURRENCY, CURRENCY_ID_GOLD, profit, 0);
        items.push_back(tmp);
        std::string attack_info = itemlistToAttach(items);
        std::string content = strAuctionSuccessMailContent;
        str_replace(content, "$N", MakeCharNameLink(bidder->m_name));
        str_replace(content, "$R", auction->item.toString(true));
        str_replace(content, "$B", LEX_CAST_STR(auction->bid));
        str_replace(content, "$C", LEX_CAST_STR(auctionCut));
        str_replace(content, "$D", LEX_CAST_STR(auction->deposit));
        str_replace(content, "$G", LEX_CAST_STR(profit));
        sendSystemMail(auction_owner->m_name, auction->owner, strAuctionMailTitle, content, attack_info, 0, 0, loot_auction);
    }
}

// does not clear ram
void AuctionHouseMgr::SendAuctionExpiredMail(AuctionItem * auction)
{
    CharData* auction_owner = GeneralDataMgr::getInstance()->GetCharData(auction->owner).get();
    if (auction_owner)
    {
        std::list<Item> items;
        items.push_back(auction->item);
        std::string attack_info = itemlistToAttach(items);
        std::string content = strAuctionExpiredMailContent;
        str_replace(content, "$R", itemlistToString(items));
        sendSystemMail(auction_owner->m_name, auction->owner, strAuctionMailTitle, content, attack_info);
    }
}

void AuctionHouseMgr::SendAuctionOutbiddedMail(AuctionItem *auction)
{
    CharData* bidder = GeneralDataMgr::getInstance()->GetCharData(auction->bidder).get();
    if (bidder)
    {
        std::list<Item> items;
        Item tmp(ITEM_TYPE_CURRENCY, CURRENCY_ID_GOLD, auction->bid, 0);
        items.push_back(tmp);
        std::string attack_info = itemlistToAttach(items);
        std::string content = strAuctionOutbiddedMailContent;
        str_replace(content, "$R", auction->item.toString(true));
        sendSystemMail(bidder->m_name, auction->bidder, strAuctionMailTitle, content, attack_info);
    }
}

void AuctionHouseMgr::SendAuctionCancelledToBidderMail(AuctionItem* auction)
{
    CharData* bidder = GeneralDataMgr::getInstance()->GetCharData(auction->bidder).get();
    if (bidder)
    {
        std::list<Item> items;
        Item tmp(ITEM_TYPE_CURRENCY, CURRENCY_ID_GOLD, auction->bid, 0);
        items.push_back(tmp);
        std::string attack_info = itemlistToAttach(items);
        std::string content = strAuctionCancelledMailContent;
        str_replace(content, "$R", auction->item.toString(true));
        str_replace(content, "$N", MakeCharNameLink(auction->ownerName));
        sendSystemMail(bidder->m_name, auction->bidder, strAuctionMailTitle, content, attack_info);
    }
}

int AuctionHouseMgr::GetAuctionDeposit(int time)
{
    return (time / MIN_AUCTION_TIME) * 10;
}

void AuctionHouseMgr::Load()
{
    Query q(GetDb());
    q.get_result("SELECT id,itemType,itemId,counts,extra,extra2,add1,add2,add3,add4,itemowner,buyoutprice,expireTime,moneyTime,buy_cid,lastbid,startbid,deposit FROM sys_auction");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        AuctionItem *auction = new AuctionItem;
        auction->id = q.getval();
        
        auction->item.type = q.getval();
        auction->item.id = q.getval();
        auction->item.nums = q.getval();
        auction->item.extra = q.getval();
        auction->item.extra2 = q.getval();
        auction->item.d_extra[0] = q.getnum();
        auction->item.d_extra[1] = q.getnum();
        auction->item.d_extra[2] = q.getnum();
        auction->item.d_extra[3] = q.getnum();
        
        auction->owner = q.getval();
        if (auction->owner)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(auction->owner).get();
            if (pc)
            {
                auction->ownerName = pc->m_name;
            }
        }
        auction->buyout = q.getval();
        auction->expireTime = q.getval();
        auction->moneyDeliveryTime = q.getval();
        auction->bidder = q.getval();
        auction->bid = q.getval();
        auction->startbid = q.getval();
        auction->deposit = q.getval();
        AddAuction(auction);
        max_aid = auction->id;
    }
}

bool AuctionItem::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", id));
    obj.push_back( Pair("owner", ownerName));
    if (bid > 0 && bidder > 0)
    {
        CharData* c = GeneralDataMgr::getInstance()->GetCharData(bidder).get();
        if (bidder)
        {
            obj.push_back( Pair("bidder", c->m_name));
        }
    }
    obj.push_back( Pair("startbid", startbid));
    obj.push_back( Pair("bid", bid));
    obj.push_back( Pair("buyout", buyout));
    obj.push_back( Pair("left", (expireTime-time(NULL))));
    json_spirit::Object itemObj;
    item.toObj(itemObj);
    obj.push_back( Pair("itemObj", itemObj));
    return true;
}

int AuctionItem::GetAuctionCut() const
{
    int cut = int(bid * 5.0f / 100.0f);
    if (cut < 5)
        cut = 5;
    return cut;
}

int AuctionItem::GetAuctionOutBid() const
{
    int outbid = 1;
    return outbid;
}

void AuctionItem::DeleteFromDB() const
{
    InsertSaveDb("DELETE FROM sys_auction WHERE id =" + LEX_CAST_STR(id));
}

void AuctionItem::SaveToDB() const
{
    InsertSaveDb("INSERT INTO sys_auction (id,itemType,itemId,counts,extra,extra2,add1,add2,add3,add4,itemowner,buyoutprice,expireTime,moneyTime,buy_cid,lastbid,startbid,deposit) "
        "VALUES ('"+LEX_CAST_STR(id)+"', '"+LEX_CAST_STR(item.type)+"', '"+LEX_CAST_STR(item.id)
        +"', '"+LEX_CAST_STR(item.nums)+"', '"+LEX_CAST_STR(item.extra)+"', '"+LEX_CAST_STR(item.extra2)
        +"', '"+LEX_CAST_STR(item.d_extra[0])+"', '"+LEX_CAST_STR(item.d_extra[1])+"', '"+LEX_CAST_STR(item.d_extra[2])
        +"', '"+LEX_CAST_STR(item.d_extra[3])+"', '"+LEX_CAST_STR(owner)+"', '"+LEX_CAST_STR(buyout)
        +"', '"+LEX_CAST_STR(expireTime)+"', '"+LEX_CAST_STR(moneyDeliveryTime)+"', '"+LEX_CAST_STR(bidder)
        +"', '"+LEX_CAST_STR(bid)+"', '"+LEX_CAST_STR(startbid)+"', '"+LEX_CAST_STR(deposit)+"')");
}

void AuctionItem::AuctionBidWinning(CharData* newbidder)
{
    moneyDeliveryTime = time(NULL);
    Singleton<AuctionHouseMgr>::Instance().SendAuctionWonMail(this);
}

bool AuctionItem::UpdateBid(int newbid, CharData* newbidder /*=NULL*/)
{
    CharData* auction_owner = owner ? GeneralDataMgr::getInstance()->GetCharData(owner).get() : NULL;
    if (auction_owner == NULL)
    {
        return false;
    }

    // bid can't be greater buyout
    if (buyout && newbid > buyout)
        newbid = buyout;

    if (newbidder && newbidder->m_id == bidder)
    {
        newbidder->subGold((newbid - bid));
    }
    else
    {
        if (newbidder)
            newbidder->subGold(newbid);

        if (bidder)// return money to old bidder if present
            Singleton<AuctionHouseMgr>::Instance().SendAuctionOutbiddedMail(this);
    }

    bidder = newbidder ? newbidder->m_id : 0;
    bid = newbid;

    if ((newbid < buyout) || (buyout == 0))                 // bid
    {
        InsertSaveDb("UPDATE sys_auction SET buyguid = '"+LEX_CAST_STR(bidder)+"', buy_cid = '"+LEX_CAST_STR(bid)+"' WHERE id = '"+LEX_CAST_STR(id)+"'");
        return true;
    }
    else                                                    // buyout
    {
        AuctionBidWinning(newbidder);
        return false;
    }
}

