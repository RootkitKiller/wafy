#include "wafyarticles.hpp"
#include <eosiolib/eosio.hpp>

void wafyarticles::createart(account_name byname,string title,string abstract,ipfshash_t arthash,cate_name_t catename){
    // 校验参数
    require_auth(byname);

    eosio_assert(title.size()<40,"错误：标题不超过40字节");
    eosio_assert(abstract.size()<400,"错误：摘要不超过400字节");
    eosio_assert(arthash.length()==46,"错误：文章hash长度为46字节");
    eosio_assert(findcate(catename)==true,"错误：未定义该分类");

    articles artmul(_self,catename);
    artmul.emplace(_self,[&](auto &obj){
        obj.id=artmul.available_primary_key();
        obj.title=title;
        obj.abstract=abstract;
        obj.author=byname;
        obj.arthash=arthash;
        obj.timestamp=now();
        obj.isend=0;
    });

    cates catemul(_self,_self);
    auto cateidx=catemul.get_index<N(bycate)>();
    auto cateit=cateidx.find(catename);

    eosio_assert(cateit!=cateidx.end(),"错误：未定义该分类");
    cateidx.modify(cateit,_self,[&](auto &obj){
        obj.addcatenum();
    });

}
void wafyarticles::modifyart(account_name byname,string title,string abstract,uint64_t id,cate_name_t catename,ipfshash_t newarthash){
    require_auth(byname);

    eosio_assert(title.size()<40,"错误：标题不超过40字节");
    eosio_assert(abstract.size()<400,"错误：摘要不超过400字节");

    eosio_assert(findcate(catename)==true,"错误：未定义该分类");
    eosio_assert(newarthash.length()==46,"错误：文章hash长度为46字节");

    articles artmul(_self,catename);
    auto artit=artmul.find(id);

    eosio_assert(artit!=artmul.end(),"错误：文章id不存在");
    eosio_assert(artit->author==byname,"错误：修改者并非作者");

    artmul.modify(artit,_self,[&](auto &obj){
        obj.title=title;
        obj.abstract=abstract;
        obj.arthash=newarthash;
    });
}
void wafyarticles::deleteart(account_name byname,uint64_t id,cate_name_t catename){
    require_auth(byname);

    eosio_assert(findcate(catename)==true,"错误：未定义该分类");
    articles artmul(_self,catename);
    auto artit=artmul.find(id);

    eosio_assert(artit!=artmul.end(),"错误：文章id不存在");
    eosio_assert(artit->author==byname,"错误：删除者并非作者");
    
    artmul.erase(artit);

    cates catemul(_self,_self);
    auto cateidx=catemul.get_index<N(bycate)>();
    auto cateit=cateidx.find(catename);

    eosio_assert(cateit!=cateidx.end(),"错误：未定义该分类");
    cateidx.modify(cateit,_self,[&](auto &obj){
        obj.subcatenum();
    });
}
void wafyarticles::createcate(account_name byname,cate_name_t catename,string memo){
    require_auth(byname);

    eosio_assert(memo.size()<400,"错误：分类简介需要小于400字节");
    eosio_assert(findcate(catename)==false,"错误：该分类已经定义过");

    cates catemul(_self,_self);
    catemul.emplace(_self,[&](auto& obj){
        obj.id=catemul.available_primary_key();
        obj.catename=catename;
        obj.memo=memo;
        obj.createname=byname;
        obj.articlenum=0;
    });
}
void wafyarticles::createscr(account_name byname,cate_name_t catename){
    require_auth(byname);

    eosio_assert(findcate(catename)==true,"错误：未定义该分类");

    subscribes submul(_self,byname);
    auto subidx=submul.get_index<N(bycate)>();
    auto subit=subidx.find(catename);

    eosio_assert(subit==subidx.end(),"错误：该分类已经订阅过");
    submul.emplace(_self,[&](auto& obj){
        obj.id=submul.available_primary_key();
        obj.catename=catename;
    });
}
void wafyarticles::createcom(account_name byname,string comcontent,cate_name_t catename,uint64_t parid,uint16_t indexnum){
    require_auth(byname);

    eosio_assert(comcontent.size()<400,"错误：评论长度不能超过400字节");
    eosio_assert(findcate(catename)==true,"错误：未定义该分类");
    eosio_assert(findartid(parid,catename)==true,"错误：文章id不存在");
    eosio_assert(indexnum==1||indexnum==2,"错误：评论等级只能为1或2");

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
void wafyarticles::modifycom(account_name byname,uint64_t id,cate_name_t catename,string newcontent){
    require_auth(byname);

    eosio_assert(findcate(catename)==true,"错误：未定义该分类");
    eosio_assert(newcontent.size()<400,"错误：评论长度不超过400字节");
    eosio_assert(findcomid(id,catename)==true,"错误：评论id不存在");

    comments commul(_self,catename);
    auto comit=commul.find(id);
    eosio_assert(comit!=commul.end(),"错误：评论id不存在");
    eosio_assert(comit->author==byname,"错误：评论修改者不是评论作者");

    commul.modify(comit,_self,[&](auto& obj){
        obj.comcontent=newcontent;
    });
}
void wafyarticles::deletecom(account_name byname,uint64_t id,cate_name_t catename,uint16_t indexnum){
    require_auth(byname);

    eosio_assert(findcate(catename)==true,"错误：未定义该分类");
    eosio_assert(findcomid(id,catename)==true,"错误：评论id不存在");
    eosio_assert(indexnum==1||indexnum==2,"错误：评论等级，参数为1或者2");

    comments commul(_self,catename);
    auto comit=commul.find(id);
    eosio_assert(comit!=commul.end(),"错误：评论id不存在");
    eosio_assert(comit->author==byname,"错误：评论修改者不是评论作者");

    uint64_t pid=comit->parid;
    commul.erase(comit);

    if(indexnum==1){
        comments commul(_self,catename);
        auto compidx=commul.get_index<N(byparid)>();
        auto compit=compidx.find(pid);
        for(;compit!=compidx.end();compit++){
            if(compit->parid==pid&&compit->indexnum==2){
                compidx.erase(compit);
                if(compit==compidx.end())
                    break;
            }
        }
    }
    
}
void wafyarticles::setbestcom(account_name byname,uint64_t artid,uint64_t comid,cate_name_t catename){
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

    commul.modify(comit,_self,[&](auto &obj){
        obj.isbest=1;
    });
    artmul.modify(artit,_self,[&](auto &obj){
        obj.isend=1;
    });
}
EOSIO_ABI( wafyarticles, (createart)(modifyart)(deleteart)(createcate)(createscr)(createcom)(modifycom)(deletecom)(setbestcom))
