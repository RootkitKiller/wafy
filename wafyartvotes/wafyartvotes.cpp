#include "wafyartvotes.hpp"

void wafyartvotes::staketit(account_name byname,account_name accname,uint64_t amount){
    // 检查签名是否匹配
    require_auth(byname);
    eosio_assert(byname==N(wafyarttoken),"错误：此帐号没有权限调用此action");

    alltickets ticmul(_self,_self);
    auto ticit=ticmul.find(accname);
    
    if(ticit!=ticmul.end()){
        ticmul.modify(ticit,_self,[&](auto &obj){
            obj.total=obj.total+amount;
        });
    }else{
        ticmul.emplace(_self,[&](auto& obj){
            obj.byname=accname;
            obj.total=amount;
            obj.balance=amount;
            obj.locking=0;
            obj.unstake=0;
        })
    }
}
void wafyartvotes::unstaketit(account_name byname,uint64_t amount){
    require_auth(byname);

    alltickets ticmul(_self,_self);
    auto ticit=ticmul.find(byname);

    eosio_assert(ticit!=ticmul.end(),"错误：该用户没有抵押过票");
    eosio_assert(ticit->balance>amount,"错误：该用户没有足够的票解锁");

    unstakes unstamul(_self,byname);
    unstamul.emplace(_self,[&](auto &obj){
        obj.id=unstamul.available_primary_key();
        obj.timestamp=now();
        obj.untime=now()-obj.timestamp;
        obj.amount=amount;
    });

    ticmul.modify(ticit,_self,[&](auto &obj){
        obj.total=obj.total-amount;
        obj.balance=obj.balance-amount;
        obj.unstake=obj.unstake+amount;
    });

    transaction t;
    symbol_type MZSYMBOL = symbol_type(string_to_symbol(4, "MZ"));
    t.actions.emplace_back(
        permission_level(_self,N(active)),
        N(wafyarttoken),
        N(transfer),
        make_tuple(_self,N(byname),asset(amount,MZSYMBOL),string("unstake")));
    t.delay_sec =15;
    t.send(N(unstake),_self);

}
void wafyartvotes::delunstake(account_name byname,uint64_t id){
    require_auth(byname);

    unstakes unstamul(_self,byname);
    auto unstait=unstamul.find(id);

    eosio_assert(unstait!=unstamul.end(),"错误：不存在该条解锁记录");

    alltickets ticmul(_self,_self);
    auto ticit=ticmul.find(byname);
    eosio_assert(ticit!=ticmul.end(),"错误：该用户没有抵押记录");

    ticmul.modify(ticit,_self,[&](auto& obj){
        obj.total=obj.total+unstait->amount;
        obj.balance=obj.balance+unstait->amount;
    });
    unstamul.erase(unstait);
}
EOSIO_ABI( wafyartvotes, (staketit)(unstaketit)(delunstake))
