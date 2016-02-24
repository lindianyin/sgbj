
#include <string.h>
#include <list>
#include "poker.hpp"
#include <iostream>
#include <algorithm>    // std::random_shuffle
#include <vector>
#include <assert.h>
#include <ctime>
#include "utils_all.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#define INFO(x) //cout<<x

using namespace std;

Database& GetDb();

myCard* get_straight(int suit, myCard* kk[], myCard* jokers[], int jokers_count, myCard* final[], int card_cnt = 5)
{
    //cout<<"jokers count "<<jokers_count<<endl;
    //cout<<jokers[0]<<endl;
    //cout<<jokers[1]<<endl;
    myCard* head = NULL;
    myCard* tail = NULL;
    int total = 0;
    int bd_left = jokers_count;
    int joker_idx = 0;
    if (bd_left)
    {
        if (jokers[0] == NULL)
        {
            joker_idx = 1;
        }
    }
    //牌组暂存
    myCard* k[iTotalCardType + 1];
    for (size_t t = 0; t < iTotalCardType; ++t)
    {
        k[t+1] = kk[t];
    }
    k[0] = k[iTotalCardType];
    //A的顺子特殊处理
    {
        //A-K-Q-J-10
        //A-2-3-4-5
        int i = iTotalCardType;
        while (i >= 0)
        {
            if (head == NULL)//设置龙头
            {
                if (k[i])//有当前牌
                {
                    head = k[i];
                    tail = k[i];
                    total = 1;
                    final[0] = head;
                }
                else if (bd_left > 0)//没有当前牌有joker
                {
                    head = jokers[joker_idx];
                    assert(head);
                    tail = head;
                    head->trans_suit = suit;
                    if (0 == i)
                    {
                        head->trans_value = Ace;
                    }
                    else
                    {
                        head->trans_value = i;
                    }
                    total = 1;
                    final[0] = head;
                    --bd_left;
                    ++joker_idx;
                }
                else
                {
                    if (i == iTotalCardType)
                    {
                        //5当龙头
                        i = card_cnt-1;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                --i;
            }
            else if (k[i])//接龙中，有当前牌
            {
                tail = k[i];
                final[total] = tail;
                ++total;
                if (total >= card_cnt)//顺子了
                {
                    return head;
                }
                --i;
            }
            else if (bd_left > 0)//接龙中，没有当前牌但有joker
            {
                tail = jokers[joker_idx];
                assert(tail);
                tail->trans_suit = suit;
                if (i > 0)
                {
                    tail->trans_value = i;
                }
                else
                {
                    tail->trans_value = Ace;
                }
                final[total] = tail;
                ++total;
                --bd_left;
                ++joker_idx;
                if (total >= card_cnt)//顺子了
                {
                    return head;
                }
                --i;
            }
            else//断了
            {
                //重置
                head = NULL;
                tail = NULL;
                total = 0;
                bd_left = jokers_count;
                joker_idx = 0;
                if (bd_left)
                {
                    if (jokers[0] == NULL)
                    {
                        joker_idx = 1;
                    }
                }
                if (i > card_cnt)
                {
                    //5当龙头
                    i = card_cnt-1;
                    continue;
                }
                else
                {
                    break;
                }
            }
        }
    }
    //从大到小判断顺子K-2
    int i = iTotalCardType-1;
    while (i >= 0)
    {
        //cout<<"i="<<i<<endl;
        //空的
        if (head == NULL)
        {
            //当前有效
            if (k[i])
            {
                head = k[i];
                tail = k[i];
                total = 1;
                final[0] = head;
                //cout<<"new head at "<<i<<endl;
            }
            //有剩余百搭
            else if (bd_left > 0)
            {
                //cout<<"new head at "<<i<<",use jokers "<<joker_idx<<endl;
                head = jokers[joker_idx];
                assert(head);
                tail = head;
                head->trans_suit = suit;
                if (0 == i)
                {
                    head->trans_value = Ace;
                }
                else
                {
                    head->trans_value = i;
                }
                total = 1;
                final[0] = head;
                --bd_left;
                ++joker_idx;
            }
            --i;
        }
        //开始了,当前有效的
        else if (k[i])
        {
            //cout<<"continue at "<<i<<endl;
            tail = k[i];
            final[total] = tail;
            ++total;
            //顺子了
            if (total >= card_cnt)
            {
                return head;
            }
            --i;
        }
        else if (bd_left > 0)
        {
            //cout<<"use jokers "<<joker_idx<<",continue at "<<i<<endl;
            tail = jokers[joker_idx];
            assert(tail);
            tail->trans_suit = suit;
            if (i > 0)
            {
                tail->trans_value = i;
            }
            else
            {
                tail->trans_value = Ace;
            }
            //cout<<"total:"<<total<<endl;
            final[total] = tail;
            ++total;
            --bd_left;
            ++joker_idx;
            //顺子了
            if (total >= card_cnt)
            {
                return head;
            }
            --i;
        }
        else
        {
            //cout<<"break at "<<i<<endl;
            //断了
            i = head->trans_value - 1;
            //重置
            head = NULL;
            tail = NULL;
            total = 0;
            bd_left = jokers_count;
            joker_idx = 0;
            if (bd_left)
            {
                if (jokers[0] == NULL)
                {
                    joker_idx = 1;
                }
            }
            if (i+bd_left < card_cnt-1)
            {
                break;
            }
        }
    }
    return NULL;
}

int get_four(myCard* kk[TOTAL_SUIT][iTotalCardType], int* every_count, myCard* jokers[], int jokers_count, myCard* final[], int card_cnt = 5)
{
    int four = 0;
    int joker_idx = 0;
    int joker_left = jokers_count;

    for (int i = iTotalCardType-1; i >= 0; --i)
    {
        //炸弹了
        if (every_count[i] + jokers_count >= 4)
        {
            if (jokers_count > 0 && jokers[0] == NULL)
            {
                joker_idx = 1;
            }
            joker_left = jokers_count;

            four = i+1;

            //4张炸弹牌
            for (int f = 0; f < 4; ++f)
            {
                if (kk[f][i])
                {
                    final[f] = kk[f][i];
                }
                else
                {
                    final[f] = jokers[joker_idx];
                    final[f]->trans_suit = f+1;
                    final[f]->trans_value = four;
                    ++joker_idx;
                    --joker_left;
                }
            }
            if (card_cnt > 4)
            {
                if (joker_left > 0)
                {
                    int left = four == Ace ? King : Ace;
                    if (every_count[left-1])
                    {
                        for (int t = 0; t < 4; ++t)
                        {
                            if (kk[t][left-1])
                            {
                                final[4] = kk[t][left-1];
                                return four;
                            }
                        }
                        assert(false);
                        return 0;
                    }

                    final[4] = jokers[joker_idx];
                    final[4]->trans_suit = 1;
                    final[4]->trans_value = left;
                    return four;
                }
                else
                {
                    for (int k = Ace; k >= Deuce; --k)
                    {
                        if (k != four && every_count[k-1])
                        {
                            for (int t = 0; t < 4; ++t)
                            {
                                if (kk[t][k-1])
                                {
                                    final[4] = kk[t][k-1];
                                    return four;
                                }
                            }
                        }
                    }
                    assert(false);
                    return 0;
                }
            }
            else
            {
                return four;
            }
        }
    }
    return 0;
}

sevenCards::sevenCards()
{
    reset();
}

//重置手牌
void sevenCards::reset()
{
    memset(flushes, 0, sizeof(int)*(TOTAL_SUIT));
    jokers_count = 0;
    memset(final_hand, NULL, sizeof(myCard*)*5);
    memset(cards_flushes, NULL, sizeof(myCard*)*iTotalCardType*(TOTAL_SUIT));
    memset(every_count, 0, sizeof(int)*iTotalCardType);
    memset(non_flushes_cards, NULL, sizeof(myCard*)*iTotalCardType);
    jokers_count = 0;
    jokers[0] = NULL;
    jokers[1] = NULL;
    rank = 0;
    score = 0;
    for (int i = 0; i < 7; ++i)
    {
        cards[i].pos = i + 1;
        cards[i].suit = 0;
        cards[i].trans_suit = 0;
        cards[i].value = 0;
        cards[i].trans_suit = 0;
    }
}

//清空模拟结算
void sevenCards::resetEva()
{
    memset(flushes, 0, sizeof(int)*(TOTAL_SUIT));
    jokers_count = 0;
    memset(final_hand, NULL, sizeof(myCard*)*5);
    memset(cards_flushes, NULL, sizeof(myCard*)*iTotalCardType*(TOTAL_SUIT));
    memset(every_count, 0, sizeof(int)*iTotalCardType);
    memset(non_flushes_cards, NULL, sizeof(myCard*)*iTotalCardType);
    jokers_count = 0;
    jokers[0] = NULL;
    jokers[1] = NULL;
    rank = 0;
    score = 0;
}

myCard* sevenCards::get_max_card(int from)
{
    //cout<<"get_max_card("<<from<<")"<<endl;
    assert(from <= Ace && from >= Deuce);
    for (int i = from; i >= Deuce; --i)
    {
        if (every_count[i-1] > 0)
        {
            for (int j = 0; j < 4; ++j)
            {
                if (cards_flushes[j][i-1])
                {
                    //cout<<"get_max_card("<<from<<") return "<<j<<","<<i<<endl;
                    return cards_flushes[j][i-1];
                }
            }
        }
    }
    return NULL;
}

void sevenCards::get_max_pair(int from, myCard* &card1, myCard* &card2)
{
    card1 = NULL;
    card2 = NULL;
    if (from <= Ace && from >= Deuce)
    {
        for (int i = from; i >= Deuce; --i)
        {
            if (every_count[i-1] == 2)
            {
                for (int f = 0; f < 4; ++f)
                {
                    if (cards_flushes[f][i-1])
                    {
                        if (card1 == NULL)
                        {
                            card1 = cards_flushes[f][i-1];
                        }
                        else
                        {
                            card2 = cards_flushes[f][i-1];
                            return;
                        }
                    }
                }
                assert(false);
            }
        }
    }
    return;
}

void sevenCards::get_max_three(int from, myCard* &card1, myCard* &card2, myCard* &card3)
{
    card1 = NULL;
    card2 = NULL;
    card3 = NULL;
    if (from <= Ace && from >= Deuce)
    {
        for (int i = from; i >= Deuce; --i)
        {
            if (every_count[i-1] == 3)
            {
                for (int f = 0; f < 4; ++f)
                {
                    if (cards_flushes[f][i-1])
                    {
                        if (card1 == NULL)
                        {
                            card1 = cards_flushes[f][i-1];
                        }
                        else if (card2 == NULL)
                        {
                            card2 = cards_flushes[f][i-1];
                        }
                        else
                        {
                            card3 = cards_flushes[f][i-1];
                            return;
                        }
                    }
                }
                assert(false);
            }
        }
    }
    return;
}

void sevenCards::trans_card(int from_idx, int value, int count)
{
    //cout<<"change card "<<from_idx<<","<<value<<","<<count<<endl;
    assert(from_idx >= 0 && (from_idx+count) <= 4);
    assert(value >= Deuce && value <= Ace);
    assert(count > 0 && count <= jokers_count);

    int jokers_idx = 0, transed = 0;
    if (jokers[0] == NULL)
    {
        jokers_idx = 1;
        assert(jokers[jokers_idx]);
    }
    else
    {
        assert(jokers[jokers_idx]);
    }

    //cout<<"111"<<endl;

    for (int f = 0; f < 4; ++f)
    {
        if (cards_flushes[f][value-1] == NULL)
        {
            final_hand[from_idx] = jokers[jokers_idx];
            final_hand[from_idx]->trans_suit = f+1;
            final_hand[from_idx]->trans_value = value;
            ++from_idx;
            ++transed;

            if (transed >= count)
            {
                return;
            }
            ++jokers_idx;
        }
    }
}

bool sevenCards::check_straight(int card_cnt)
{
    //是否顺子
    myCard* h = get_straight(1, non_flushes_cards, jokers, jokers_count, final_hand, card_cnt);
    if (h)
    {
        switch (h->trans_value)
        {
            case Ace:
                score = Ace + 1;
                break;
            case Five:
                score = Ace;
                break;
            default:
                score = h->trans_value;
                break;
        }
        rank = STRAIGHT;
        return true;
    }
    else
    {
        return false;
    }
}

void sevenCards::cal_flush(int suit)
{
    INFO("is only flushes"<<endl);
    //同花
    rank = FLUSH;
    //计算同花的分
    int bd_left = jokers_count;
    int bd_idx = 0;
    if (jokers_count && jokers[0] == NULL)
    {
        bd_idx = 1;
    }
    int final_idx = 0;
    for (int k = iTotalCardType-1; k >= 0; --k)
    {
        if (cards_flushes[suit-1][k] == 0)
        {
            if (bd_left > 0)
            {
                --bd_left;
                final_hand[final_idx] = jokers[bd_idx];
                final_hand[final_idx]->trans_value = k + 1;
                final_hand[final_idx]->trans_suit = suit;
                score += iScore[k];
                ++bd_idx;
                ++final_idx;
                if (final_idx > 4)
                {
                    return;
                }
            }
        }
        else
        {
            score += iScore[k];
            final_hand[final_idx] = cards_flushes[suit-1][k];
            ++final_idx;
            if (final_idx > 4)
            {
                return;
            }
        }
    }
    return;
}

void sevenCards::evaluator()
{
    cout<<"evaluator()"<<endl;
    if (rank)
    {
        return;
    }
    for (int i = 0; i < 7; ++i)
    {
        debug_print(cards[i]);
    }
    cout << endl;
    for (int i = 0; i < 7; ++i)
    {
        switch (cards[i].value)
        {
            case Black_Joker:
                ++jokers_count;
                jokers[0] = cards + i;
                break;
            case Red_Joker:
                ++jokers_count;
                jokers[1] = cards + i;
                break;
            case 0:
                //cout<<"card "<<(i+1) << " is zero!!!"<<endl;
                break;
            default:
                //cout<<"card "<<(i+1)<<endl;
                assert(cards[i].suit);
                ++flushes[cards[i].suit-1];
                cards_flushes[cards[i].suit-1][cards[i].value-1] = cards + i;
                ++every_count[cards[i].value-1];
        }
    }

    for (int i = 0; i < iTotalCardType; ++i)
    {
        if (cards_flushes[0][i])
        {
            non_flushes_cards[i] = cards_flushes[0][i];
        }
        else if (cards_flushes[1][i])
        {
            non_flushes_cards[i] = cards_flushes[1][i];
        }
        else if (cards_flushes[2][i])
        {
            non_flushes_cards[i] = cards_flushes[2][i];
        }
        else if (cards_flushes[3][i])
        {
            non_flushes_cards[i] = cards_flushes[3][i];
        }
    }

    INFO("start ..."<<endl);
    bool be_flush = false;
    int flush_suit = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (flushes[i] + jokers_count >= 5)
        {
            INFO("is flushes"<<endl);
            //同花了

            INFO("check if is flushes straight"<<endl);
            //判断是否同花顺
            myCard* h = get_straight(i+1, cards_flushes[i], jokers, jokers_count, final_hand);
            if (h)
            {
                INFO("is flushes straight"<<endl);
                //同花顺
                switch (h->trans_value)
                {
                    case Ace:
                        score = Ace + 1;
                        rank = ROYAL_FLUSH;
                        break;
                    case Five:
                        score = Ace;
                        rank = SHELL_FLUSH;
                        break;
                    default:
                        score = h->trans_value;
                        rank = STRAIGHT_FLUSH;
                        break;
                }
                return;
            }
            //没有同花顺先保留同花保底,等后面判断炸弹和满屋
            be_flush = true;
            flush_suit = i+1;
        }
    }

    INFO("check if is four in a kind"<<endl);
    //是否炸弹
    int f = get_four(cards_flushes, every_count, jokers, jokers_count, final_hand);
    if (f > 0)
    {
        score = f*100 + final_hand[4]->trans_value;
        rank = FOUR_OF_A_KIND;
        return;
    }

    switch (jokers_count)
    {
        case 2:
        {
            INFO("2 jokers"<<endl);
            //前面判断过炸弹，如果有大小王，还没有炸弹，那肯定都是单个,不可能是葫芦了

            //是否同花
            if (be_flush)
            {
                cal_flush(flush_suit);
                return;
            }
            //是否顺子
            if (check_straight())
            {
                return;
            }
            //只能是三个了
            rank = THREE_OF_A_KIND;
            memset(final_hand, NULL, sizeof(myCard*)*5);

            final_hand[0] = get_max_card(Ace);
            final_hand[3] = get_max_card(final_hand[0]->trans_value-1);
            final_hand[4] = get_max_card(final_hand[3]->trans_value-1);
            trans_card(1, final_hand[0]->trans_value, 2);
            assert(final_hand[1]);
            assert(final_hand[2]);
            score = final_hand[0]->trans_value * 400 + final_hand[3]->trans_value*20 + final_hand[4]->trans_value;
            return;
        }
        case 1:
        {
            INFO("1 jokers"<<endl);
            myCard *max_pair1 = NULL, *max_pair2 = NULL;

            get_max_pair(Ace, max_pair1, max_pair2);
            //找到一对
            if (max_pair1 && max_pair2)
            {
                INFO("find first pair"<<endl);
                myCard *second_pair1 = NULL, *second_pair2 = NULL;
                get_max_pair(max_pair1->trans_value-1, second_pair1, second_pair2);
                //找到第二对，就是葫芦了
                if (second_pair1 && second_pair2)
                {
                    INFO("find second pair"<<endl);
                    rank = FULL_HOUSE;
                    final_hand[0] = max_pair1;
                    final_hand[1] = max_pair2;
                    trans_card(2, final_hand[0]->trans_value, 1);
                    final_hand[3] = second_pair1;
                    final_hand[4] = second_pair2;

                    score = final_hand[0]->trans_value * 20 + final_hand[3]->trans_value;
                    return;
                }
                else if (be_flush)
                {
                    //是否同花
                    cal_flush(flush_suit);
                    return;
                }
                else
                {
                    //是否顺子
                    if (check_straight())
                    {
                        return;
                    }
                    INFO("only three in a kind"<<endl);
                    //三个
                    rank = THREE_OF_A_KIND;
                    final_hand[0] = max_pair1;
                    final_hand[1] = max_pair2;
                    trans_card(2, final_hand[0]->trans_value, 1);
                    final_hand[3] = get_max_card(Ace);
                    if (final_hand[3]->trans_value == max_pair1->trans_value)
                    {
                        final_hand[3] = get_max_card(max_pair1->trans_value-1);
                    }
                    final_hand[4] = get_max_card(final_hand[3]->trans_value-1);
                    if (final_hand[4]->trans_value == max_pair1->trans_value)
                    {
                        final_hand[4] = get_max_card(max_pair1->trans_value-1);
                    }
                    score = final_hand[0]->trans_value * 400 + final_hand[3]->trans_value*20 + final_hand[4]->trans_value;
                }
                return;
            }
            else
            {
                //是否同花
                if (be_flush)
                {
                    cal_flush(flush_suit);
                    return;
                }
                INFO("1 jokers, no pair, check straight"<<endl);
                //是否顺子
                if (check_straight())
                {
                    return;
                }

                INFO("one pair!"<<endl);
                //一对
                rank = ONE_PAIR;
                final_hand[0] = get_max_card(Ace);
                trans_card(1, final_hand[0]->trans_value, 1);
                final_hand[2] = get_max_card(final_hand[0]->trans_value-1);
                final_hand[3] = get_max_card(final_hand[2]->trans_value-1);
                final_hand[4] = get_max_card(final_hand[3]->trans_value-1);
                score = final_hand[0]->trans_value * 8000 + final_hand[2]->trans_value*400 + final_hand[3]->trans_value*20 + final_hand[4]->trans_value;
                return;
            }
        }
        default:
        {
            INFO("0 jokers"<<endl);
            myCard *max_three1 = NULL, *max_three2 = NULL, *max_three3 = NULL;
            get_max_three(Ace, max_three1, max_three2, max_three3);
            //有三个
            if (max_three1 && max_three2 && max_three3)
            {
                INFO("find a tree in a kind"<<endl);
                //还有三个,葫芦了
                myCard *second_three1 = NULL, *second_three2 = NULL, *second_three3 = NULL;
                get_max_three(max_three1->trans_value-1, second_three1, second_three2, second_three3);
                if (second_three1 && second_three2 && second_three3)
                {
                    INFO("full house"<<endl);
                    rank = FULL_HOUSE;
                    final_hand[0] = max_three1;
                    final_hand[1] = max_three2;
                    final_hand[2] = max_three3;
                    final_hand[3] = second_three1;
                    final_hand[4] = second_three2;
                    score = final_hand[0]->trans_value * 20 + final_hand[3]->trans_value;
                    return;
                }
                myCard *max_pair1 = NULL, *max_pair2 = NULL;
                get_max_pair(Ace, max_pair1, max_pair2);
                //有一对,葫芦了
                if (max_pair1 && max_pair2)
                {
                    INFO("full house"<<endl);
                    rank = FULL_HOUSE;
                    final_hand[0] = max_three1;
                    final_hand[1] = max_three2;
                    final_hand[2] = max_three3;
                    final_hand[3] = max_pair1;
                    final_hand[4] = max_pair2;
                    score = final_hand[0]->trans_value * 20 + final_hand[3]->trans_value;
                    return;
                }
                else if (be_flush)
                {
                    //是否同花
                    cal_flush(flush_suit);
                    return;
                }
                else
                {
                    INFO("check straight"<<endl);
                    //是否顺子
                    if (check_straight())
                    {
                        return;
                    }

                    INFO("three in a kind"<<endl);
                    //三个
                    rank = THREE_OF_A_KIND;
                    final_hand[0] = max_three1;
                    final_hand[1] = max_three2;
                    final_hand[2] = max_three3;
                    final_hand[3] = get_max_card(Ace);
                    if (final_hand[3]->trans_value == max_three1->trans_value)
                    {
                        final_hand[3] = get_max_card(max_three1->trans_value-1);
                    }
                    final_hand[4] = get_max_card(final_hand[3]->trans_value-1);
                    if (final_hand[4]->trans_value == max_three1->trans_value)
                    {
                        final_hand[4] = get_max_card(max_three1->trans_value-1);
                    }
                    score = final_hand[0]->trans_value * 400 + final_hand[3]->trans_value*20 + final_hand[4]->trans_value;
                    return;
                }
            }
            else
            {
                INFO("no tree in a kind"<<endl);

                //是否同花
                if (be_flush)
                {
                    cal_flush(flush_suit);
                    return;
                }

                INFO("check straight"<<endl);
                //是否顺子
                if (check_straight())
                {
                    return;
                }
                INFO("find max pair"<<endl);
                myCard *max_pair1 = NULL, *max_pair2 = NULL;
                get_max_pair(Ace, max_pair1, max_pair2);
                //有一对了
                if (max_pair1 && max_pair2)
                {
                    INFO("find a pair"<<endl);
                    myCard *second_pair1 = NULL, *second_pair2 = NULL;
                    get_max_pair(max_pair1->trans_value-1, second_pair1, second_pair2);
                    //找到第二对，就是两对了
                    if (second_pair1 && second_pair2)
                    {
                        INFO("find two pair"<<endl);
                        rank = TWO_PAIR;
                        final_hand[0] = max_pair1;
                        final_hand[1] = max_pair2;
                        final_hand[2] = second_pair1;
                        final_hand[3] = second_pair2;
                        final_hand[4] = get_max_card(Ace);
                        if (final_hand[4]->trans_value == max_pair1->trans_value)
                        {
                            final_hand[4] = get_max_card(max_pair1->trans_value-1);
                        }
                        if (final_hand[4]->trans_value == second_pair1->trans_value)
                        {
                            final_hand[4] = get_max_card(second_pair1->trans_value-1);
                        }
                        score = final_hand[0]->trans_value * 400 + final_hand[2]->trans_value*20 + final_hand[4]->trans_value;
                        return;
                    }
                    else
                    {
                        INFO("only one pair ->"<<(int)max_pair1->trans_value<<endl);
                        //一对
                        rank = ONE_PAIR;
                        final_hand[0] = max_pair1;
                        final_hand[1] = max_pair2;
                        final_hand[2] = get_max_card(Ace);
                        if (final_hand[2]->trans_value == max_pair1->trans_value)
                        {
                            final_hand[2] = get_max_card(max_pair1->trans_value-1);
                        }
                        final_hand[3] = get_max_card(final_hand[2]->trans_value-1);
                        if (final_hand[3]->trans_value == max_pair1->trans_value)
                        {
                            final_hand[3] = get_max_card(max_pair1->trans_value-1);
                        }
                        final_hand[4] = get_max_card(final_hand[3]->trans_value-1);
                        if (final_hand[4]->trans_value == max_pair1->trans_value)
                        {
                            final_hand[4] = get_max_card(max_pair1->trans_value-1);
                        }
                        score = final_hand[0]->trans_value * 8000 + final_hand[2]->trans_value*400 + 20*final_hand[3]->trans_value + final_hand[4]->trans_value;
                        return;
                    }
                }
                else
                {
                    INFO("high card"<<endl);
                    //高牌
                    rank = HIGH_CARD;
                    final_hand[0] = get_max_card(Ace);
                    final_hand[1] = get_max_card(final_hand[0]->trans_value-1);
                    final_hand[2] = get_max_card(final_hand[1]->trans_value-1);
                    final_hand[3] = get_max_card(final_hand[2]->trans_value-1);
                    final_hand[4] = get_max_card(final_hand[3]->trans_value-1);
                    score = final_hand[4]->trans_value + final_hand[3]->trans_value*20 + final_hand[2]->trans_value*400 + final_hand[1]->trans_value*8000 + final_hand[0]->trans_value*16000;
                    return;
                }
            }
        }
    }
    //高牌
    rank = HIGH_CARD;
    score = 0;
    return;
}

void sevenCards::evaluatorSimple()
{
    cout<<"evaluatorSimple()"<<endl;
    if (rank)
    {
        return;
    }
    for (int i = 0; i < 7; ++i)
    {
        debug_print(cards[i]);
    }
    cout << endl;
    int card_cnt = 0;
    for (int i = 0; i < 7; ++i)
    {
        switch (cards[i].value)
        {
            case Black_Joker:
                ++jokers_count;
                jokers[0] = cards + i;
                ++card_cnt;
                break;
            case Red_Joker:
                ++jokers_count;
                jokers[1] = cards + i;
                ++card_cnt;
                break;
            case 0:
                //cout<<"card "<<(i+1) << " is zero!!!"<<endl;
                break;
            default:
                //cout<<"card "<<(i+1)<<endl;
                assert(cards[i].suit);
                ++flushes[cards[i].suit-1];
                cards_flushes[cards[i].suit-1][cards[i].value-1] = cards + i;
                ++every_count[cards[i].value-1];
                ++card_cnt;
                break;
        }
    }

    for (int i = 0; i < iTotalCardType; ++i)
    {
        if (cards_flushes[0][i])
        {
            non_flushes_cards[i] = cards_flushes[0][i];
        }
        else if (cards_flushes[1][i])
        {
            non_flushes_cards[i] = cards_flushes[1][i];
        }
        else if (cards_flushes[2][i])
        {
            non_flushes_cards[i] = cards_flushes[2][i];
        }
        else if (cards_flushes[3][i])
        {
            non_flushes_cards[i] = cards_flushes[3][i];
        }
    }

    INFO("start ... cnt=" << card_cnt <<endl);
    if (card_cnt > 5)
        card_cnt = 5;
    //小于5张不成牌只判断数量
    if (card_cnt < 5)
    {
        for (int i = iTotalCardType-1; i >= 0; --i)
        {
            //炸弹了
            if (every_count[i] + jokers_count >= 4)
            {
                rank = FOUR_OF_A_KIND;
                return;
            }
            else if (every_count[i] + jokers_count >= 3)
            {
                rank = THREE_OF_A_KIND;
                return;
            }
        }
        int use_joker_cnt = 0;
        int pair_cnt = 0;
        for (int i = iTotalCardType-1; i >= 0; --i)
        {
            int joker_left = jokers_count - use_joker_cnt;
            if (every_count[i] + joker_left >= 2)
            {
                use_joker_cnt += (2-every_count[i]);
                ++pair_cnt;
            }
        }
        switch (pair_cnt)
        {
            case 2:
            {
                rank = TWO_PAIR;
                return;
            }
            case 1:
            {
                rank = ONE_PAIR;
                return;
            }
            default:
            {
                rank = HIGH_CARD;
                return;
            }
        }
        //高牌
        rank = HIGH_CARD;
        return;
    }
    bool be_flush = false;
    for (int i = 0; i < 4; ++i)
    {
        if (flushes[i] + jokers_count >= card_cnt)
        {
            //同花了
            INFO("is flushes"<<endl);

            INFO("check if is flushes straight"<<endl);
            //判断是否同花顺
            myCard* h = get_straight(i+1, cards_flushes[i], jokers, jokers_count, final_hand, card_cnt);
            if (h)
            {
                INFO("is flushes straight"<<endl);
                //同花顺
                switch (h->trans_value)
                {
                    case Ace:
                        rank = ROYAL_FLUSH;
                        break;
                    case Five:
                        rank = SHELL_FLUSH;
                        break;
                    default:
                        rank = STRAIGHT_FLUSH;
                        break;
                }
                return;
            }
            //没有同花顺先保留同花保底,等后面判断炸弹和满屋
            be_flush = true;
        }
    }

    INFO("check if is four in a kind"<<endl);
    //是否炸弹
    int f = get_four(cards_flushes, every_count, jokers, jokers_count, final_hand, card_cnt);
    if (f > 0)
    {
        rank = FOUR_OF_A_KIND;
        return;
    }

    switch (jokers_count)
    {
        case 2:
        {
            INFO("2 jokers"<<endl);
            //前面判断过炸弹，如果有大小王，还没有炸弹，那肯定都是单个,不可能是葫芦了

            //是否同花
            if (be_flush)
            {
                rank = FLUSH;
                return;
            }
            //是否顺子
            if (check_straight(card_cnt))
            {
                return;
            }
            //只能是三个了
            rank = THREE_OF_A_KIND;
            return;
        }
        case 1:
        {
            INFO("1 jokers"<<endl);
            myCard *max_pair1 = NULL, *max_pair2 = NULL;

            get_max_pair(Ace, max_pair1, max_pair2);
            //找到一对
            if (max_pair1 && max_pair2)
            {
                INFO("find first pair"<<endl);
                myCard *second_pair1 = NULL, *second_pair2 = NULL;
                get_max_pair(max_pair1->trans_value-1, second_pair1, second_pair2);
                //找到第二对，就是葫芦了
                if (second_pair1 && second_pair2)
                {
                    INFO("find second pair"<<endl);
                    rank = FULL_HOUSE;
                    return;
                }
                else if (be_flush)
                {
                    //是否同花
                    rank = FLUSH;
                    return;
                }
                else
                {
                    //是否顺子
                    if (check_straight(card_cnt))
                    {
                        return;
                    }
                    INFO("only three in a kind"<<endl);
                    //三个
                    rank = THREE_OF_A_KIND;
                }
                return;
            }
            else
            {
                //是否同花
                if (be_flush)
                {
                    rank = FLUSH;
                    return;
                }
                INFO("1 jokers, no pair, check straight"<<endl);
                //是否顺子
                if (check_straight(card_cnt))
                {
                    return;
                }

                INFO("one pair!"<<endl);
                //一对
                rank = ONE_PAIR;
                return;
            }
        }
        default:
        {
            INFO("0 jokers"<<endl);
            myCard *max_three1 = NULL, *max_three2 = NULL, *max_three3 = NULL;
            get_max_three(Ace, max_three1, max_three2, max_three3);
            //有三个
            if (max_three1 && max_three2 && max_three3)
            {
                INFO("find a tree in a kind"<<endl);
                //还有三个,葫芦了
                myCard *second_three1 = NULL, *second_three2 = NULL, *second_three3 = NULL;
                get_max_three(max_three1->trans_value-1, second_three1, second_three2, second_three3);
                if (second_three1 && second_three2 && second_three3)
                {
                    INFO("full house"<<endl);
                    rank = FULL_HOUSE;
                    return;
                }
                myCard *max_pair1 = NULL, *max_pair2 = NULL;
                get_max_pair(Ace, max_pair1, max_pair2);
                //有一对,葫芦了
                if (max_pair1 && max_pair2)
                {
                    INFO("full house"<<endl);
                    rank = FULL_HOUSE;
                    return;
                }
                else if (be_flush)
                {
                    //是否同花
                    rank = FLUSH;
                    return;
                }
                else
                {
                    INFO("check straight"<<endl);
                    //是否顺子
                    if (check_straight(card_cnt))
                    {
                        return;
                    }

                    INFO("three in a kind"<<endl);
                    //三个
                    rank = THREE_OF_A_KIND;
                    return;
                }
            }
            else
            {
                INFO("no tree in a kind"<<endl);

                //是否同花
                if (be_flush)
                {
                    rank = FLUSH;
                    return;
                }

                INFO("check straight"<<endl);
                //是否顺子
                if (check_straight(card_cnt))
                {
                    return;
                }
                INFO("find max pair"<<endl);
                myCard *max_pair1 = NULL, *max_pair2 = NULL;
                get_max_pair(Ace, max_pair1, max_pair2);
                //有一对了
                if (max_pair1 && max_pair2)
                {
                    INFO("find a pair"<<endl);
                    myCard *second_pair1 = NULL, *second_pair2 = NULL;
                    get_max_pair(max_pair1->trans_value-1, second_pair1, second_pair2);
                    //找到第二对，就是两对了
                    if (second_pair1 && second_pair2)
                    {
                        INFO("find two pair"<<endl);
                        rank = TWO_PAIR;
                        return;
                    }
                    else
                    {
                        INFO("only one pair ->"<<(int)max_pair1->trans_value<<endl);
                        //一对
                        rank = ONE_PAIR;
                        return;
                    }
                }
                else
                {
                    INFO("high card"<<endl);
                    //高牌
                    rank = HIGH_CARD;
                    return;
                }
            }
        }
    }
    //高牌
    rank = HIGH_CARD;
    return;
}

fiveCards::fiveCards()
{
    reset();
}

//重置手牌
void fiveCards::reset()
{
    memset(flushes, 0, sizeof(int)*(TOTAL_SUIT));
    jokers_count = 0;
    memset(cards_flushes, NULL, sizeof(myCard*)*iTotalCardType*(TOTAL_SUIT));
    memset(every_count, 0, sizeof(int)*iTotalCardType);
    memset(non_flushes_cards, NULL, sizeof(myCard*)*iTotalCardType);
    jokers_count = 0;
    jokers[0] = NULL;
    jokers[1] = NULL;
    rank = 0;
    score = 0;
    for (int i = 0; i < 5; ++i)
    {
        cards[i].pos = i + 1;
        cards[i].suit = 0;
        cards[i].trans_suit = 0;
        cards[i].value = 0;
        cards[i].trans_suit = 0;
    }
}

myCard* fiveCards::get_max_card(int from)
{
    //cout<<"get_max_card("<<from<<")"<<endl;
    assert(from <= Ace && from >= Deuce);
    for (int i = from; i >= Deuce; --i)
    {
        if (every_count[i-1] > 0)
        {
            for (int j = 0; j < 4; ++j)
            {
                if (cards_flushes[j][i-1])
                {
                    //cout<<"get_max_card("<<from<<") return "<<j<<","<<i<<endl;
                    return cards_flushes[j][i-1];
                }
            }
        }
    }
    return NULL;
}

void fiveCards::get_max_pair(int from, myCard* &card1, myCard* &card2)
{
    card1 = NULL;
    card2 = NULL;
    if (from <= Ace && from >= Deuce)
    {
        for (int i = from; i >= Deuce; --i)
        {
            if (every_count[i-1] == 2)
            {
                for (int f = 0; f < 4; ++f)
                {
                    if (cards_flushes[f][i-1])
                    {
                        if (card1 == NULL)
                        {
                            card1 = cards_flushes[f][i-1];
                        }
                        else
                        {
                            card2 = cards_flushes[f][i-1];
                            return;
                        }
                    }
                }
                assert(false);
            }
        }
    }
    return;
}

void fiveCards::get_max_three(int from, myCard* &card1, myCard* &card2, myCard* &card3)
{
    card1 = NULL;
    card2 = NULL;
    card3 = NULL;
    if (from <= Ace && from >= Deuce)
    {
        for (int i = from; i >= Deuce; --i)
        {
            if (every_count[i-1] == 3)
            {
                for (int f = 0; f < 4; ++f)
                {
                    if (cards_flushes[f][i-1])
                    {
                        if (card1 == NULL)
                        {
                            card1 = cards_flushes[f][i-1];
                        }
                        else if (card2 == NULL)
                        {
                            card2 = cards_flushes[f][i-1];
                        }
                        else
                        {
                            card3 = cards_flushes[f][i-1];
                            return;
                        }
                    }
                }
                assert(false);
            }
        }
    }
    return;
}

bool fiveCards::check_straight(int card_cnt)
{
    //是否顺子
    myCard* final_hand[5];
    myCard* h = get_straight(1, non_flushes_cards, jokers, jokers_count, final_hand, card_cnt);
    if (h)
    {
        switch (h->trans_value)
        {
            case Ace:
                score = Ace + 1;
                break;
            case Five:
                score = Ace;
                break;
            default:
                score = h->trans_value;
                break;
        }
        rank = STRAIGHT;
        return true;
    }
    else
    {
        return false;
    }
}

void fiveCards::evaluator()
{
    cout<<"evaluator()"<<endl;
    myCard* final_hand[5];
    if (rank)
    {
        return;
    }
    for (int i = 0; i < 5; ++i)
    {
        debug_print(cards[i]);
    }
    cout << endl;
    int card_cnt = 0;
    for (int i = 0; i < 5; ++i)
    {
        switch (cards[i].value)
        {
            case Black_Joker:
                ++jokers_count;
                jokers[0] = cards + i;
                ++card_cnt;
                break;
            case Red_Joker:
                ++jokers_count;
                jokers[1] = cards + i;
                ++card_cnt;
                break;
            default:
                //cout<<"card "<<(i+1)<<endl;
                assert(cards[i].suit);
                ++flushes[cards[i].suit-1];
                cards_flushes[cards[i].suit-1][cards[i].value-1] = cards + i;
                ++every_count[cards[i].value-1];
                ++card_cnt;
                break;
        }
    }

    for (int i = 0; i < iTotalCardType; ++i)
    {
        if (cards_flushes[0][i])
        {
            non_flushes_cards[i] = cards_flushes[0][i];
        }
        else if (cards_flushes[1][i])
        {
            non_flushes_cards[i] = cards_flushes[1][i];
        }
        else if (cards_flushes[2][i])
        {
            non_flushes_cards[i] = cards_flushes[2][i];
        }
        else if (cards_flushes[3][i])
        {
            non_flushes_cards[i] = cards_flushes[3][i];
        }
    }

    INFO("start ... cnt=" << card_cnt <<endl);
    if (card_cnt > 5)
        card_cnt = 5;
    //小于5张不成牌只判断数量
    if (card_cnt < 5)
    {
        return;
    }
    bool be_flush = false;
    for (int i = 0; i < 4; ++i)
    {
        if (flushes[i] + jokers_count >= card_cnt)
        {
            //同花了
            INFO("is flushes"<<endl);

            INFO("check if is flushes straight"<<endl);
            //判断是否同花顺
            myCard* h = get_straight(i+1, cards_flushes[i], jokers, jokers_count, final_hand, card_cnt);
            if (h)
            {
                INFO("is flushes straight"<<endl);
                //同花顺
                switch (h->trans_value)
                {
                    case Ace:
                        rank = ROYAL_FLUSH;
                        break;
                    case Five:
                        rank = SHELL_FLUSH;
                        break;
                    default:
                        rank = STRAIGHT_FLUSH;
                        break;
                }
                return;
            }
            //没有同花顺先保留同花保底,等后面判断炸弹和满屋
            be_flush = true;
        }
    }

    INFO("check if is four in a kind"<<endl);
    //是否炸弹
    int f = get_four(cards_flushes, every_count, jokers, jokers_count, final_hand, card_cnt);
    if (f > 0)
    {
        rank = FOUR_OF_A_KIND;
        return;
    }

    switch (jokers_count)
    {
        case 2:
        {
            INFO("2 jokers"<<endl);
            //前面判断过炸弹，如果有大小王，还没有炸弹，那肯定都是单个,不可能是葫芦了

            //是否同花
            if (be_flush)
            {
                rank = FLUSH;
                return;
            }
            //是否顺子
            if (check_straight(card_cnt))
            {
                return;
            }
            //只能是三个了
            rank = THREE_OF_A_KIND;
            return;
        }
        case 1:
        {
            INFO("1 jokers"<<endl);
            myCard *max_pair1 = NULL, *max_pair2 = NULL;

            get_max_pair(Ace, max_pair1, max_pair2);
            //找到一对
            if (max_pair1 && max_pair2)
            {
                INFO("find first pair"<<endl);
                myCard *second_pair1 = NULL, *second_pair2 = NULL;
                get_max_pair(max_pair1->trans_value-1, second_pair1, second_pair2);
                //找到第二对，就是葫芦了
                if (second_pair1 && second_pair2)
                {
                    INFO("find second pair"<<endl);
                    rank = FULL_HOUSE;
                    return;
                }
                else if (be_flush)
                {
                    //是否同花
                    rank = FLUSH;
                    return;
                }
                else
                {
                    //是否顺子
                    if (check_straight(card_cnt))
                    {
                        return;
                    }
                    INFO("only three in a kind"<<endl);
                    //三个
                    rank = THREE_OF_A_KIND;
                }
                return;
            }
            else
            {
                //是否同花
                if (be_flush)
                {
                    rank = FLUSH;
                    return;
                }
                INFO("1 jokers, no pair, check straight"<<endl);
                //是否顺子
                if (check_straight(card_cnt))
                {
                    return;
                }

                INFO("one pair!"<<endl);
                //一对
                rank = ONE_PAIR;
                return;
            }
        }
        default:
        {
            INFO("0 jokers"<<endl);
            myCard *max_three1 = NULL, *max_three2 = NULL, *max_three3 = NULL;
            get_max_three(Ace, max_three1, max_three2, max_three3);
            //有三个
            if (max_three1 && max_three2 && max_three3)
            {
                INFO("find a tree in a kind"<<endl);
                //还有三个,葫芦了
                myCard *second_three1 = NULL, *second_three2 = NULL, *second_three3 = NULL;
                get_max_three(max_three1->trans_value-1, second_three1, second_three2, second_three3);
                if (second_three1 && second_three2 && second_three3)
                {
                    INFO("full house"<<endl);
                    rank = FULL_HOUSE;
                    return;
                }
                myCard *max_pair1 = NULL, *max_pair2 = NULL;
                get_max_pair(Ace, max_pair1, max_pair2);
                //有一对,葫芦了
                if (max_pair1 && max_pair2)
                {
                    INFO("full house"<<endl);
                    rank = FULL_HOUSE;
                    return;
                }
                else if (be_flush)
                {
                    //是否同花
                    rank = FLUSH;
                    return;
                }
                else
                {
                    INFO("check straight"<<endl);
                    //是否顺子
                    if (check_straight(card_cnt))
                    {
                        return;
                    }

                    INFO("three in a kind"<<endl);
                    //三个
                    rank = THREE_OF_A_KIND;
                    return;
                }
            }
            else
            {
                INFO("no tree in a kind"<<endl);

                //是否同花
                if (be_flush)
                {
                    rank = FLUSH;
                    return;
                }

                INFO("check straight"<<endl);
                //是否顺子
                if (check_straight(card_cnt))
                {
                    return;
                }
                INFO("find max pair"<<endl);
                myCard *max_pair1 = NULL, *max_pair2 = NULL;
                get_max_pair(Ace, max_pair1, max_pair2);
                //有一对了
                if (max_pair1 && max_pair2)
                {
                    INFO("find a pair"<<endl);
                    myCard *second_pair1 = NULL, *second_pair2 = NULL;
                    get_max_pair(max_pair1->trans_value-1, second_pair1, second_pair2);
                    //找到第二对，就是两对了
                    if (second_pair1 && second_pair2)
                    {
                        INFO("find two pair"<<endl);
                        rank = TWO_PAIR;
                        return;
                    }
                    else
                    {
                        INFO("only one pair ->"<<(int)max_pair1->trans_value<<endl);
                        //一对
                        rank = ONE_PAIR;
                        return;
                    }
                }
                else
                {
                    INFO("high card"<<endl);
                    //高牌
                    rank = HIGH_CARD;
                    return;
                }
            }
        }
    }
    //高牌
    rank = HIGH_CARD;
    return;
}

void debug_print(const baseCard& c)
{
    switch (c.value)
    {
        case Ace:
            cout<<"A";
            break;
        case King:
            cout<<"K";
            break;
        case Queen:
            cout<<"Q";
            break;
        case Jack:
            cout<<"J";
            break;
        case Ten:
            cout<<"10";
            break;
        case Black_Joker:
            cout<<"Black Joker";
            break;
        case Red_Joker:
            cout<<"Red Joker";
            break;
        case Nine:
            cout<<"9";
            break;
        case Eight:
            cout<<"8";
            break;
        case Seven:
            cout<<"7";
            break;
        case Six:
            cout<<"6";
            break;
        case Five:
            cout<<"5";
            break;
        case Four:
            cout<<"4";
            break;
        case Trey:
            cout<<"3";
            break;
        case Deuce:
            cout<<"2";
            break;
        default:
            cout<<(char)('1' + c.value-Deuce);
    }
    cout<<"(";
    switch (c.suit)
    {
        case SPADE:
        {
            cout<<"#";
            break;
        }
        case DIAMOND:
        {
            cout<<"<>";
            break;
        }
        case HEART:
        {
            cout<<"V";
            break;
        }
        case CLUB:
        {
            cout<<"*";
            break;
        }
        default:
            cout<<"";
            break;
    }
    cout<<"),";
}

void debug_print(const myCard& c)
{
    switch (c.value)
    {
        case Ace:
            cout<<"A";
            break;
        case King:
            cout<<"K";
            break;
        case Queen:
            cout<<"Q";
            break;
        case Jack:
            cout<<"J";
            break;
        case Ten:
            cout<<"10";
            break;
        case Black_Joker:
            cout<<"Black Joker";
            break;
        case Red_Joker:
            cout<<"Red Joker";
            break;
        case Nine:
            cout<<"9";
            break;
        case Eight:
            cout<<"8";
            break;
        case Seven:
            cout<<"7";
            break;
        case Six:
            cout<<"6";
            break;
        case Five:
            cout<<"5";
            break;
        case Four:
            cout<<"4";
            break;
        case Trey:
            cout<<"3";
            break;
        case Deuce:
            cout<<"2";
            break;
        default:
            cout<<(char)('1' + c.value-Deuce);
    }
    cout<<"(";
    switch (c.suit)
    {
        case SPADE:
        {
            cout<<"#";
            break;
        }
        case DIAMOND:
        {
            cout<<"<>";
            break;
        }
        case HEART:
        {
            cout<<"V";
            break;
        }
        case CLUB:
        {
            cout<<"*";
            break;
        }
        default:
            cout<<"";
            break;
    }
    cout<<"),";
}

void debug_print_input(const sevenCards& cards)
{
    cout<<"input cards:";
    for (int i = 0; i < 7; ++i)
    {
        debug_print(cards.cards[i]);
    }
    cout<<endl;
}

void debug_print_result(const sevenCards& cards)
{
    cout<<"best hand: ";
    switch (cards.rank)
    {
        case ROYAL_FLUSH:
            cout<<"Royal flush";
            break;
        case SHELL_FLUSH:
            cout<<"Shell flush";
            break;
        case STRAIGHT_FLUSH:
            cout<<"Straight flush";
            break;
        case FOUR_OF_A_KIND:
            cout<<"Four of a kind";
            break;
        case FULL_HOUSE:
            cout<<"Full house";
            break;
        case FLUSH:
            cout<<"Flush";
            break;
        case STRAIGHT:
            cout<<"Straight";
            break;
        case THREE_OF_A_KIND:
            cout<<"Three of a kind";
            break;
        case TWO_PAIR:
            cout<<"Two pair";
            break;
        case ONE_PAIR:
            cout<<"One pair";
            break;
        case HIGH_CARD:
        default:
            cout<<"High card";
            break;
    }
    cout<<",score->"<<cards.score<<endl;
    for (int i = 0; i < 5; ++i)
    {
        assert(cards.final_hand[i]);
        debug_print(*(cards.final_hand[i]));
    }
    cout<<endl<<flush;
}

void random_init(sevenCards& cards)
{
    cout<<"random_init()..."<<endl;
    std::vector<baseCard> decks;
    for (int f = 1; f <= 4; ++f)
    {
        for (int v = Deuce; v <= Ace; ++v)
        {
            baseCard c;
            c.suit = f;
            c.value = v;
            decks.push_back(c);
        }
    }
    baseCard joker1;
    joker1.suit = 0;
    joker1.value = Black_Joker;
    baseCard joker2;
    joker2.suit = 0;
    joker2.value = Red_Joker;
    decks.push_back(joker1);
    decks.push_back(joker2);
    std::random_shuffle (decks.begin(), decks.end(), myrandom);

    std::vector<baseCard>::iterator it = decks.begin();
    for (int i = 0; i < 7; ++i)
    {
        cards.cards[i].pos = i+1;
        cards.cards[i].suit = it->suit;
        cards.cards[i].value = it->value;
        cards.cards[i].trans_suit = it->suit;
        cards.cards[i].trans_value = it->value;
        ++it;
    }
}

void test_DB_card()
{
    Query q(GetDb());
    q.get_result("select suit1,value1,suit2,value2,suit3,value3,suit4,value4,suit5,value5,suit6,value6,suit7,value7 from test_card where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int suit1 = 0, suit2 = 0, suit3 = 0, suit4 = 0, suit5 = 0, suit6 = 0, suit7 = 0;
        int value1 = 0, value2 = 0, value3 = 0, value4 = 0, value5 = 0, value6 = 0, value7 = 0;
        suit1 = q.getval();
        value1 = q.getval();
        suit2 = q.getval();
        value2 = q.getval();
        suit3 = q.getval();
        value3 = q.getval();
        suit4 = q.getval();
        value4 = q.getval();
        suit5 = q.getval();
        value5 = q.getval();
        suit6 = q.getval();
        value6 = q.getval();
        suit7 = q.getval();
        value7 = q.getval();
        sevenCards myCards;
        myCards.cards[0].set(suit1, value1);
        myCards.cards[1].set(suit2, value2);
        myCards.cards[2].set(suit3, value3);
        myCards.cards[3].set(suit4, value4);
        myCards.cards[4].set(suit5, value5);
        myCards.cards[5].set(suit6, value6);
        myCards.cards[6].set(suit7, value7);

        debug_print_input(myCards);
        myCards.evaluator();
        debug_print_result(myCards);
    }
    q.free_result();
}

void test_sevenCards()
{
    std::vector<baseCard> decks;
    for (int f = 1; f <= 4; ++f)
    {
        for (int v = Deuce; v <= Ace; ++v)
        {
            baseCard c;
            c.suit = f;
            c.value = v;
            decks.push_back(c);
        }
    }
    baseCard joker1;
    joker1.suit = 0;
    joker1.value = Black_Joker;
    baseCard joker2;
    joker2.suit = 0;
    joker2.value = Red_Joker;
    decks.push_back(joker1);
    decks.push_back(joker2);
    std::random_shuffle (decks.begin(), decks.end(), myrandom);

    int total = 0;
    for (int a = 0; a < 48; ++a)
    {
        for (int b = a+1; b < 49; ++b)
        {
            for (int c = b+1; c < 50; ++c)
            {
                for (int d = c+1; d < 51; ++d)
                {
                    for (int e = d+1; e < 52; ++e)
                    {
                        for (int f = e+1; f < 53; ++f)
                        {
                            for (int g = f+1; g < 54; ++g)
                            {
                                sevenCards myCards;
                                myCards.cards[0].set(decks[a].suit, decks[a].value);
                                myCards.cards[1].set(decks[b].suit, decks[b].value);
                                myCards.cards[2].set(decks[c].suit, decks[c].value);
                                myCards.cards[3].set(decks[d].suit, decks[d].value);
                                myCards.cards[4].set(decks[e].suit, decks[e].value);
                                myCards.cards[5].set(decks[f].suit, decks[f].value);
                                myCards.cards[6].set(decks[g].suit, decks[g].value);

                                debug_print_input(myCards);
                                myCards.evaluator();
                                debug_print_result(myCards);
                                ++total;

                                if (total > 10000)
                                {
                                    cout<<"total "<<total<<endl;
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    cout<<"total "<<total<<endl;
}

myPoker::myPoker()
{
    for (int f = 1; f <= 4; ++f)
    {
        for (int v = Deuce; v <= Ace; ++v)
        {
            baseCard c;
            c.suit = f;
            c.value = v;
            cards.push_back(c);
        }
    }
    baseCard joker1;
    joker1.suit = 0;
    joker1.value = Black_Joker;
    baseCard joker2;
    joker2.suit = 0;
    joker2.value = Red_Joker;
    cards.push_back(joker1);
    cards.push_back(joker2);

    reShuffle();
}

void myPoker::remove(uint8_t suit, uint8_t v)
{
    for (std::vector<baseCard>::iterator it = cards.begin(); it != cards.end(); ++it)
    {
        if (it->suit == suit && it->value == v)
        {
            cards.erase(it);
            return;
        }
    }
}

void myPoker::reShuffle()
{
    cur = 0;
    std::random_shuffle (cards.begin(), cards.end(), myrandom);
}

baseCard* myPoker::deal()
{
    if (cur < cards.size())
    {
        cout << "deal " << cur << endl;
        baseCard* r = &cards[cur];
        ++cur;
        return r;
    }
    else
    {
        return NULL;
    }
}

void myPoker::reset_cur()
{
    cur = 0;
}

int myPoker::findPos(uint8_t suit, uint8_t v)
{
    int pos = 1;
    for (std::vector<baseCard>::iterator it = cards.begin(); it != cards.end(); ++it)
    {
        if (it->suit == suit && it->value == v)
        {
            return pos;
        }
        ++pos;
    }
    return -1;
}

void myPoker::swap(int pos1, int pos2)
{
    uint8_t tmp_suit, tmp_v;
    tmp_suit = cards[pos1-1].suit;
    tmp_v = cards[pos1-1].value;
    cards[pos1-1].suit = cards[pos2-1].suit;
    cards[pos1-1].value = cards[pos2-1].value;
    cards[pos2-1].suit = tmp_suit;
    cards[pos2-1].value = tmp_v;
}

