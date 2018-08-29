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
            obj.idletick=obj.idletick+amount;
        });
    }else{
        accticmul.emplace(_self,[&](auto& obj){
            obj.byname=accname;
            obj.idletick=amount;
            obj.votetick=0;
            obj.unstaketick=0;
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
}
// 解锁代币，由用户调用
void wafyartvotes::unstaketit(account_name byname,uint64_t amount){
    require_auth(byname);

    acctickets accticmul(_self,_self);
    auto accticit=accticmul.find(byname);

    eosio_assert(accticit!=accticmul.end(),"错误：该用户没有抵押过票");
    eosio_assert(accticit->idletick>amount,"错误：该用户没有足够的票解锁");
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
        obj.idletick=obj.idletick-amount;
        obj.unstaketick=obj.unstaketick+amount;
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
        obj.idletick  = obj.idletick  + unstait->amount;
        obj.unstaketick = obj.unstaketick - unstait->amount;
    });
    uint128_t unstakeid=unstait->unstakeid;
    // 更新解锁MZP列表
    unstamul.erase(unstait);

    //取消延迟交易
    cancel_deferred(unstakeid);
}
// 赎回投票
void wafyartvotes::redeemvote(account_name byname,account_name accname,account_name catename,uint64_t amount){
    // 验证签名
    require_auth(byname);
    eosio_assert(byname==N(wafyartvotes),"错误：调用者只能为合约本身");
    //eosio_assert(type==0||type==1,"错误：类型为0或者1");

    // 更新账户投票分布表
    accvinfos accvinmul(_self,accname);
    auto accvinit=accvinmul.find(catename);
    eosio_assert(accvinit!=accvinmul.end(),"错误：未发现该账户的投票分布表 ");
    accvinmul.modify(accvinit,_self,[&](auto &obj){
        obj.votetick=obj.votetick-amount;
    });
    //发送此次投票获得的奖励
    //更新类别表 - 类别所获得的投票、奖励、收益等
    cates catmul(_self,_self);
    auto catit=catmul.find(catename);
    eosio_assert(catit!=catmul.end(),"错误：未发现分类表");
    uint64_t reword=(catit->reword)*((double)amount/(double)(catit->votetick));
    catmul.modify(catit,_self,[&](auto &obj){
        obj.reword=obj.reword-reword;
        obj.votetick=obj.votetick-amount;
    });
    // 更新账户资产表
    acctickets accticmul(_self,_self);
    auto accticit=accticmul.find(accname);
    eosio_assert(accticit!=accticmul.end(),"错误：未发现账户资产表");
    accticmul.modify(accticit,_self,[&](auto &obj){
        obj.idletick=obj.idletick+amount+reword;
        obj.votetick=obj.votetick-amount;
    });
}
//赎回文章所得票
void wafyartvotes::redeemart (account_name byname,uint64_t artid,account_name catename,uint64_t amount){
    // 验证签名
    require_auth(byname);
    eosio_assert(byname==N(wafyartvotes),"错误：调用者只能为合约本身");
    // 撤回该文章的所得票
    articles artmul(_self,catename);
    auto artit=artmul.find(artid);
    eosio_assert(artit!=artmul.end(),"错误：未发现文章id");
    artmul.modify(artit,_self,[&](auto &obj){
        obj.votenum = obj.votenum-amount;
    });
}
//赎回审核者所得票
void wafyartvotes::redeemaud (account_name byname,account_name audname,account_name catename,uint64_t amount){
    // 验证签名
    require_auth(byname);
    eosio_assert(byname==N(wafyartvotes),"错误：调用者只能为合约本身");
    //撤回审核者的所得票
    auditorlists audimul(_self,catename);
    auto audiit=audimul.find(audname);
    eosio_assert(audiit!=audimul.end(),"错误：未发现该分类审核员");
    audimul.modify(audiit,_self,[&](auto &obj){
        obj.amount = obj.amount-amount;
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
    eosio_assert(accticit->idletick>votenum,"错误：投票数量超过余额");
    eosio_assert(votenum>10000,"错误：投票数量最少为1MZP");
    // 验证分类、文章id是否存在、文章是否已经过了投票期
    eosio_assert(findcate(catename)==true,"错误：该分类未定义");
    eosio_assert(findartid(artid,catename)==true,"错误：文章id不存在");
    eosio_assert(getartstat(artid,catename)==false,"错误：文章已经关闭投票");

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
        obj.idletick=obj.idletick-votenum;
        obj.votetick=obj.votetick+votenum;
    });
    // 更新项目资产表
    allticks allticmul(_self,_self);
    auto allticit=allticmul.find(_self);
    if(allticit==allticmul.end()){
        allticmul.emplace(_self,[&](auto &obj){
            obj.addtick = addtick;
            obj.devfunds  = 0;
        });
    }else{
        allticmul.modify(allticit,_self,[&](auto &obj){
            obj.addtick  = obj.addtick  + addtick;
        });
    }
    // 设置延时交易
    // 生成合约唯一id
    uint128_t voteartid=getsenderid(N(wafyvoteart));
    transaction ttrans;
    ttrans.actions.emplace_back(
        permission_level(_self,N(active)),
        N(wafyartvotes),
        N(redeemvote),
        make_tuple(_self,byname,catename,votenum));
    ttrans.actions.emplace_back(
        permission_level(_self,N(active)),
        N(wafyartvotes),
        N(redeemart),
        make_tuple(_self,artid,catename,votenum));
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
    eosio_assert(accticit->idletick>1000000,"错误：申请成为审核者需要支付100MZP，余额不足");

    auditorlists audimul(_self,catename);
    auto audiit=audimul.find(byname);
    eosio_assert(audiit==audimul.end(),"错误：已经申请过了，请勿重复申请");

    // 更新账户资产表
    accticmul.modify(accticit,_self,[&](auto &obj){
        obj.idletick=obj.idletick-1000000;
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
    eosio_assert(audiit!=audimul.end(),"错误：审核者列表中不存在该用户");

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
    eosio_assert(accticit->idletick>votenum,"错误：投票数量超过余额");
    eosio_assert(votenum>10000,"错误：投票数量最少为1MZP");
    // 验证分类、审核者是否存在
    eosio_assert(findcate(catename)==true,"错误：该分类未定义");
    eosio_assert(findaudit(auditor,catename)==true,"错误：审核者不存在");

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
    //更新审核员表 
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
        obj.idletick=obj.idletick-votenum;
        obj.votetick=obj.votetick+votenum;
    });

    // 设置延时交易
    uint128_t voteaudiid=getsenderid(N(wafyvoteaudi));
    transaction ttrans;
    ttrans.actions.emplace_back(
        permission_level(_self,N(active)),
        N(wafyartvotes),
        N(redeemvote),
        make_tuple(_self,byname,catename,votenum));
    ttrans.actions.emplace_back(
        permission_level(_self,N(active)),
        N(wafyartvotes),
        N(redeemaud),
        make_tuple(_self,auditor,catename,votenum));
    ttrans.delay_sec =VOTETIME;
    ttrans.send(voteaudiid,_self);
}
// 创建文章
void wafyartvotes::createart (account_name byname,string title,string abstract,ipfshash_t arthash,account_name catename,uint64_t payticket){
    // 校验参数
    require_auth(byname);
    eosio_assert(getaccblance(byname)>payticket,"错误：该账户没有足够的MZP发布信息");
    eosio_assert(title.size()<80,"错误：标题不超过80字节");
    eosio_assert(abstract.size()<400,"错误：摘要不超过400字节");
    eosio_assert(arthash.length()==46,"错误：文章hash长度为46字节");
    eosio_assert(findcate(catename)==true,"错误：未定义该分类");
    eosio_assert(checkaudit(catename)==true,"错误：该分类审核员还没有满员，无法创建文章");
    cates catemul(_self,_self);
    auto cateit=catemul.find(catename);
    eosio_assert(payticket>=cateit->paylimit,"错误：支付的MZP，不能小于账户的限制");

    // 更新文章表
    articles artmul(_self,catename);
    uint64_t artid=artmul.available_primary_key();
    artmul.emplace(_self,[&](auto &obj){
        obj.id=artid;
        obj.votenum=0;
        obj.addtick=0;
        obj.basetick=payticket*((cateit->comratio)/100.00);
        obj.title=title;
        obj.abstract=abstract;
        obj.author=byname;
        obj.arthash=arthash;
        obj.timestamp=now();
        obj.modifynum=0;
        obj.isend=0;
    });
    // 更新类别表
    catemul.modify(cateit,_self,[&](auto &obj){
        obj.reword=obj.reword+(uint64_t)(payticket/100.00*(cateit->voteratio));
        obj.addcatenum();
    });
    // 更新项目表
    allticks allticmul(_self,_self);
    auto allticit=allticmul.find(_self);
    if(allticit==allticmul.end()){
        allticmul.emplace(_self,[&](auto &obj){
            obj.byname=_self;
            obj.addtick=0;
            obj.devfunds  = payticket/100.00*(cateit->devratio);
        });
    }else{
        allticmul.modify(allticit,_self,[&](auto &obj){
            obj.devfunds  = obj.devfunds  + payticket/100.00*(cateit->devratio);
        });
    }
    // 更新账户资产表
    acctickets accticmul(_self,_self);
    auto accticit=accticmul.find(byname);
    accticmul.modify(accticit,_self,[&](auto &obj){
        obj.idletick=obj.idletick-payticket;
    });
    // 发放奖励给审核者
    auditorlists auditmul(_self,catename);
    uint64_t num =0;
    for( auto auditit = auditmul.begin();auditit != auditmul.end(); ++auditit){
        if(num>=(catemul.find(catename)->auditornum))
            break;
        num++;
    }
    uint64_t tempnum=0;
    uint64_t singlere=payticket/100.00*(cateit->devratio)/num;
    for( auto auditit = auditmul.begin();auditit != auditmul.end(); ++auditit){
        if(tempnum>=(catemul.find(catename)->auditornum))
            break;
        // 更新账户资产表
        auto accticit=accticmul.find(auditit->auditor);
        accticmul.modify(accticit,_self,[&](auto &obj){
            obj.idletick=obj.idletick+singlere;
        });
        tempnum++;
    }
    // 设置延迟交易
    uint128_t setbestid=getsenderid(N(wafysetbest));
    transaction ttrans;
    ttrans.actions.emplace_back(
        permission_level(_self,N(active)),
        N(wafyartvotes),
        N(setartend),
        make_tuple(_self,artid,catename));
    ttrans.delay_sec =ARTENDTIME;
    ttrans.send(setbestid,_self);
}
// 修改文章
void wafyartvotes::modifyart (account_name byname,string title,string abstract,uint64_t id,account_name catename,ipfshash_t newarthash){
    require_auth(byname);

    eosio_assert(title.size()<80,"错误：标题不超过80字节");
    eosio_assert(abstract.size()<400,"错误：摘要不超过400字节");

    eosio_assert(findcate(catename)==true,"错误：未定义该分类");
    eosio_assert(newarthash.length()==46,"错误：文章hash长度为46字节");

    articles artmul(_self,catename);
    auto artit=artmul.find(id);

    eosio_assert(artit!=artmul.end(),"错误：文章id不存在");
    eosio_assert(artit->author==byname,"错误：修改者并非作者");
    eosio_assert(artit->modifynum<10,"错误：文章修改不能超过10次");

    artmul.modify(artit,_self,[&](auto &obj){
        obj.title=title;
        obj.abstract=abstract;
        obj.arthash=newarthash;
    });
}
// 删除文章
void wafyartvotes::deleteart (account_name byname,uint64_t id,account_name catename){
    require_auth(byname);

    eosio_assert(findcate(catename)==true,"错误：未定义该分类");
    articles artmul(_self,catename);
    auto artit=artmul.find(id);

    eosio_assert(artit!=artmul.end(),"错误：文章id不存在");
    eosio_assert(artit->author==byname||getauditor(byname,catename)==true,"错误:没有权限删除文章");
    
    artmul.erase(artit);

    cates catemul(_self,_self);
    auto cateit=catemul.find(catename);

    eosio_assert(cateit!=catemul.end(),"错误：未定义该分类");
    catemul.modify(cateit,_self,[&](auto &obj){
        obj.subcatenum();
    });
}
// 创建分类
void wafyartvotes::createcate(account_name byname,account_name catename,string memo,\
uint64_t paylimit, uint64_t auditnum,uint64_t comratio ,\
uint64_t voteratio, uint64_t audiratio, uint64_t devratio ){
    require_auth(byname);

    eosio_assert(getaccblance(byname)>=5000000,"错误：该账户没有足够的MZP创建分类，创建分类需要消耗500MZP");
    eosio_assert(paylimit>=10000,"错误：分类下的文章，最少需要支付1MZP");
    eosio_assert(comratio+voteratio+audiratio+devratio==100,"错误：分成比例相加应为100");
    eosio_assert(comratio>1 && voteratio>1 && audiratio>1 && devratio>1,"错误：分成比例不能存在小于1的方式");
    eosio_assert(auditnum>=1 && auditnum <100,"错误：每个分类的审核员不能小于1位，不能超过100位");


    eosio_assert(memo.size()<400,"错误：分类简介需要小于400字节");
    eosio_assert(findcate(catename)==false,"错误：该分类已经定义过");

    cates catemul(_self,_self);
    catemul.emplace(_self,[&](auto& obj){
        obj.catename=catename;
        obj.reword=5000000;
        obj.votetick=0;
        obj.paylimit=paylimit;
        obj.comratio=comratio;
        obj.voteratio=voteratio;
        obj.audiratio=audiratio;
        obj.devratio=devratio;
        obj.auditornum=auditnum;
        obj.memo=memo;
        obj.createname=byname;
        obj.articlenum=0;
    });
    // 更新创建者账户余额
    acctickets accticmul(_self,_self);
    auto accticit=accticmul.find(byname);
    accticmul.modify(accticit,_self,[&](auto &obj){
        obj.idletick=obj.idletick-5000000;
    });
}
// 订阅分类
void wafyartvotes::createscr (account_name byname,account_name catename){
    require_auth(byname);

    eosio_assert(findcate(catename)==true,"错误：未定义该分类");

    subscribes submul(_self,byname);
    auto subit=submul.find(catename);

    eosio_assert(subit==submul.end(),"错误：该分类已经订阅过");
    submul.emplace(_self,[&](auto& obj){
        obj.catename=catename;
    });
}
// 删除订阅分类
void wafyartvotes::deletescr (account_name byname,account_name catename){
    require_auth(byname);
    eosio_assert(findcate(catename)==true,"错误：未定义该分类");
    subscribes submul(_self,byname);
    auto subit=submul.find(catename);

    eosio_assert(subit!=submul.end(),"错误：该分类尚未订阅过");

    submul.erase(subit);
}
// 创建评论
void wafyartvotes::createcom (account_name byname,string comcontent,account_name catename,uint64_t parid,uint16_t indexnum){
    require_auth(byname);

    eosio_assert(comcontent.size()<1000,"错误：评论长度不能超过1000字节");
    eosio_assert(findcate(catename)==true,"错误：未定义该分类");
    eosio_assert(indexnum==1||indexnum==2,"错误：评论等级只能为1或2");
    if(indexnum==1){
        eosio_assert(findartid(parid,catename)==true,"错误：文章id不存在");
    }else{
        eosio_assert(findcomid(parid,catename)==true,"错误：一级评论id不存在");
    }

    comments commul(_self,catename);
    commul.emplace(_self,[&](auto &obj){
        obj.id=commul.available_primary_key();
        obj.parid=parid;
        obj.author=byname;
        obj.comcontent=comcontent;
        obj.timestamp=now();
        obj.indexnum=indexnum;
        obj.isbest=0;
    });
}
// 删除评论
void wafyartvotes::deletecom (account_name byname,uint64_t id,account_name catename,uint16_t indexnum){
    require_auth(byname);

    eosio_assert(findcate(catename)==true,"错误：未定义该分类");
    eosio_assert(findcomid(id,catename)==true,"错误：评论id不存在");
    eosio_assert(indexnum==1||indexnum==2,"错误：评论等级，参数为1或者2");

    comments commul(_self,catename);
    auto comit=commul.find(id);
    eosio_assert(comit!=commul.end(),"错误：评论id不存在");
    eosio_assert(comit->author==byname||getauditor(byname,catename)==true,"错误：该账户没有权限删除评论");

    uint64_t pid=comit->parid;
    commul.erase(comit);

    if(indexnum==1){
        comments commul(_self,catename);
        auto compidx=commul.get_index<N(byparid)>();
        auto compit=compidx.find(pid);
        for(;compit!=compidx.end();){
            if(compit->parid==pid&&compit->indexnum==2){
                compit=compidx.erase(compit);
                if(compit==compidx.end())
                    break;
            }else{
                compit++;
            }
        }
    }
}
// 设置优秀评论
void wafyartvotes::setbestcom(account_name byname,uint64_t artid,uint64_t comid,account_name catename){
    require_auth(byname);

    eosio_assert(findcate(catename)==true,"错误：未定义该分类");
    eosio_assert(findartid(artid,catename)==true,"错误：文章id不存在");
    eosio_assert(findcomid(comid,catename)==true,"错误：评论id不存在");

    articles artmul(_self,catename);
    auto artit=artmul.find(artid);
    eosio_assert(artit->author==byname,"错误：非文章作者无权设置有效评论");

    comments commul(_self,catename);
    auto comit=commul.find(comid);
    eosio_assert(comit->indexnum==1,"错误：有效评论只能为一级评论");
    eosio_assert(comit->parid==artid,"错误：该评论不属于此文章");
    artmul.modify(artit,_self,[&](auto &obj){
        obj.isend=1;
    });
    commul.modify(comit,_self,[&](auto &obj){
        obj.isbest=1;
    });
    
    // 最佳评论者赚取收益
    uint64_t comreword=artit->addtick+artit->basetick;
    artmul.modify(artit,_self,[&](auto &obj){
        obj.addtick=0;
        obj.basetick=0;
    });
    account_name comautor=comit->author;
    acctickets accticmul(_self,_self);
    auto accticit=accticmul.find(comautor);
    accticmul.modify(accticit,_self,[&](auto &obj){
        obj.idletick=obj.idletick+comreword;
    });
    // 根据增发的MZP奖励，释放对应的token
     symbol_type MZSYMBOL = symbol_type(string_to_symbol(4, "MZ"));

    // inline action 
    eosio::action theAction = action(permission_level{ N(wafyartvotes), N(active) }, N(wafyarttoken), N(addtoken),
                                    std::make_tuple(N(wafyartvotes),comreword));
    theAction.send();
}
void wafyartvotes::setartend (account_name byname,uint64_t artid,account_name catename){
    require_auth(byname);
    eosio_assert(byname==N(wafyartvotes),"错误：该账户没有权限设置文章结束");

    articles artmul(_self,catename);
    auto artit=artmul.find(artid);
    artmul.modify(artit,_self,[&](auto &obj){
        obj.isend=1;
    });
}
EOSIO_ABI( wafyartvotes, (staketit)(unstaketit)(setstats)(delunstake)(voteart)(redeemvote)(redeemart)(redeemaud)(regauditor)(delauditor)\
(voteaud)(createart)(modifyart)(deleteart)(createcate)(createscr)(deletescr)(createcom)(deletecom)(setbestcom)(setartend))
