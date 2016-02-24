
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

//������Ʒ
struct AuctionItem
{
    int id;//����id
    Item item;//��Ʒ
    int owner;//ӵ����
    std::string ownerName;
    int startbid;//���ļ�
    int bid;//��ǰ����
    int buyout;//һ�ڼ�
    time_t expireTime;//����ʱ��
    time_t moneyDeliveryTime;//���۳ɹ�ʱ��
    int bidder;//���������
    int deposit;//��֤��
    
    int GetAuctionCut() const;
    int GetAuctionOutBid() const;
    bool toObj(json_spirit::Object& obj);
    void DeleteFromDB() const;
    void SaveToDB() const;
    void AuctionBidWinning(CharData* bidder = NULL);

    bool UpdateBid(int newbid, CharData* newbidder = NULL);// true if normal bid, false if buyout, bidder==NULL for generated bid
};

//������
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
        
        //������Ʒ��������
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

//��ʼ������Ʒ
int ProcessAuctionSellItem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������Ʒ
int ProcessAuctionPlaceBid(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ȡ������
int ProcessAuctionRemoveItem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����б�
int ProcessAuctionListBidderItems(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����б�
int ProcessAuctionListOwnerItems(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������Ʒ
int ProcessAuctionListItems(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

#endif
