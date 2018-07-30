#include "wafyartvotes.hpp"

uint128_t wafyartvotes::getid(uint64_t id){
    uint128_t scopeid = N(wafyartvotes);
    print(scopeid);
    uint128_t result  = (scopeid<<64) +id;
    print("ssssssssss");
    print(result);
    return result;
}
void wafyartvotes::staketit(account_name byname,account_name accname,uint64_t amount){
    // 检查签名是否匹配
    require_auth(byname);
    eosio_assert(byname==N(wafyarttoken),"错误：此帐号没有权限调用此action");

    alltickets ticmul(_self,_self);
    auto ticit=ticmul.find(accname);
    
    if(ticit!=ticmul.end()){
        ticmul.modify(ticit,_self,[&](auto &obj){
            obj.total=obj.total+amount;
            obj.balance=obj.balance+amount;
        });
    }else{
        ticmul.emplace(_self,[&](auto& obj){
            obj.byname=accname;
            obj.total=amount;
            obj.balance=amount;
            obj.locking=0;
            obj.unstake=0;
        });
    }
}
void wafyartvotes::setstats(account_name byname,account_name accname,uint64_t id){
    require_auth(byname);

    eosio_assert(byname==N(wafyartvotes),"错误：该账户没有权限修改解锁票状态");

    unstakes unstamul(_self,accname);
    auto unstait=unstamul.find(id);

    eosio_assert(unstait!=unstamul.end(),"错误：未发现该条抵押记录");
    unstamul.modify(unstait,_self,[&](auto &obj){
        obj.isend=true;
    });
}
void wafyartvotes::unstaketit(account_name byname,uint64_t amount){
    require_auth(byname);

    alltickets ticmul(_self,_self);
    auto ticit=ticmul.find(byname);

    eosio_assert(ticit!=ticmul.end(),"错误：该用户没有抵押过票");
    eosio_assert(ticit->balance>amount,"错误：该用户没有足够的票解锁");
    eosio_assert(amount>10000,"错误：防止恶意ddos攻击，最少解锁1 MZ的投票 ");

    //生成合约唯一id
    senderids senmul(_self,_self);
    uint64_t id=senmul.available_primary_key();
    senmul.emplace(_self,[&](auto &obj){
        obj.id=id;
    });

    uint128_t unstakeid=getid(id);

    unstakes unstamul(_self,byname);
    uint64_t unstaid=unstamul.available_primary_key();

    unstamul.emplace(_self,[&](auto &obj){
        obj.id=unstaid;
        obj.timestamp=now();
        obj.amount=amount;
        obj.isend=false;
        obj.unstakeid=unstakeid;
    });

    ticmul.modify(ticit,_self,[&](auto &obj){
        obj.total=obj.total-amount;
        obj.balance=obj.balance-amount;
        obj.unstake=obj.unstake+amount;
    });

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
void wafyartvotes::delunstake(account_name byname,uint64_t id){
    require_auth(byname);

    unstakes unstamul(_self,byname);
    auto unstait=unstamul.find(id);

    eosio_assert(unstait!=unstamul.end(),"错误：不存在该解锁记录");

    alltickets ticmul(_self,_self);
    auto ticit=ticmul.find(byname);
    eosio_assert(ticit!=ticmul.end(),"错误：该用户没有抵押记录");

    ticmul.modify(ticit,_self,[&](auto& obj){
        obj.total=obj.total+unstait->amount;
        obj.balance=obj.balance+unstait->amount;
    });
    uint128_t unstakeid=unstait->unstakeid;
    unstamul.erase(unstait);

    //取消延迟交易
    cancel_deferred(unstakeid);
}
EOSIO_ABI( wafyartvotes, (staketit)(unstaketit)(setstats)(delunstake))
