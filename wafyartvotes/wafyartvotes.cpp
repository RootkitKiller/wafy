#include "wafyartvotes.hpp"

// 生成该合约内唯一的id
uint128_t wafyartvotes::getsenderid(account_name cate){
    senderids senmul(_self,_self);
    uint64_t id=senmul.available_primary_key();
    senmul.emplace(_self,[&](auto &obj){
        obj.id=id;
    });
    uint128_t scopeid = cate;
    uint128_t result  = (scopeid<<64) +id;
    return result;
}
// 抵押代币，只能用wafyarttoken账户调用
void wafyartvotes::staketit(account_name byname,account_name accname,uint64_t amount){
    // 检查签名是否匹配
    require_auth(byname);
    eosio_assert(byname==N(wafyarttoken),"错误：此帐号没有权限调用此action");

    // 更新账户资产表
    acctickets accticmul(_self,_self);
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
    // 更新项目资产表
    allticks allticmul(_self,_self);
    auto allticit=allticmul.find(_self);
    if(allticit!=allticmul.end()){
        allticmul.modify(allticit,_self,[&](auto &obj){
            obj.staketick=obj.staketick+amount;
        });
    }else{
        allticmul.emplace(_self,[&](auto& obj){
            obj.byname=_self;
            obj.staketick=amount;
            obj.unstaketick=0;
            obj.addtick=0;
            obj.devfunds=0;
        });
    }
}
// 设置解锁代币的状态，解锁完成时调用
void wafyartvotes::setstats(account_name byname,account_name accname,uint64_t id){
    require_auth(byname);

    eosio_assert(byname==N(wafyartvotes),"错误：该账户没有权限修改解锁票状态");

    accunstakes unstamul(_self,accname);
    auto unstait=unstamul.find(id);
    // 更新解锁MZP表
    eosio_assert(unstait!=unstamul.end(),"错误：未发现该条抵押记录");
    uint64_t amount=unstait->amount;
    unstamul.modify(unstait,_self,[&](auto &obj){
        obj.isend=true;
    });
    // 更新账户资产表
    acctickets accticmul(_self,_self);
    auto accticit=accticmul.find(accname);
    eosio_assert(accticit!=accticmul.end(),"错误：该用户没有抵押过票");
    accticmul.modify(accticit,_self,[&](auto &obj){
        obj.unstaketick=obj.unstaketick-amount;
    });
    // 更新项目资产表
    allticks allticmul(_self,_self);
    auto allticit=allticmul.find(_self);
    eosio_assert(allticit!=allticmul.end(),"错误：未发现项目票统计信息");
    allticmul.modify(allticit,_self,[&](auto &obj){
        obj.unstaketick = obj.unstaketick - amount;
    });
}
// 解锁代币，由用户调用
void wafyartvotes::unstaketit(account_name byname,uint64_t amount){
    require_auth(byname);

    acctickets accticmul(_self,_self);
    auto accticit=accticmul.find(byname);

    eosio_assert(accticit!=accticmul.end(),"错误：该用户没有抵押过票");
    eosio_assert(accticit->unvotetick>amount,"错误：该用户没有足够的票解锁");
    eosio_assert(amount>10000,"错误：防止恶意ddos攻击，最少解锁1 MZ的投票 ");

    //生成合约唯一id
    uint128_t unstakeid=getsenderid(N(wafyunstake));

    // 更新解锁MZP列表
    accunstakes unstamul(_self,byname);
    uint64_t unstaid=unstamul.available_primary_key();
    unstamul.emplace(_self,[&](auto &obj){
        obj.id=unstaid;
        obj.timestamp=now();
        obj.amount=amount;
        obj.isend=false;
        obj.unstakeid=unstakeid;
    });
    // 更新账户资产表
    accticmul.modify(accticit,_self,[&](auto &obj){
        obj.totaltick=obj.totaltick-amount;
        obj.unvotetick=obj.unvotetick-amount;
        obj.unstaketick=obj.unstaketick+amount;
    });
    // 更新项目资产表
    allticks allticmul(_self,_self);
    auto allticit=allticmul.find(_self);
    eosio_assert(allticit!=allticmul.end(),"错误：未发现项目票统计信息");
    allticmul.modify(allticit,_self,[&](auto &obj){
        obj.staketick  = obj.staketick  - amount;
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

    accunstakes unstamul(_self,byname);
    auto unstait=unstamul.find(id);
    eosio_assert(unstait!=unstamul.end(),"错误：不存在该解锁记录");

    // 更新账户资产表
    acctickets accticmul(_self,_self);
    auto accticit=accticmul.find(byname);
    eosio_assert(accticit!=accticmul.end(),"错误：该用户没有抵押记录");
    uint64_t amount=unstait->amount;
    accticmul.modify(accticit,_self,[&](auto& obj){
        obj.totaltick   = obj.totaltick   + unstait->amount;
        obj.unvotetick  = obj.unvotetick  + unstait->amount;
        obj.unstaketick = obj.unstaketick - unstait->amount;
    });
    uint128_t unstakeid=unstait->unstakeid;
    // 更新解锁MZP列表
    unstamul.erase(unstait);
    // 更新项目资产表
    allticks allticmul(_self,_self);
    auto allticit=allticmul.find(_self);
    eosio_assert(allticit!=allticmul.end(),"错误：未发现项目票统计信息");
    allticmul.modify(allticit,_self,[&](auto &obj){
        obj.staketick  = obj.staketick  + amount;
        obj.unstaketick = obj.unstaketick - amount;
    });

    //取消延迟交易
    cancel_deferred(unstakeid);
}
// 赎回投票
void wafyartvotes::redeemvote(account_name byname,account_name accname,account_name catename,uint64_t amount,bool type,uint64_t index){
    // 验证签名
    require_auth(byname);
    eosio_assert(byname==N(wafyartvotes),"错误：调用者只能为合约本身");
    eosio_assert(type==0||type==1,"错误：类型为0或者1");

    // 更新账户投票分布表
    accvinfos accvinmul(_self,accname);
    auto accvinit=accvinmul.find(catename);
    accvinmul.modify(accvinit,_self,[&](auto &obj){
        obj.votetick=obj.votetick-amount;
    });
    //根据类型，选择不同的撤回方式
    if(type==1){
        //撤回该文章的所得票
        articles artmul(_self,catename);
        auto artit=artmul.find(index);
        artmul.modify(artit,_self,[&](auto &obj){
            obj.votenum = obj.votenum-amount;
        });
    }else{
        //撤回审核者的所得票
        auditorlists audimul(_self,catename);
        auto audiit=audimul.find(index);
        audimul.modify(audiit,_self,[&](auto &obj){
            obj.amount = obj.amount-amount;
        });
    }
    // 发送此次投票获得的奖励
    // 更新类别表 - 类别所获得的投票、奖励、收益等
    cates catmul(_self,_self);
    auto catit=catmul.find(catename);
    uint64_t reword=(catit->reword)*((double)amount/(double)(catit->votetick));
    catmul.modify(catit,_self,[&](auto &obj){
        obj.reword=obj.reword-reword;
        obj.votetick=obj.votetick-amount;
    });
    // 更新账户资产表
    acctickets accticmul(_self,_self);
    auto accticit=accticmul.find(accname);
    accticmul.modify(accticit,_self,[&](auto &obj){
        obj.unvotetick=obj.unvotetick+amount+reword;
        obj.votetick=obj.votetick-amount;
    });

}
// 给文章投票
void wafyartvotes::voteart(account_name byname,uint64_t votenum,account_name catename,uint64_t artid){
    // 验证签名
    require_auth(byname);
    // 验证账户与ticket是否满足条件
    acctickets accticmul(_self,_self);
    auto accticit=accticmul.find(byname);
    eosio_assert(accticit!=accticmul.end(),"错误：该账户尚未抵押过Token");
    eosio_assert(accticit->unvotetick>votenum,"错误：投票数量超过余额");
    eosio_assert(votenum>10000,"错误：投票数量最少为1MZP");
    // 验证分类、文章id是否存在
    eosio_assert(findcate(catename)==true,"错误：该分类未定义");
    eosio_assert(findartid(artid,catename)==true,"错误：文章id不存在");

    // 更新账户投票分布表
    accvinfos accvinmul(_self,byname);
    auto accvinit=accvinmul.find(catename);
    if(accvinit==accvinmul.end()){
        accvinmul.emplace(_self,[&](auto &obj){
            obj.catename=catename;
            obj.votetick=votenum;
        });
    }else{
        accvinmul.modify(accvinit,_self,[&](auto &obj){
            obj.votetick=obj.votetick+votenum;
        });
    }
    // 更新文章表 - 文章获得投票数
    articles artmul(_self,catename);
    auto artit=artmul.find(artid);
    uint64_t addtick=votenum/VOTERATIO;
    artmul.modify(artit,_self,[&](auto &obj){
        obj.votenum = obj.votenum+votenum;
        obj.addtick = obj.addtick+addtick;
    });
    // 更新类别表 - 类别所获得的投票、奖励、收益等
    cates catmul(_self,_self);
    auto catit=catmul.find(catename);
    catmul.modify(catit,_self,[&](auto &obj){
        obj.votetick=obj.votetick+votenum;
    });
    // 更新账户资产表
    accticmul.modify(accticit,_self,[&](auto &obj){
        obj.unvotetick=obj.unvotetick-votenum;
        obj.votetick=obj.votetick+votenum;
    });
    // 更新项目资产表
    allticks allticmul(_self,_self);
    auto allticit=allticmul.find(_self);
    eosio_assert(allticit!=allticmul.end(),"错误：未发现项目票统计信息");
    allticmul.modify(allticit,_self,[&](auto &obj){
        obj.addtick  = obj.addtick  + addtick;
    });
    // 设置延时交易
    // 生成合约唯一id
    uint128_t voteartid=getsenderid(N(wafyvoteart));
    transaction ttrans;
    ttrans.actions.emplace_back(
        permission_level(_self,N(active)),
        N(wafyartvotes),
        N(redeemvote),
        make_tuple(_self,byname,catename,votenum,0,artid));
    ttrans.delay_sec =VOTETIME;
    ttrans.send(voteartid,_self);
}
// 申请成为审核者
void wafyartvotes::regauditor(account_name byname,account_name catename){
    require_auth(byname);

    // 验证账户的ticket是否满足条件
    acctickets accticmul(_self,_self);
    auto accticit=accticmul.find(byname);
    eosio_assert(accticit!=accticmul.end(),"错误：该账户尚未抵押过Token");
    eosio_assert(accticit->unvotetick>1000000,"错误：申请成为审核者需要支付100MZP，余额不足");

    auditorlists audimul(_self,catename);
    auto audiit=audimul.find(byname);
    eosio_assert(audiit==audimul.end(),"错误：已经申请过了，请勿重复申请");

    // 更新账户资产表
    accticmul.modify(accticit,_self,[&](auto &obj){
        obj.unvotetick=obj.unvotetick-1000000;
    });
    // 更新类别表
    cates catmul(_self,_self);
    auto catit=catmul.find(catename);
    catmul.modify(catit,_self,[&](auto &obj){
        obj.reword=obj.reword+1000000;
    });
    // 更新审核者列表
    audimul.emplace(_self,[&](auto &obj){
        obj.auditor=byname;
        obj.amount=0;
        obj.timestamp=now();
    });
}
// 取消审核者
void wafyartvotes::delauditor(account_name byname,account_name catename){
    require_auth(byname);

    auditorlists audimul(_self,catename);
    auto audiit=audimul.find(byname);
    eosio_assert(audiit==audimul.end(),"错误：审核者列表中不存在该用户");

    audimul.erase(audiit);
}
// 为审核者投票
void wafyartvotes::voteaud(account_name byname,uint64_t votenum,account_name catename,account_name auditor){
    // 验证签名
    require_auth(byname);
    // 验证账户与ticket是否满足条件
    acctickets accticmul(_self,_self);
    auto accticit=accticmul.find(byname);
    eosio_assert(accticit!=accticmul.end(),"错误：该账户尚未抵押过Token");
    eosio_assert(accticit->unvotetick>votenum,"错误：投票数量超过余额");
    eosio_assert(votenum>10000,"错误：投票数量最少为1MZP");
    // 验证分类、审核者是否存在
    eosio_assert(findcate(catename)==true,"错误：该分类未定义");
    eosio_assert(findaudit(auditor,catename)==true,"错误：文章id不存在");

    // 更新账户投票分布表
    accvinfos accvinmul(_self,byname);
    auto accvinit=accvinmul.find(catename);
    if(accvinit==accvinmul.end()){
        accvinmul.emplace(_self,[&](auto &obj){
            obj.catename=catename;
            obj.votetick=votenum;
        });
    }else{
        accvinmul.modify(accvinit,_self,[&](auto &obj){
            obj.votetick=obj.votetick+votenum;
        });
    }
    // 更新审核员表 
    auditorlists audimul(_self,catename);
    auto audiit=audimul.find(auditor);
    audimul.modify(audiit,_self,[&](auto &obj){
        obj.amount = obj.amount+votenum;
    });
    // 更新类别表 - 类别所获得的投票、奖励、收益等
    cates catmul(_self,_self);
    auto catit=catmul.find(catename);
    catmul.modify(catit,_self,[&](auto &obj){
        obj.votetick=obj.votetick+votenum;
    });
    // 更新账户资产表
    accticmul.modify(accticit,_self,[&](auto &obj){
        obj.unvotetick=obj.unvotetick-votenum;
        obj.votetick=obj.votetick+votenum;
    });

    // 设置延时交易
    uint128_t voteaudiid=getsenderid(N(wafyvoteaudi));
    transaction ttrans;
    ttrans.actions.emplace_back(
        permission_level(_self,N(active)),
        N(wafyartvotes),
        N(redeemvote),
        make_tuple(_self,byname,catename,votenum,1,auditor));
    ttrans.delay_sec =VOTETIME;
    ttrans.send(voteaudiid,_self);
}
EOSIO_ABI( wafyartvotes, (staketit)(unstaketit)(setstats)(delunstake)(voteart)(redeemvote)(regauditor)(delauditor)(voteaud))
