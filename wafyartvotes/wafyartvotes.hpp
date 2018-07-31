/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  票由MZ token抵押得到，以下简称MZP MZ Power
 *  MZP 有三种状态
 *  状态1：投票态，指该MZP参与了投票
 *  状态2：闲置态，指该MZP置换过来后未参与投票，未解锁。闲置态是默认的状态。
 *  状态3：解锁态，参与了解锁的MZP，解锁成功，MZP减少，Token增加。解锁可以撤销。
 * 
 *  关于MZP的操作，实质上是MZP在以上三种状态下的转换
 *  抵押MZP：  Token ----> MZP 闲置态               基于账户操作
 *  解锁MZP：  MZP 闲置态 ----> Token               基于账户操作
 *  投票MZP：  MZP 闲置态 ----> MZP 投票态           基于账户操作
 *  撤销投票：  MZP 投票态 ----> MZP 闲置态           基于账户操作
 *  
 *  奖励MZP用来给参与投票的用户、审核员、价值评论者作为激励。奖励MZP = 支付MZP + 增发MZP  奖励MZP 分为四部分
 *  价值评论者：支付MZP*40%+增发MZP                  基于文章操作
 *  投票参与者：支付MZP*40%                          基于分类操作  当该分类投票奖励超过界限值 VOTEREWORD 分发奖励 
 *  审核员：    支付MZP*15%                         基于分类操作  同投票参与者奖励一起分发
 *  资源支付金：支付MZP*5%                           基于项目操作  用于奖励RAM、CPU、NET、磁盘提供者
 */
#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;
using namespace std;
const uint64_t VOTELIMIT = 10000;       //投票界限，最少投一票1MZP 1.0000 MZP 10000 MMZP
const uint64_t AUDITLIMIT = 1;
const uint64_t VOTEREWORD = 10000;      //分发投票奖励界限，最少达到1票，再分发 1.0000 MZP
const uint64_t UNSTAKETIME = 30;

class wafyartvotes : public eosio::contract {
      public:
        using contract::contract;

        // scope为 _self 每个用户的票数统计
        // @abi table accounticks i64
        struct accountick{
            account_name byname;
            uint64_t totaltick;
            uint64_t unvotetick;                    //闲置态
            uint64_t votetick;                      //投票态
            uint64_t unstaketick;                   //解锁态

            uint64_t primary_key()const { return byname;}
            uint64_t get_total()const { return totaltick;}

            EOSLIB_SERIALIZE(accountick,(byname)(totaltick)(unvotetick)(votetick)(unstaketick))
        };
        typedef multi_index<N(accounticks),accountick,
            indexed_by<N(bytotal),const_mem_fun<accountick,uint64_t,&accountick::get_total>>,
            > accounticks;

        // scope为account_name  每个用户在不同分类下的投票分布
        // @abi table accvoteinfos i64
        struct accvoteinfo{
            account_name catename;
            uint64_t  votetick;

            uint64_t primary_key()const { return catename;}
        }
        typedef multi_index<N(accvoteinfos),accvoteinfo> accvoteinfos;

        // scope为account_name 每个用户的投票列表
        // @abi table votelists i64
        struct votelist{
            uint64_t id;
            account_name catename;
            uint64_t amount;                //参投的MZP数量
            uint64_t artid;                 //文章id
            uint64_t timestamp;             //时间戳，用来撤销投票

            uint64_t primary_key()const { return id;}
        }
        typedef multi_index<N(votelists),votelist> votelists;

        // 文章

        // 审核者

        // 提案

        // scope 为 _self 
        // @abi table cateticks i64
        struct catetick{
            account_name catename;
            uint64_t paytick;               //该分类赚取的收益，来自于发表文章的人，消耗的票，按比例分发出去
            uint64_t votetick;              //该分类获得的投票数，部分奖励根据投票数分发给投票人
            uint64_t addtick;               //根据投票，增发的票据，用来奖励价值评论者。

            uint64_t voterewo;              //投票者该分得的奖励
            uint64_t auditrewo;             //审核者该分得的奖励  此两种奖励是独立于文章之外的

            uint64_t primary_key()const { return catename;}
            uint64_t get_vote()const { return votetick;}
            uint64_t get_pay()const { return paytick;}

            bool checkvote(){
                if(voterewo>10000){return true;}
                else{return false;}
            }
            EOSLIB_SERIALIZE(catetick,(catename)(paytick)(votetick)(addtick)(voterewo)(auditrewo))
        };
        typedef multi_index<N(cateticks),catetick,
            indexed_by<N(byvote),const_mem_fun<catetick,uint64_t,&catetick::get_vote>>,
            indexed_by<N(bypay) ,const_mem_fun<catetick,uint64_t,&catetick::get_pay>>
            > cateticks;

        // scope为accountname
        // @abi table unstakes i64
        struct unstake{
            uint64_t id;
            uint64_t timestamp;
            uint64_t amount;
            uint64_t isend;
            uint128_t unstakeid;

            uint64_t primary_key()const {return id;}
            uint64_t get_time()const {return timestamp;}
            uint64_t get_isend()const { return isend;}

            EOSLIB_SERIALIZE(unstake,(id)(timestamp)(amount)(isend)(unstakeid))
        };
        typedef multi_index<N(unstakes),unstake,
            indexed_by<N(bytime),const_mem_fun<unstake,uint64_t,&unstake::get_time>>,
            indexed_by<N(byisend),const_mem_fun<unstake,uint64_t,&unstake::get_isend>>
            > unstakes;
        
        // scope为_self
        // @abi table senderids i64
        struct senderid{
            uint64_t id;
            uint64_t primary_key()const { return id;}

            EOSLIB_SERIALIZE(senderid,(id))
        };
        typedef multi_index<N(senderids),senderid> senderids;

        // scope为_self alltick 账户余额 = 抵押票（已投+未投+支付）+ 增发票
        // @abi table allticks i64
        struct alltick{
            account_name byname;
            uint64_t paytick;                       // 该项目总共获得的收益，来自于各个分类，支付的赏金
            uint64_t votetick;                      // 该项目抵押的票中，投出去的数目
            uint64_t unvotetick;                    // 该项目抵押的票中，未投出去的数目
            uint64_t unstaketick;                   // 该项目中解锁中的票
            uint64_t addtick;                       // 该项目增发的票

            uint64_t buyramtick;                    // 用作购买ram的票 基于该项目，不基于分类，更不基于项目

            uint64_t primary_key()const {return byname;}
        }
        typedef multi_index<N(allticks),alltick> allticks;

        // @abi action
        void staketit(account_name byname,account_name accname,uint64_t amount);
        // @abi action
        void unstaketit(account_name byname,uint64_t amount);
        // @abi action
        void setstats(account_name byname,account_name accname,uint64_t id);
        // @abi action
        void delunstake(account_name byname,uint64_t id);
        // @abi action
        void voteart(account_name byname,uint64_t votenum,account_name catename,uint64_t artid);
        // @abi action
        //void createart(account_name byname,)
        
    private:
        uint128_t getid(uint64_t id);
        
};

