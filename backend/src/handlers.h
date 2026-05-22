//handlers.h - 负责将所有API路由和处理函数绑定到HTTP服务器
#pragma once
#include "httplib.h"
#include "db.h"

class Handlers{
public:
    //构造函数需要数据库对象引用和Token加密密钥
    Handlers(Database& db,const std::string& tokenSecret);

    //将本类中定义的所有API注册到服务器对象上
    void registerRoutes(httplib::Server& svr);

private:
    Database& db_;            //数据库操作对象引用
    std::string tokenSecret_; //Token密钥

    //辅助函数：验证请求中的Token是否有效，若有效则outUserId返回用户ID
    bool verifyToken(const httplib::Request& req,int& outUserId);

     //各API的具体处理函数
    void handleRegister(const httplib::Request& req,httplib::Response& res);
    void handleLogin(const httplib::Request& req,httplib::Response& res);
    void handleUserInfo(const httplib::Request& req,httplib::Response& res);

    void handleGetGoods(const httplib::Request& req,httplib::Response& res);
    void handleCreateGoods(const httplib::Request& req,httplib::Response& res);
    void handleGetGoodsById(const httplib::Request& req,httplib::Response& res);
    void handleUpdateGoods(const httplib::Request& req,httplib::Response& res);
    void handleDeleteGoods(const httplib::Request& req,httplib::Response& res);

    void handleOrder(const httplib::Request& req,httplib::Response& res);
    void handleTransactions(const httplib::Request& req,httplib::Response& res);
};