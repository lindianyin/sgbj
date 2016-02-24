
#ifndef _AUCTION_H
#define _AUCTION_H

#include <string>
#include <map>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "utils_all.h"
#include "data.h"
#include "net.h"

struct Item;
struct CharData;

#define MIN_AUCTION_TIME (12*HOUR)

//拍卖物品
struct AuctionItem
{
    int id;//拍卖id
    Item item;//物品
    int owner;//拥有者
    std::string ownerName;
    int startbid;//起拍价
    int bid;//当前竞价
    int buyout;//一口价
    time_t expireTime;//到期时间
    time_t moneyDeliveryTime;//竞价成功时间
    int bidder;//出价最高者
    int deposit;//保证金
    
    int GetAuctionCut() const;
    int GetAuctionOutBid() const;
    bool toObj(json_spirit::Object& obj);
    void DeleteFromDB() const;
    void SaveToDB() const;
    void AuctionBidWinning(CharData* bidder = NULL);

    bool UpdateBid(int newbid, CharData* newbidder = NULL);// true if normal bid, false if buyout, bidder==NULL for generated bid
};

//拍卖行
class AuctionHouseMgr
{
    public:
        AuctionHouseMgr();
        ~AuctionHouseMgr()
        {
            for (AuctionItemMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
                delete itr->second;
        }
        

        typedef std::map<int, AuctionItem*> AuctionItemMap;
        typedef std::pair<AuctionItemMap::const_iterator, AuctionItemMap::const_iterator> AuctionItemMapBounds;

        int GetCount() { return AuctionsMap.size(); }

        AuctionItemMap const& GetAuctions() const { return AuctionsMap; }
        AuctionItemMapBounds GetAuctionsBounds() const {return AuctionItemMapBounds(AuctionsMap.begin(), AuctionsMap.end()); }

        void AddAuction(AuctionItem *ah)
        {
            assert( ah );
            AuctionsMap[ah->id] = ah;
        }

        AuctionItem* GetAuction(int id) const
        {
            AuctionItemMap::const_iterator itr = AuctionsMap.find( id );
            return itr != AuctionsMap.end() ? itr->second : NULL;
        }

        bool RemoveAuction(int id)
        {
            return AuctionsMap.erase(id);
        }

        void Update();

        void BuildListBidderItems(json_spirit::Array& list, CharData* player, int page, int nums_per_page, int& cur_nums);
        void BuildListOwnerItems(json_spirit::Array& list, CharData* player, int page, int nums_per_page, int& cur_nums);
        void BuildListAllItems(json_spirit::Array& list, CharData* player, int& count);

        AuctionItem* AddAuction(Item newItem, int etime, int bid, int buyout = 0, int deposit = 0, CharData * pl = NULL);
        
        //拍卖物品结束处理
        void SendAuctionWonMail( AuctionItem * auction );
        void SendAuctionSuccessfulMail( AuctionItem * auction );
        void SendAuctionExpiredMail( AuctionItem * auction );
        void SendAuctionOutbiddedMail(AuctionItem *auction);
        void SendAuctionCancelledToBidderMail(AuctionItem* auction);
        static int GetAuctionDeposit(int time);
        void Load();
    private:
        AuctionItemMap AuctionsMap;
        int max_aid;
};

//开始拍卖物品
int ProcessAuctionSellItem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//竞拍物品
int ProcessAuctionPlaceBid(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//取消拍卖
int ProcessAuctionRemoveItem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//竞拍列表
int ProcessAuctionListBidderItems(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//拍卖列表
int ProcessAuctionListOwnerItems(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//搜索商品
int ProcessAuctionListItems(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

#endif
