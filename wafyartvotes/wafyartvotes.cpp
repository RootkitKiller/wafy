#include "wafyartvotes.hpp"

// 生成该合约内唯一的id
uint128_t wafyartvotes::getid(uint64_t id){
    uint128_t scopeid = N(wafyartvotes);
    uint128_t result  = (scopeid<<64) +id;
    return result;
}
// 抵押代币，只能用wafyarttoken账户调用
void wafyartvotes::staketit(account_name byname,account_name accname,uint64_t amount){
    // 检查签名是否匹配
    require_auth(byname);
    eosio_assert(byname==N(wafyarttoken),"错误：此帐号没有权限调用此action");

    // 更新账户票统计信息
    accounticks accticmul(_self,_self);
    auto accticit=accticmul.find(accname);
    
    if(accticit!=accticmul.end()){
        accticmul.modify(accticit,_self,[&](auto &obj){
            obj.totaltick=obj.totaltick+amount;
            obj.unvotetick=obj.unvotetick+amount;
        });
    }else{
        accticmul.emplace(_self,[&](auto& obj){
            obj.byname=accname;
            obj.totaltick=amount;
            obj.unvotetick=amount;
            obj.votetick=0;
            obj.unstaketick=0;
        });
    }
    // 更新项目票统计信息
    allticks allticmul(_self,_self);
    auto allticit=allticmul.find(_self);
    if(allticit!=allticmul.end()){
        allticmul.modify(allticit,_self,[&](auto &obj){
            obj.unvotetick=obj.unvotetick+amount;
        });
    }else{
        allticmul.emplace(_self,[&](auto& obj){
            obj.byname=_self;
            obj.paytick=0;
            obj.votetick=0;
            obj.unvotetick=amount;
            obj.unstaketick=0;
            obj.addtick=0;
            obj.buyramtick=0;
        });
    }
}
// 设置解锁代币的状态，解锁完成时调用
void wafyartvotes::setstats(account_name byname,account_name accname,uint64_t id){
    require_auth(byname);

    eosio_assert(byname==N(wafyartvotes),"错误：该账户没有权限修改解锁票状态");

    unstakes unstamul(_self,accname);
    auto unstait=unstamul.find(id);
    // 更新解锁MZP列表
    eosio_assert(unstait!=unstamul.end(),"错误：未发现该条抵押记录");
    unstamul.modify(unstait,_self,[&](auto &obj){
        obj.isend=true;
    });
    // 更新用户票统计信息
    accounticks accticmul(_self,_self);
    auto accticit=accticmul.find(accname);
    eosio_assert(accticit!=accticmul.end(),"错误：该用户没有抵押过票");
    accticmul.modify(ticit,_self,[&](auto &obj){
        obj.unstaketick=obj.unstaketick-amount;
    });
    // 更新项目票统计信息
    allticks allticmul(_self,_self);
    auto allticit=allticmul.find(_self);
    eosio_assert(allticit!=allticmul.end(),"错误：未发现项目票统计信息")
    allticmul.modify(allticit,_self,[&](auto &obj){
        obj.unvotetick  = obj.unvotetick  - amount;
        obj.unstaketick = obj.unstaketick + amount;
    });
}
// 解锁代币，由用户调用
void wafyartvotes::unstaketit(account_name byname,uint64_t amount){
    require_auth(byname);

    accounticks accticmul(_self,_self);
    auto accticit=accticmul.find(byname);

    eosio_assert(accticit!=accticmul.end(),"错误：该用户没有抵押过票");
    eosio_assert(accticit->unvotetick>amount,"错误：该用户没有足够的票解锁");
    eosio_assert(amount>10000,"错误：防止恶意ddos攻击，最少解锁1 MZ的投票 ");

    //生成合约唯一id
    senderids senmul(_self,_self);
    uint64_t id=senmul.available_primary_key();
    senmul.emplace(_self,[&](auto &obj){
        obj.id=id;
    });
    uint128_t unstakeid=getid(id);

    // 更新解锁MZP列表
    unstakes unstamul(_self,byname);
    uint64_t unstaid=unstamul.available_primary_key();
    unstamul.emplace(_self,[&](auto &obj){
        obj.id=unstaid;
        obj.timestamp=now();
        obj.amount=amount;
        obj.isend=false;
        obj.unstakeid=unstakeid;
    });
    // 更新用户票统计信息
    accticmul.modify(ticit,_self,[&](auto &obj){
        obj.totaltick=obj.totaltick-amount;
        obj.unvotetick=obj.unvotetick-amount;
        obj.unstaketick=obj.unstaketick+amount;
    });
    // 更新项目票统计信息
    allticks allticmul(_self,_self);
    auto allticit=allticmul.find(_self);
    eosio_assert(allticit!=allticmul.end(),"错误：未发现项目票统计信息")
    allticmul.modify(allticit,_self,[&](auto &obj){
        obj.unvotetick  = obj.unvotetick  - amount;
        obj.unstaketick = obj.unstaketick + amount;
    });

    // 设置延时交易
    transaction ttrans;
    symbol_type MZSYMBOL = symbol_type(string_to_symbol(4, "MZ"));

    ttrans.actions.emplace_back(
        permission_level(_self,N(active)),
        N(wafyartvotes),
        N(setstats),
        make_tuple(_self,byname,unstaid));
    ttrans.actions.emplace_back(
        permission_level(_self,N(active)),
        N(wafyarttoken),
        N(transfer),
        make_tuple(_self,byname,asset(amount,MZSYMBOL),string("unstake")));
    ttrans.delay_sec =UNSTAKETIME;

    ttrans.send(unstakeid,_self);

}
// 撤销解锁代币调用
void wafyartvotes::delunstake(account_name byname,uint64_t id){
    require_auth(byname);

    unstakes unstamul(_self,byname);
    auto unstait=unstamul.find(id);
    eosio_assert(unstait!=unstamul.end(),"错误：不存在该解锁记录");

    // 更新用户票信息
    accounticks accticmul(_self,_self);
    auto accticit=accticmul.find(byname);
    eosio_assert(accticit!=accticmul.end(),"错误：该用户没有抵押记录");
    accticmul.modify(accticit,_self,[&](auto& obj){
        obj.totaltick   = obj.totaltick   + unstait->amount;
        obj.unvotetick  = obj.unvotetick  + unstait->amount;
        obj.unstaketick = obj.unstaketick - unstait->amount;
    });
    uint128_t unstakeid=unstait->unstakeid;
    // 更新解锁MZP列表
    unstamul.erase(unstait);
    // 更新项目票信息
    allticks allticmul(_self,_self);
    auto allticit=allticmul.find(_self);
    eosio_assert(allticit!=allticmul.end(),"错误：未发现项目票统计信息")
    allticmul.modify(allticit,_self,[&](auto &obj){
        obj.unvotetick  = obj.unvotetick  + amount;
        obj.unstaketick = obj.unstaketick - amount;
    });

    //取消延迟交易
    cancel_deferred(unstakeid);
}
// 给文章投票
void voteart(account_name byname,uint64_t votenum,account_name catename,uint64_t artid){
    require_auth(byname);

    
}
EOSIO_ABI( wafyartvotes, (staketit)(unstaketit)(setstats)(delunstake)(voteart))
