#include "handlers.h"
#include "utils.h"
#include "json.hpp"
#include <iostream>
#include <mutex>

using json=nlohmann::json;

//构造函数：初始化数据库引用和密钥
Handlers::Handlers(Database& db,const std::string& tokenSecret)
    :db_(db),tokenSecret_(tokenSecret){}

//注册所有路由
void Handlers::registerRoutes(httplib::Server& svr){
    //用户相关
    svr.Post("/api/register",[this](const auto& req,auto& res){handleRegister(req,res);});
    svr.Post("/api/login",[this](const auto& req,auto& res){handleLogin(req,res);});
    svr.Get("/api/user/info",[this](const auto& req,auto& res){handleUserInfo(req,res);});

    //商品相关
    svr.Get("/api/goods",[this](const auto& req,auto& res){handleGetGoods(req,res);});
    svr.Post("/api/goods",[this](const auto& req,auto& res){handleCreateGoods(req,res);});
    svr.Get("/api/goods/:id",[this](const auto& req,auto& res){handleGetGoodsById(req,res);});
    svr.Put("/api/goods/:id",[this](const auto& req,auto& res){handleUpdateGoods(req,res);});
    svr.Delete("/api/goods/:id",[this](const auto& req,auto& res){handleDeleteGoods(req,res);});

    //交易相关
    svr.Post("/api/order",[this](const auto& req,auto& res){handleOrder(req,res);});
    svr.Get("/api/transactions",[this](const auto& req,auto& res){handleTransactions(req,res);});
}

//====================Token验证辅助====================
bool Handlers::verifyToken(const httplib::Request& req,int& outUserId){        //->登录校验
    //从请求头Authorization中获取token（格式：Bearer <token>）
    auto auth=req.get_header_value("Authorization");
    if(auth.empty()||auth.find("Bearer ")!=0) return false;
    std::string token=auth.substr(7); //去掉"Bearer "前缀

    //解析token："哈希:userId:timestamp"
    size_t pos1=token.find(':');
    size_t pos2=token.rfind(':');
    if(pos1==std::string::npos||pos2==std::string::npos||pos1==pos2) return false;

    std::string hashPart=token.substr(0,pos1);
    std::string userIdStr=token.substr(pos1+1,pos2-pos1-1);
    std::string timeStr=token.substr(pos2+1);

    int userId=std::stoi(userIdStr);
    //重新计算哈希，看是否匹配（防止伪造）
    std::string raw=userIdStr+":"+timeStr+":"+tokenSecret_;
    if(sha256(raw)!=hashPart) return false;

    outUserId=userId;
    return true;
}

//====================注册====================
void Handlers::handleRegister(const httplib::Request& req,httplib::Response& res){
    auto body=json::parse(req.body);
    std::string username=body.value("username","");
    std::string password=body.value("password","");
    std::string nickname=body.value("nickname","");

    if(username.empty()||password.empty()){
        res.status=400;
        res.set_content(json{{"error","用户名和密码不能为空"}}.dump(),"application/json");
        return;
    }

    //检查用户名是否已存在
    if(db_.getUserByUsername(username).id!=0){
        res.status=400;
        res.set_content(json{{"error","用户名已存在"}}.dump(),"application/json");
        return;
    }

    //生成盐并计算密码哈希
    std::string salt=generateSalt();
    std::string hash=hashPassword(password,salt);

    //插入数据库
    if(db_.createUser(username,hash,salt,nickname)){
        res.set_content(json{{"msg","注册成功"}}.dump(),"application/json");
    }else{
        res.status=500;
        res.set_content(json{{"error","注册失败，数据库错误"}}.dump(),"application/json");
    }
}

//====================登录====================
void Handlers::handleLogin(const httplib::Request& req,httplib::Response& res){
    auto body=json::parse(req.body);
    std::string username=body.value("username","");
    std::string password=body.value("password","");

    User user=db_.getUserByUsername(username);
    if(user.id==0){
        res.status=400;
        res.set_content(json{{"error","用户名或密码错误"}}.dump(),"application/json");
        return;
    }

    //验证密码：用数据库里的盐重新计算哈希
    if(hashPassword(password,user.salt)!=user.passwordHash){
        res.status=400;
        res.set_content(json{{"error","用户名或密码错误"}}.dump(),"application/json");
        return;
    }

    //生成token
    std::string token=generateToken(user.id,tokenSecret_);
    json resp={
        {"msg","登录成功"},
        {"token",token},
        {"user",{{"id",user.id},{"username",user.username},{"nickname",user.nickname},{"balance",user.balance}}}
    };
    res.set_content(resp.dump(),"application/json");
}

//====================获取用户信息====================
void Handlers::handleUserInfo(const httplib::Request& req,httplib::Response& res){
    int userId;
    if(!verifyToken(req,userId)){
        res.status=401;
        res.set_content(json{{"error","未登录或token无效"}}.dump(),"application/json");
        return;
    }
    User user=db_.getUserById(userId);
    json resp={
        {"id",user.id},
        {"username",user.username},
        {"nickname",user.nickname},
        {"balance",user.balance}
    };
    res.set_content(resp.dump(),"application/json");
}

//====================获取所有在售商品====================
void Handlers::handleGetGoods(const httplib::Request& req,httplib::Response& res){
    auto list=db_.getOnSaleGoods();
    json arr=json::array();
    for(auto& g:list){
        arr.push_back({
            {"id",g.id},
            {"sellerId",g.sellerId},
            {"sellerNickname",g.sellerNickname},
            {"title",g.title},
            {"description",g.description},
            {"price",g.price},
            {"status",g.status},
            {"createdAt",g.createdAt}
        });
    }
    res.set_content(arr.dump(),"application/json");
}

//====================发布商品====================
void Handlers::handleCreateGoods(const httplib::Request& req,httplib::Response& res){
    int userId;
    if(!verifyToken(req,userId)){
        res.status=401;
        res.set_content(json{{"error","请先登录"}}.dump(),"application/json");
        return;
    }
    auto body=json::parse(req.body);
    std::string title=body.value("title","");
    std::string description=body.value("description","");
    int price=body.value("price",0);
    if(title.empty()||price<=0){
        res.status=400;
        res.set_content(json{{"error","商品名称和价格必须有效"}}.dump(),"application/json");
        return;
    }
    int goodsId=db_.createGoods(userId,title,description,price);
    res.set_content(json{{"msg","发布成功"},{"goodsId",goodsId}}.dump(),"application/json");
}

//====================获取单个商品详情====================
void Handlers::handleGetGoodsById(const httplib::Request& req,httplib::Response& res){
    int goodsId=std::stoi(req.matches[1]);
    Goods g=db_.getGoodsById(goodsId);
    if(g.id==0){
        res.status=404;
        res.set_content(json{{"error","商品不存在"}}.dump(),"application/json");
        return;
    }
    json resp={
        {"id",g.id},
        {"sellerId",g.sellerId},
        {"sellerNickname",g.sellerNickname},
        {"title",g.title},
        {"description",g.description},
        {"price",g.price},
        {"status",g.status},
        {"createdAt",g.createdAt}
    };
    res.set_content(resp.dump(),"application/json");
}

//====================修改商品====================
void Handlers::handleUpdateGoods(const httplib::Request& req,httplib::Response& res){
    int userId;
    if(!verifyToken(req,userId)){
        res.status=401;
        res.set_content(json{{"error","请先登录"}}.dump(),"application/json");
        return;
    }
    int goodsId=std::stoi(req.matches[1]);
    Goods g=db_.getGoodsById(goodsId);
    if(g.id==0||g.sellerId!=userId){
        res.status=403;
        res.set_content(json{{"error","无权操作此商品"}}.dump(),"application/json");
        return;
    }
    auto body=json::parse(req.body);
    std::string title=body.value("title",g.title);
    std::string desc=body.value("description",g.description);
    int price=body.value("price",g.price);
    db_.updateGoods(goodsId,title,desc,price);
    res.set_content(json{{"msg","修改成功"}}.dump(),"application/json");
}

//====================下架/删除商品====================
void Handlers::handleDeleteGoods(const httplib::Request& req,httplib::Response& res){
    int userId;
    if(!verifyToken(req,userId)){
        res.status=401;
        res.set_content(json{{"error","请先登录"}}.dump(),"application/json");
        return;
    }
    int goodsId=std::stoi(req.matches[1]);
    Goods g=db_.getGoodsById(goodsId);
    if(g.id==0||g.sellerId!=userId){
        res.status=403;
        res.set_content(json{{"error","无权操作"}}.dump(),"application/json");
        return;
    }
    db_.deleteGoods(goodsId);
    res.set_content(json{{"msg","删除成功"}}.dump(),"application/json");
}

//====================下单购买（核心事务）====================
static std::mutex orderMutex; //全局互斥锁，保证同一时间只有一个购买请求修改数据
void Handlers::handleOrder(const httplib::Request& req,httplib::Response& res){
    int buyerId;
    if(!verifyToken(req,buyerId)){
        res.status=401;
        res.set_content(json{{"error","请先登录"}}.dump(),"application/json");
        return;
    }
    auto body=json::parse(req.body);
    int goodsId=body.value("goods_id",0);
    if(goodsId==0){
        res.status=400;
        res.set_content(json{{"error","缺少商品ID"}}.dump(),"application/json");
        return;
    }

    std::lock_guard<std::mutex> lock(orderMutex); //加锁，防止并发导致数据不一致

    //1.查询商品
    Goods goods=db_.getGoodsById(goodsId);
    if(goods.id==0||goods.status!="on_sale"){
        res.status=400;
        res.set_content(json{{"error","商品不存在或已售出"}}.dump(),"application/json");
        return;
    }
    if(goods.sellerId==buyerId){
        res.status=400;
        res.set_content(json{{"error","不能购买自己的商品"}}.dump(),"application/json");
        return;
    }

    //2.查询买家余额
    User buyer=db_.getUserById(buyerId);
    if(buyer.balance<goods.price){
        res.status=400;
        res.set_content(json{{"error","余额不足"}}.dump(),"application/json");
        return;
    }

    //3.执行扣款、加款、更新商品状态、生成交易记录
    bool ok=true;
    ok=ok&&db_.updateBalance(buyerId,buyer.balance-goods.price);
    User seller=db_.getUserById(goods.sellerId);
    ok=ok&&db_.updateBalance(goods.sellerId,seller.balance+goods.price);
    ok=ok&&db_.updateGoodsStatus(goodsId,"sold");
    int transId=0;
    if(ok){
        transId=db_.createTransaction(buyerId,goods.sellerId,goodsId,goods.price);
        ok=(transId>0);
    }

    if(ok){
        res.set_content(json{{"msg","购买成功"},{"transactionId",transId}}.dump(),"application/json");
    }else{
        res.status=500;
        res.set_content(json{{"error","交易失败，数据库错误"}}.dump(),"application/json");
    }
}

//====================查询交易记录====================
void Handlers::handleTransactions(const httplib::Request& req,httplib::Response& res){
    int userId;
    if(!verifyToken(req,userId)){
        res.status=401;
        res.set_content(json{{"error","请先登录"}}.dump(),"application/json");
        return;
    }
    auto list=db_.getUserTransactions(userId);
    json arr=json::array();
    for(auto& t:list){
        arr.push_back({
            {"id",t.id},
            {"buyerId",t.buyerId},
            {"sellerId",t.sellerId},
            {"goodsId",t.goodsId},
            {"amount",t.amount},
            {"createdAt",t.createdAt}
        });
    }
    res.set_content(arr.dump(),"application/json");
}