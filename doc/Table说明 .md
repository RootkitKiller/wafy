### Table 信息。

table数据，通过http api来获取。**获取数据库内容的唯一方式**。

示例：

```python
import requests

url = "http://api.party.tc.ink/v1/chain/get_table_rows"

payload = "{\"scope\":\"wafywafywafy\",\"code\":\"wafywafywafy\",\"table\":\"cates\" \
,\"json\":\"true\",\"index_position\":\"2\",\"key_type\":\"i64\"}"
payload1="{\"account_name\":\"gaoqihengyue\"}"
response = requests.request("POST", url, data=payload)

print(response.text)
```

详细参数信息：https://developers.eos.io/eosio-nodeos/reference#get_table_rows

Ps: "index_position"参数与"key_type"参数参考文档中未给出，表示索引方式和索引类型。例如：temptable 定义了三种索引方式，index_position = 2 则表示按照第二种索引方式返回数据。

### 定义的table（wafyartvotes合约）

名称：账户票统计信息

scope:wafyartvotes

code:  wafyartvotes

Table: acctickets

索引方式定义了两种，均是i64类型，一种按用户名索引，一种按票的数量进行索引。

```c
// scope为 _self 每个用户的票数统计
// @abi table acctickets i64
 struct accticket{
     account_name byname;
     uint64_t idletick;                      //闲置态
     uint64_t votetick;                      //投票态
     uint64_t unstaketick;                   //解锁态

     uint64_t primary_key()const { return byname;}
     uint64_t get_total()const   { return idletick;}

     EOSLIB_SERIALIZE(accticket,(byname)(idletick)(votetick)(unstaketick))
 };
typedef multi_index<N(acctickets),accticket,
indexed_by<N(bytotal),const_mem_fun<accticket,uint64_t,&accticket::get_total>>
            > acctickets;
```

名称：账户抵押的票在不同分类下的分布

Scope:account_name

code:  wafyartvotes

Table: accvinfos

索引方式定义了一种，i64类型，按分类名进行索引。

```c++
// scope为account_name  每个用户在不同分类下的投票分布
// @abi table accvinfos i64
struct accvinfo{
    account_name    catename;
    uint64_t        votetick;

    uint64_t primary_key()const { return catename;}

    EOSLIB_SERIALIZE(accvinfo,(catename)(votetick))
};
typedef multi_index<N(accvinfos),accvinfo> accvinfos;
```

名称：账户票解锁信息统计

Scope:account_name

code:  wafyartvotes

Table: accunstakes

索引方式定义了三种，均是i64类型，一种按id索引，一种解锁时间进行索引，一种按是否解锁完成进行索引。

```c++
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
```

名称：文章表

Scope:分类名

code:  wafyartvotes

Table: articles

索引方式定义了五种，均是i64类型，一种按id索引，一种按作者名进行索引，一种按时间戳进行索引，一种是按是否完结进行索引，一种是按得票数进行索引。

```c
// 文章table，scope 为catename
// @abi table articles i64
struct article
{
    uint64_t        id;                     //文章id

    uint64_t        votenum;                //文章得票数
    uint64_t        addtick;                //根据得票数增发奖励
    uint64_t        paytick;                //作者支付的金额,奖励给价值评论者的部分

    string          title;                  //文章标题
    string          abstract;               //文章摘要
    account_name    author;                 //文章作者
    ipfshash_t      arthash;                //文章保存在ipfs网络上的哈希值
    uint64_t        timestamp;              //文章创建的时间戳
    uint64_t        modifynum;              //文章修改次数，设置为不超过10次
    uint64_t        isend;                  //文章是否发现价值评论

    uint64_t primary_key() const    { return id; }              //主键索引
    uint64_t get_author() const     { return author; }          //作者名索引
    uint64_t get_createtime() const { return timestamp; }       //时间戳索引
    uint64_t get_isend() const      { return isend; }           //是否完结索引
    uint64_t get_vnum() const       { return votenum;}          //得票数索引

    EOSLIB_SERIALIZE(article, (id)(votenum)(addtick)(paytick)(title)(abstract)(author)(arthash)(timestamp)(modifynum)(isend))
};
typedef multi_index<N(articles),article,
    indexed_by<N(byauthor), const_mem_fun<article,uint64_t,&article::get_author>>,
    indexed_by<N(bytime),   const_mem_fun<article,uint64_t,&article::get_createtime>>,
    indexed_by<N(byisend),  const_mem_fun<article,uint64_t,&article::get_isend>>,
    indexed_by<N(byvnum),   const_mem_fun<article,uint64_t,&article::get_vnum>>
    > articles;
```

名称：审核员表

Scope:分类名

code:  wafyartvotes

Table: auditorlists

索引方式定义了两种，均是i64类型，一种按名称索引，一种按得票数进行索引。

```c
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
```

名称：分类表

Scope:wafyartvotes

code:  wafyartvotes

Table: cates

索引方式定义了四种，均是i64类型，一种按分类名索引，一种按得票数索引，一种按创建者进行索引，一种是按文章数进行索引。

```c
// scope 为 _self   类别所得票信息，以及类别自身信息
// @abi table cates i64
struct cate{
    account_name    catename;              //类别名称
    uint64_t        reword;                //该分类当前的收益，发放给投票者，来自于该分类的设置的比例
    uint64_t        votetick;              //该分类当前的投票数，部分奖励根据投票数分发给投票人

    uint64_t        paylimit;              //最小支付悬赏金额
    uint64_t        comratio;              //价值评论分成比例
    uint64_t        voteratio;             //投票者分成比例      
    uint64_t        audiratio;             //审核者分成比例
    uint64_t        devratio;              //项目开发资金分成比例
    uint64_t        auditornum;            //设置审核者人数

    string          memo;                  //类别备注
    account_name    createname;            //类别创建者
    uint64_t        articlenum;            //包含的文章数

    uint64_t primary_key()const { return catename;}     //主键索引
    uint64_t get_vote() const   { return votetick;}     //得票数索引    
    uint64_t get_create() const { return createname; }  //创建者索引
    uint64_t get_artnum() const { return articlenum; }  //文章数量索引

    void addcatenum() { articlenum++; } //递增类别中的文章计数
    void subcatenum() { articlenum--; } //递减类别中的文章计数

    EOSLIB_SERIALIZE(cate,(catename)(reword)(votetick)(paylimit)(comratio)(voteratio)(audiratio)(devratio)(auditornum)(memo)(createname)(articlenum))
};
typedef multi_index<N(cates),cate,
indexed_by<N(byvote),   const_mem_fun<cate,uint64_t,&cate::get_vote>>,
indexed_by<N(bycreate), const_mem_fun<cate,uint64_t,&cate::get_create>>,
indexed_by<N(byartnum), const_mem_fun<cate,uint64_t,&cate::get_artnum>>
            > cates;

```

名称：订阅表

Scope:account_name

code:  wafyartvotes

Table: subscribes

索引方式定义了两种，均是i64类型，一种按id索引，一种按分类名索引。

```c
// 订阅table scope为byname
// @abi table subscribes i64
struct subscribe
{
    account_name catename;      //订阅的类别名称

    uint64_t primary_key() const { return catename; }    //主键索引

    EOSLIB_SERIALIZE(subscribe, (catename))
};
typedef multi_index<N(subscribes),subscribe> subscribes;
        
```

名称：评论表

Scope:分类名

code:  wafyartvotes

Table: comments

索引方式定义了三种，均是i64类型，一种按评论id索引，一种按父id索引，一种是按优质评论索引。

Ps：评论只设置了两级，一级评论和二级评论。一级评论的父id是文章id，二级评论的父id是一级评论id。

```c
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
```

名称：延迟交易表（前端可以不用获取显示，合约中用的）

Scope:wafyartvotes

code:  wafyartvotes

Table: senderids

索引方式定义了一种  i64类型，按id索引。

```c
// scope为_self
// @abi table senderids i64
struct senderid{
    uint64_t id;
    uint64_t primary_key()const { return id;}

    EOSLIB_SERIALIZE(senderid,(id))
};
typedef multi_index<N(senderids),senderid> senderids;
```

名称：项目票统计信息表

Scope:wafyartvotes

code:  wafyartvotes

Table: allticks

索引方式定义了一种  i64类型，用户名索引。

```c
// scope为_self 抵押的MZP 解压中的MZP 增发的MZP 用于购买资源的MZP
// @abi table allticks i64
struct alltick{
    account_name byname;
    uint64_t addtick;                       // 该项目增发的票
    uint64_t devfunds;                      // 用作开发资金、购买ram等等

    uint64_t primary_key()const {return byname;}
    EOSLIB_SERIALIZE(alltick,(byname)(addtick)(devfunds))
};
typedef multi_index<N(allticks),alltick> allticks;
```

### 定义的table（wafyarttoken合约）

名称：账户余额表

Scope:account_name

code:  wafyarttoken

Table: accounts

索引方式定义了一种  i64类型，用户名索引。

```c
struct account
{
    asset balance;
    uint64_t primary_key() const { return balance.symbol.name(); }

};
typedef eosio::multi_index<N(accounts), account> accounts;

```

名称：代币信息表

Scope:account_name

code:  wafyarttoken

Table: stats

索引方式定义了一种  i64类型，用户名索引。

```c
struct currency_stats
{
    asset supply;
    asset max_supply;
    account_name issuer;

    uint64_t primary_key() const { return sucpply.symbol.name(); }
};  

typedef eosio::multi_index<N(stat), currency_stats> stats;

```



  

