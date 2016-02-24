
#pragma once

#include <boost/cstdint.hpp>
#include <vector>

struct sevenCards;
struct myCard;

void debug_print_input(const sevenCards& hand);
void debug_print_result(const sevenCards& hand);
void debug_print(const myCard& c);


void random_init(sevenCards& hand);

void test_DB_card();


enum CARD_SUIT
{
    CLUB = 1,
    HEART = 2,
    DIAMOND = 3,//方块
    SPADE = 4,  //黑桃

    TOTAL_SUIT = 4
};

enum CARD_VALUE
{
    Deuce = 1,  //  2
    Trey =  2,  //  3
    Four = 3,   //  4
    Five = 4,   //  5
    Six = 5,    //  6
    Seven = 6,  //  7
    Eight = 7,  //  8
    Nine = 8,   //  9
    Ten = 9,    //  10
    Jack = 10,  //  J
    Queen = 11, //  Q
    King = 12,  //  K
    Ace = 13,   //  A
    Black_Joker = 14, //小王
    Red_Joker = 15    //大王
};

const int iTotalCardType = 13;

const int iScore[iTotalCardType] = {0x0,0x1,0x2,0x4,0x8,0x10,0x20,0x40,0x80,0x100,0x200,0x400,0x800};

enum HAND_RANK
{
    ROYAL_FLUSH = 1,
    SHELL_FLUSH = 2,
    STRAIGHT_FLUSH = 3,
    FOUR_OF_A_KIND = 4,
    FULL_HOUSE = 5,
    FLUSH = 6,
    STRAIGHT = 7,
    THREE_OF_A_KIND = 8,
    TWO_PAIR = 9,
    ONE_PAIR = 10,
    HIGH_CARD = 11,

    TOTAL_HAND_RANK = 11,
};

const int iDamagePer[TOTAL_HAND_RANK] = {28,23,21,19,17,15,13,11,9,7,5};

struct baseCard
{
    uint8_t suit;
    uint8_t value;
    void set(uint8_t s, uint8_t v)
    {
        suit = s;
        value = v;
    }
    bool valid() {return value > 0;}
};

struct myCard
{
    myCard()
    {
        pos = 0;
        suit = 0;
        value = 0;
        trans_suit = 0;
        trans_value = 0;
    }
    void set(uint8_t s, uint8_t v)
    {
        suit = s;
        value = v;
        trans_suit = s;
        trans_value = v;
    }
    uint8_t pos;
    uint8_t suit;
    uint8_t value;
    uint8_t trans_suit;
    uint8_t trans_value;
};

class myPoker
{
public:
    myPoker();
    void reShuffle();
    void reset_cur();
    baseCard* deal();
    void remove(uint8_t suit, uint8_t v);
    int findPos(uint8_t suit, uint8_t v);
    void swap(int pos1, int pos2);
private:
    int cur;
    std::vector<baseCard> cards;
};

struct sevenCards
{
    sevenCards();

    void reset();
    void resetEva();

    //7张牌，输入
    myCard cards[7];

    //中间数据
    //4种花色数量,0没使用
    int flushes[TOTAL_SUIT];
    myCard* cards_flushes[TOTAL_SUIT][iTotalCardType];
    myCard* non_flushes_cards[iTotalCardType];

    //2-A数量
    int every_count[iTotalCardType];

    //百搭数量
    myCard* jokers[2];
    int jokers_count;

    //输出
    int rank;
    int score;
    myCard* final_hand[5];

    void evaluator();//牌局结束估算
    void evaluatorSimple();//牌局中简单根据现有几张牌估算
    bool check_straight(int card_cnt = 5);
    void cal_flush(int suit);
    myCard* get_max_card(int from);
    void trans_card(int idx, int value, int count);
    void get_max_pair(int from, myCard* &card1, myCard* &card2);
    void get_max_three(int from, myCard* &card1, myCard* &card2, myCard* &card3);
};

struct fiveCards
{
    fiveCards();
    void reset();

    //5张牌，输入
    myCard cards[5];

    //中间数据
    //4种花色数量,0没使用
    int flushes[TOTAL_SUIT];
    myCard* cards_flushes[TOTAL_SUIT][iTotalCardType];
    myCard* non_flushes_cards[iTotalCardType];

    //2-A数量
    int every_count[iTotalCardType];

    //百搭数量
    myCard* jokers[2];
    int jokers_count;

    //输出
    int rank;
    int score;

    void evaluator();
    bool check_straight(int card_cnt = 5);
    myCard* get_max_card(int from);
    void get_max_pair(int from, myCard* &card1, myCard* &card2);
    void get_max_three(int from, myCard* &card1, myCard* &card2, myCard* &card3);
};

