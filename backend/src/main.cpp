//main.cpp - 程序入口：初始化数据库，启动HTTP服务器
#include <iostream>
#include "httplib.h"
#include "db.h"
#include "handlers.h"

int main(){
    //数据库文件路径：相对于backend目录运行，所以用../data/
    std::string dbPath="../data/game_trade.db";

    //创建数据库对象并建表
    Database db(dbPath);
    if(!db.initTables()){
        std::cerr<<"数据库初始化失败！"<<std::endl;
        return 1; //返回非0表示异常退出
    }
    std::cout<<"数据库已就绪: "<<dbPath<<std::endl;

    //Token加密密钥（生产环境应放在配置文件，这里写死便于学习）
    std::string tokenSecret="my_super_secret_key_for_token";

    //创建处理器，负责所有API逻辑
    Handlers handlers(db,tokenSecret);

    //创建HTTP服务器
    httplib::Server svr;

    //注册所有API路由
    handlers.registerRoutes(svr);

    //启动服务器，监听本机8080端口
    std::cout<<"服务器启动中，访问 http://localhost:8080"<<std::endl;
    svr.listen("0.0.0.0",8080);

    return 0;
}