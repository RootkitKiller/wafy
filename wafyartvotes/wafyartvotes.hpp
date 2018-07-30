/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;
using namespace std;
const uint64_t VOTELIMIT = 10000;
const uint64_t AUDITLIMIT = 1;
const uint64_t UNSTAKETIME = 30;

class wafyartvotes : public eosio::contract {
      public:
        using contract::contract;

        // scope为 _self 每个用户的票数统计
        // @abi table alltickets i64
        struct allticket{
            account_name byname;
            uint64_t total;
            uint64_t balance;
            uint64_t locking;
            uint64_t unstake;

            uint64_t primary_key()const { return byname;}
            uint64_t get_total()const { return total;}
            uint64_t get_bala()const { return balance;}
            uint64_t get_lock()const { return locking;}

            EOSLIB_SERIALIZE(allticket,(byname)(total)(balance)(locking)(unstake))
        };
        typedef multi_index<N(alltickets),allticket,
            indexed_by<N(bytotal),const_mem_fun<allticket,uint64_t,&allticket::get_total>>,
            indexed_by<N(bybala),const_mem_fun<allticket,uint64_t,&allticket::get_bala>>,
            indexed_by<N(bylock),const_mem_fun<allticket,uint64_t,&allticket::get_lock>>
            > alltickets;
        
        // scope 为 _self 
        // @abi table catepots i64
        struct catepot{
            account_name catename;
            uint64_t totalvote;
            uint64_t totalrewo;
            uint64_t bestrewo;
            uint64_t voterewo;
            uint64_t auditrewo;
            uint64_t buyramrewo;

            uint64_t primary_key()const { return catename;}
            uint64_t get_vote()const { return totalvote;}
            uint64_t get_rewo()const { return totalrewo;}

            bool checkvote(){
                if(voterewo>10000){return true;}
                else{return false;}
            }
            bool checkaudit(){
                if(auditrewo>1){return true;}
                else{return false;}
            }
            EOSLIB_SERIALIZE(catepot,(catename)(totalvote)(totalrewo)(bestrewo)(voterewo)(auditrewo)(buyramrewo))
        };
        typedef multi_index<N(catepots),catepot,
            indexed_by<N(byvote),const_mem_fun<catepot,uint64_t,&catepot::get_vote>>,
            indexed_by<N(byrewo),const_mem_fun<catepot,uint64_t,&catepot::get_rewo>>
            > catepots;

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

        // @abi action
        void staketit(account_name byname,account_name accname,uint64_t amount);
        // @abi action
        void unstaketit(account_name byname,uint64_t amount);
        // @abi action
        void setstats(account_name byname,account_name accname,uint64_t id);
        // @abi action
        void delunstake(account_name byname,uint64_t id);
    private:
        uint128_t getid(uint64_t id);
        
};

