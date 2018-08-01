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
const uint64_t UNSTAKETIME = 30;        //解锁ticket30秒之后会回复
const uint64_t VOTETIME    = 30;        //投票30秒后会回复
const uint64_t VOTERATIO   = 1000;      //1000个MZP投票，增发1个MZP奖励

class wafyartvotes : public eosio::contract {
      public:
        using contract::contract;
        using ipfshash_t=string;

        // scope为 _self 每个用户的票数统计
        // @abi table acctickets i64
        struct accticket{
            account_name byname;
            uint64_t totaltick;
            uint64_t unvotetick;                    //闲置态
            uint64_t votetick;                      //投票态
            uint64_t unstaketick;                   //解锁态

            uint64_t primary_key()const { return byname;}
            uint64_t get_total()const   { return totaltick;}

            EOSLIB_SERIALIZE(accticket,(byname)(totaltick)(unvotetick)(votetick)(unstaketick))
        };
        typedef multi_index<N(acctickets),accticket,
            indexed_by<N(bytotal),const_mem_fun<accticket,uint64_t,&accticket::get_total>>
            > acctickets;

        // scope为account_name  每个用户在不同分类下的投票分布
        // @abi table accvinfos i64
        struct accvinfo{
            account_name    catename;
            uint64_t        votetick;

            uint64_t primary_key()const { return catename;}

            EOSLIB_SERIALIZE(accvinfo,(catename)(votetick))
        };
        typedef multi_index<N(accvinfos),accvinfo> accvinfos;

        // scope为accountname 每个用户的抵押列表
        // @abi table accunstakes i64
        struct accunstake{
            uint64_t        id;
            uint64_t        timestamp;
            uint64_t        amount;
            uint64_t        isend;
            uint128_t       unstakeid;

            uint64_t primary_key()const {return id;}
            uint64_t get_time()const {return timestamp;}
            uint64_t get_isend()const { return isend;}

            EOSLIB_SERIALIZE(accunstake,(id)(timestamp)(amount)(isend)(unstakeid))
        };
        typedef multi_index<N(accunstakes),accunstake,
            indexed_by<N(bytime),const_mem_fun<accunstake,uint64_t,&accunstake::get_time>>,
            indexed_by<N(byisend),const_mem_fun<accunstake,uint64_t,&accunstake::get_isend>>
            > accunstakes;

        // 文章table，scope 为catename
        // @abi table articles i64
        struct article
        {
            uint64_t        id;                     //文章id

            uint64_t        votenum;                //文章得票数
            uint64_t        addtick;                //根据得票数增发奖励
            uint64_t        paytick;                //作者支付的金额

            string          title;                  //文章标题
            string          abstract;               //文章摘要
            account_name    author;                 //文章作者
            ipfshash_t      arthash;                //文章保存在ipfs网络上的哈希值
            uint64_t        timestamp;              //文章创建的时间戳
            uint64_t        isend;                  //文章是否发现价值评论

            uint64_t primary_key() const    { return id; }              //主键索引
            uint64_t get_author() const     { return author; }          //作者名索引
            uint64_t get_createtime() const { return timestamp; }       //时间戳索引
            uint64_t get_isend() const      { return isend; }           //是否完结索引
            uint64_t get_vnum() const       { return votenum;}          //得票数索引

            EOSLIB_SERIALIZE(article, (id)(votenum)(addtick)(paytick)(title)(abstract)(author)(arthash)(timestamp)(isend))
        };
        typedef multi_index<N(articles),article,
            indexed_by<N(byauthor), const_mem_fun<article,uint64_t,&article::get_author>>,
            indexed_by<N(bytime),   const_mem_fun<article,uint64_t,&article::get_createtime>>,
            indexed_by<N(byisend),  const_mem_fun<article,uint64_t,&article::get_isend>>,
            indexed_by<N(byvnum),   const_mem_fun<article,uint64_t,&article::get_vnum>>
            > articles;
        // scope 为 catename 
        // @abi table auditorlists i64
        struct auditorlist
        {
            account_name    auditor;
            uint64_t        amount;                //得票数
            uint64_t        timestamp;             //时间戳

            uint64_t primary_key()const { return auditor;}      //主键索引
            uint64_t get_amount()const  { return amount;}       //得票数索引

            EOSLIB_SERIALIZE(auditorlist,(auditor)(amount)(timestamp))
        };
        typedef multi_index<N(auditorlists),auditorlist,
            indexed_by<N(byamount),const_mem_fun<auditorlist,uint64_t,&auditorlist::get_amount>>
            > auditorlists;
        // 提案

        // scope 为 _self   类别所得票信息，以及类别自身信息
        // @abi table cates i64
        struct cate{
            account_name    catename;              //类别名称
            uint64_t        reword;                //该分类当前的收益，来自于发表文章的人的60%比例
            uint64_t        votetick;              //该分类当前的投票数，部分奖励根据投票数分发给投票人

            uint64_t        paylimit;              //最小支付悬赏金额
            string          memo;                  //类别备注
            account_name    createname;            //类别创建者
            uint64_t        articlenum;            //包含的文章数

            uint64_t primary_key()const { return catename;}     //主键索引
            uint64_t get_vote() const   { return votetick;}     //得票数索引    
            uint64_t get_create() const { return createname; }  //创建者索引
            uint64_t get_artnum() const { return articlenum; }  //文章数量索引

            void addcatenum() { articlenum++; } //递增类别中的文章计数
            void subcatenum() { articlenum--; } //递减类别中的文章计数

            EOSLIB_SERIALIZE(cate,(catename)(reword)(votetick)(paylimit)(memo)(createname)(articlenum))
        };
        typedef multi_index<N(cates),cate,
            indexed_by<N(byvote),   const_mem_fun<cate,uint64_t,&cate::get_vote>>,
            indexed_by<N(bycreate), const_mem_fun<cate,uint64_t,&cate::get_create>>,
            indexed_by<N(byartnum), const_mem_fun<cate,uint64_t,&cate::get_artnum>>
            > cates;
        
        // scope为_self
        // @abi table senderids i64
        struct senderid{
            uint64_t id;
            uint64_t primary_key()const { return id;}

            EOSLIB_SERIALIZE(senderid,(id))
        };
        typedef multi_index<N(senderids),senderid> senderids;

        // scope为_self 抵押的MZP 解压中的MZP 增发的MZP 用于购买资源的MZP
        // @abi table allticks i64
        struct alltick{
            account_name byname;
            uint64_t staketick;                     // 该项目抵押的票中，未投出去的数目
            uint64_t unstaketick;                   // 该项目中解锁中的票
            uint64_t addtick;                       // 该项目增发的票
            uint64_t devfunds;                      // 用作开发资金、购买ram等等

            uint64_t primary_key()const {return byname;}
            EOSLIB_SERIALIZE(alltick,(byname)(staketick)(unstaketick)(addtick)(devfunds))
        };
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
        void redeemvote(account_name byname,account_name accname,account_name catename,uint64_t amount,bool type,uint64_t index);
        // @abi action
        void regauditor(account_name byname,account_name catename);
        // @abi action
        void delauditor(account_name byname,account_name catename);
        // @abi action
        void voteaud(account_name byname,uint64_t votenum,account_name catename,account_name auditor);


    private:
        uint128_t getsenderid(account_name cate);
        bool findcate(account_name catename){
            cates catemul(_self,_self);
            auto cateit=catemul.find(catename);
            if(cateit!=catemul.end())
                return true;
            else
                return false;
        }
        bool findartid(uint64_t id,account_name catename){
            articles artmul(_self,catename);
            auto artit=artmul.find(id);
            if(artit!=artmul.end())
                return true;
            else
                return false;
        }
        // bool findcomid(uint64_t id,account_name catename){
        //     comments commul(_self,catename);
        //     auto comit=commul.find(id);
        //     if(comit!=commul.end())
        //         return true;
        //     else
        //         return false;
        // }
        bool findaudit(account_name auditor,account_name catename){
            auditorlists audimul(_self,catename);
            auto audiit=audimul.find(auditor);
            if(audiit!=audimul.end())
                return true;
            else
                return false;
        }
};

