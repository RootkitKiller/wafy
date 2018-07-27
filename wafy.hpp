/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <string.h>

using namespace eosio;
using namespace std;

class wafy : public contract
{
  public:
      using contract::contract;
      using ipfshash_t=string;
      using cate_name_t=account_name;
  private:
      // 文章table，scope 为catename
      // @abi table articles i64
      struct article
      {
        uint64_t id;         //文章id
        string title;        //文章标题
        string abstract;     //文章摘要
        account_name author; //文章作者
        ipfshash_t arthash;  //文章保存在ipfs网络上的哈希值
        uint64_t timestamp;  //文章创建的时间戳
        uint64_t isend;      //文章是否发现价值评论

        uint64_t primary_key() const { return id; }           //主键索引
        uint64_t get_author() const { return author; }        //作者名索引
        uint64_t get_createtime() const { return timestamp; } //时间戳索引
        uint64_t get_isend() const { return isend; }          //是否完结索引

        EOSLIB_SERIALIZE(article, (id)(title)(abstract)(author)(arthash)(timestamp)(isend))
      };
      typedef multi_index<N(articles),article,
          indexed_by<N(byauthor),const_mem_fun<article,uint64_t,&article::get_author>>,
          indexed_by<N(bytime),const_mem_fun<article,uint64_t,&article::get_createtime>>,
          indexed_by<N(byisend),const_mem_fun<article,uint64_t,&article::get_isend>>
          > articles;

      // 订阅table scope为byname
      // @abi table subscribes i64
      struct subscribe
      {
        uint64_t id;          //订阅id
        cate_name_t catename; //订阅的类别名称

        uint64_t primary_key() const { return id; }    //主键索引
        uint64_t get_cate() const { return catename; } //类别名称索引

        EOSLIB_SERIALIZE(subscribe, (id)(catename))
      };
      typedef multi_index<N(subscribes),subscribe,
          indexed_by<N(bycate),const_mem_fun<subscribe,uint64_t,&subscribe::get_cate>>
          > subscribes;

      // 类别talbe scope为wafywafywafy
      // @abi table cates i64
      struct cate
      {
        uint64_t id;             //类别id
        cate_name_t catename;    //类别名称
        string memo;             //类别备注
        account_name createname; //类别创建者
        uint64_t articlenum;     //包含的文章数

        uint64_t primary_key() const { return id; }        //主键索引
        uint64_t get_cate() const { return catename; }     //类别名称索引
        uint64_t get_create() const { return createname; } //创建者索引
        uint64_t get_artnum() const { return articlenum; } //文章数量索引

        void addcatenum() { articlenum++; } //递增类别中的文章计数
        void subcatenum() { articlenum--; } //递减类别中的文章计数

        EOSLIB_SERIALIZE(cate, (id)(catename)(memo)(createname)(articlenum))
      };

      typedef multi_index<N(cates),cate,
          indexed_by<N(bycate),const_mem_fun<cate,uint64_t,&cate::get_cate>>,
          indexed_by<N(bycreate),const_mem_fun<cate,uint64_t,&cate::get_create>>,
          indexed_by<N(byartnum),const_mem_fun<cate,uint64_t,&cate::get_artnum>>
          > cates;

      // 评论table scope为catename
      // @abi table comments i64
      struct comment
      {
        uint64_t id;          //类别id
        uint64_t parid;       //父id
        account_name author;  //评论作者
        string comcontent;    //评论的内容
        uint64_t timestamp;   //评论时间戳
        uint16_t indexnum;    //评论等级（一级评论、二级评论）
        uint64_t isbest;      //是否为最优评论

        uint64_t primary_key() const { return id; }     //主键索引
        uint64_t get_parid() const { return parid; }    //父主键索引
        uint64_t get_best() const { return isbest; }    //有效评论索引

        EOSLIB_SERIALIZE(comment, (id)(parid)(author)(comcontent)(timestamp)(indexnum)(isbest))
      };
      typedef multi_index<N(comments),comment,
          indexed_by<N(byparid),const_mem_fun<comment,uint64_t,&comment::get_parid>>,
          indexed_by<N(bybest),const_mem_fun<comment,uint64_t,&comment::get_best>>
          > comments;
  public:

      // @abi action
      void createart(account_name byname,string title,string abstract,ipfshash_t arthash,cate_name_t catename);  

      // @abi action
      void modifyart(account_name byname,string title,string abstract,uint64_t id,cate_name_t catename,ipfshash_t newarthash);  

      // @abi action
      void deleteart(account_name byname,uint64_t id,cate_name_t catename); 

      // @abi action
      void createcate(account_name byname,cate_name_t catename,string memo);

      // @abi action
      void createscr(account_name byname,cate_name_t catename);

      // @abi action
      void createcom(account_name byname,string comcontent,cate_name_t catename,uint64_t parid,uint16_t indexnum);

      // @abi action
      void modifycom(account_name byname,uint64_t id,cate_name_t catename,string newcontent);

      // @abi action
      void deletecom(account_name byname,uint64_t id,cate_name_t catename,uint16_t indexnum);

      // @abi action
      void setbestcom(account_name byname,uint64_t artid,uint64_t comid,cate_name_t catename);
  
  private:
      bool findcate(cate_name_t catename){
        cates catemul(_self,_self);
        auto cateidx=catemul.get_index<N(bycate)>();
        auto cateit=cateidx.find(catename);

        if(cateit!=cateidx.end())
          return true;
        else
          return false;
      }
      bool findartid(uint64_t id,cate_name_t catename){
        articles artmul(_self,catename);
        auto artit=artmul.find(id);
        if(artit!=artmul.end())
          return true;
        else
          return false;
      }
      bool findcomid(uint64_t id,cate_name_t catename){
        comments commul(_self,catename);
        auto comit=commul.find(id);
        
        if(comit!=commul.end())
          return true;
        else
          return false;
      }
};
