#include "db.h"
#include <iostream>

// 打开数据库文件
Database::Database(const std::string& dbPath){
    int rc=sqlite3_open(dbPath.c_str(),&db_);
    if(rc!=SQLITE_OK){
        std::cerr<<"打开数据库失败: "<<sqlite3_errmsg(db_)<<std::endl;
        db_=nullptr;
    }
}

// 关闭数据库
Database::~Database(){
    if(db_) sqlite3_close(db_);
}

//建表：users、goods、transactions
bool Database::initTables(){
    const char* sql=R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            balance INTEGER DEFAULT 0,
            nickname TEXT DEFAULT ''
        );
        CREATE TABLE IF NOT EXISTS goods (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            seller_id INTEGER NOT NULL,
            title TEXT NOT NULL,
            description TEXT DEFAULT '',
            price INTEGER NOT NULL,
            status TEXT DEFAULT 'on_sale',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(seller_id) REFERENCES users(id)
        );
        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            buyer_id INTEGER NOT NULL,
            seller_id INTEGER NOT NULL,
            goods_id INTEGER NOT NULL,
            amount INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(buyer_id) REFERENCES users(id),
            FOREIGN KEY(seller_id) REFERENCES users(id),
            FOREIGN KEY(goods_id) REFERENCES goods(id)
        );
    )";
    char* errMsg=nullptr;
    int rc=sqlite3_exec(db_,sql,nullptr,nullptr,&errMsg);
    if(rc!=SQLITE_OK){
        std::cerr<<"建表失败: "<<errMsg<<std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

//====================用户操作====================
//插入新用户
bool Database::createUser(const std::string& username,const std::string& passwordHash,
                          const std::string& salt,const std::string& nickname){
    const char* sql="INSERT INTO users (username,password_hash,salt,balance,nickname) VALUES (?,?,?,0,?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    sqlite3_bind_text(stmt,1,username.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,2,passwordHash.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,3,salt.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,4,nickname.c_str(),-1,SQLITE_STATIC);
    int rc=sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc==SQLITE_DONE;
}

//根据用户名查询用户
User Database::getUserByUsername(const std::string& username){
    User user={0};
    const char* sql="SELECT id,username,password_hash,salt,balance,nickname FROM users WHERE username=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    sqlite3_bind_text(stmt,1,username.c_str(),-1,SQLITE_STATIC);
    if(sqlite3_step(stmt)==SQLITE_ROW){
        user.id=sqlite3_column_int(stmt,0);
        user.username=reinterpret_cast<const char*>(sqlite3_column_text(stmt,1));
        user.passwordHash=reinterpret_cast<const char*>(sqlite3_column_text(stmt,2));
        user.salt=reinterpret_cast<const char*>(sqlite3_column_text(stmt,3));
        user.balance=sqlite3_column_int(stmt,4);
        user.nickname=reinterpret_cast<const char*>(sqlite3_column_text(stmt,5));
    }
    sqlite3_finalize(stmt);
    return user;
}

//根据用户ID查询用户
User Database::getUserById(int userId){
    User user={0};
    const char* sql="SELECT id,username,password_hash,salt,balance,nickname FROM users WHERE id=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    sqlite3_bind_int(stmt,1,userId);
    if(sqlite3_step(stmt)==SQLITE_ROW){
        user.id=sqlite3_column_int(stmt,0);
        user.username=reinterpret_cast<const char*>(sqlite3_column_text(stmt,1));
        user.passwordHash=reinterpret_cast<const char*>(sqlite3_column_text(stmt,2));
        user.salt=reinterpret_cast<const char*>(sqlite3_column_text(stmt,3));
        user.balance=sqlite3_column_int(stmt,4);
        user.nickname=reinterpret_cast<const char*>(sqlite3_column_text(stmt,5));
    }
    sqlite3_finalize(stmt);
    return user;
}

//更新用户余额
bool Database::updateBalance(int userId,int newBalance){
    const char* sql="UPDATE users SET balance=? WHERE id=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    sqlite3_bind_int(stmt,1,newBalance);
    sqlite3_bind_int(stmt,2,userId);
    int rc=sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc==SQLITE_DONE;
}

//====================商品操作====================
//发布商品，返回新商品ID
int Database::createGoods(int sellerId,const std::string& title,
                          const std::string& description,int price){
    const char* sql="INSERT INTO goods (seller_id,title,description,price) VALUES (?,?,?,?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    sqlite3_bind_int(stmt,1,sellerId);
    sqlite3_bind_text(stmt,2,title.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,3,description.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_int(stmt,4,price);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (int)sqlite3_last_insert_rowid(db_);
}

//根据商品ID查详情，联表查出卖家昵称
Goods Database::getGoodsById(int goodsId){
    Goods goods={0};
    const char* sql=R"(
        SELECT g.id,g.seller_id,g.title,g.description,g.price,g.status,g.created_at,u.nickname
        FROM goods g JOIN users u ON g.seller_id=u.id
        WHERE g.id=?
    )";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    sqlite3_bind_int(stmt,1,goodsId);
    if(sqlite3_step(stmt)==SQLITE_ROW){
        goods.id=sqlite3_column_int(stmt,0);
        goods.sellerId=sqlite3_column_int(stmt,1);
        goods.title=reinterpret_cast<const char*>(sqlite3_column_text(stmt,2));
        goods.description=reinterpret_cast<const char*>(sqlite3_column_text(stmt,3)?:"");
        goods.price=sqlite3_column_int(stmt,4);
        goods.status=reinterpret_cast<const char*>(sqlite3_column_text(stmt,5));
        goods.createdAt=reinterpret_cast<const char*>(sqlite3_column_text(stmt,6)?:"");
        goods.sellerNickname=reinterpret_cast<const char*>(sqlite3_column_text(stmt,7)?:"");
    }
    sqlite3_finalize(stmt);
    return goods;
}

//获取所有在售商品
std::vector<Goods> Database::getOnSaleGoods(){
    std::vector<Goods> list;
    const char* sql=R"(
        SELECT g.id,g.seller_id,g.title,g.description,g.price,g.status,g.created_at,u.nickname
        FROM goods g JOIN users u ON g.seller_id=u.id
        WHERE g.status='on_sale' ORDER BY g.created_at DESC
    )";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    while(sqlite3_step(stmt)==SQLITE_ROW){
        Goods goods;
        goods.id=sqlite3_column_int(stmt,0);
        goods.sellerId=sqlite3_column_int(stmt,1);
        goods.title=reinterpret_cast<const char*>(sqlite3_column_text(stmt,2));
        goods.description=reinterpret_cast<const char*>(sqlite3_column_text(stmt,3)?:"");
        goods.price=sqlite3_column_int(stmt,4);
        goods.status=reinterpret_cast<const char*>(sqlite3_column_text(stmt,5));
        goods.createdAt=reinterpret_cast<const char*>(sqlite3_column_text(stmt,6)?:"");
        goods.sellerNickname=reinterpret_cast<const char*>(sqlite3_column_text(stmt,7)?:"");
        list.push_back(goods);
    }
    sqlite3_finalize(stmt);
    return list;
}

//获取某用户发布的商品
std::vector<Goods> Database::getUserGoods(int sellerId){
    std::vector<Goods> list;
    const char* sql=R"(
        SELECT g.id,g.seller_id,g.title,g.description,g.price,g.status,g.created_at,u.nickname
        FROM goods g JOIN users u ON g.seller_id=u.id
        WHERE g.seller_id=? ORDER BY g.created_at DESC
    )";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    sqlite3_bind_int(stmt,1,sellerId);
    while(sqlite3_step(stmt)==SQLITE_ROW){
        Goods goods;
        goods.id=sqlite3_column_int(stmt,0);
        goods.sellerId=sqlite3_column_int(stmt,1);
        goods.title=reinterpret_cast<const char*>(sqlite3_column_text(stmt,2));
        goods.description=reinterpret_cast<const char*>(sqlite3_column_text(stmt,3)?:"");
        goods.price=sqlite3_column_int(stmt,4);
        goods.status=reinterpret_cast<const char*>(sqlite3_column_text(stmt,5));
        goods.createdAt=reinterpret_cast<const char*>(sqlite3_column_text(stmt,6)?:"");
        goods.sellerNickname=reinterpret_cast<const char*>(sqlite3_column_text(stmt,7)?:"");
        list.push_back(goods);
    }
    sqlite3_finalize(stmt);
    return list;
}

//更新商品状态
bool Database::updateGoodsStatus(int goodsId,const std::string& status){
    const char* sql="UPDATE goods SET status=? WHERE id=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    sqlite3_bind_text(stmt,1,status.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_int(stmt,2,goodsId);
    int rc=sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc==SQLITE_DONE;
}

//更新商品信息
bool Database::updateGoods(int goodsId,const std::string& title,
                           const std::string& description,int price){
    const char* sql="UPDATE goods SET title=?,description=?,price=? WHERE id=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    sqlite3_bind_text(stmt,1,title.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,2,description.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_int(stmt,3,price);
    sqlite3_bind_int(stmt,4,goodsId);
    int rc=sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc==SQLITE_DONE;
}

//删除商品
bool Database::deleteGoods(int goodsId){
    const char* sql="DELETE FROM goods WHERE id=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    sqlite3_bind_int(stmt,1,goodsId);
    int rc=sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc==SQLITE_DONE;
}

//====================交易操作====================
//创建交易记录，返回交易ID
int Database::createTransaction(int buyerId,int sellerId,int goodsId,int amount){
    const char* sql="INSERT INTO transactions (buyer_id,seller_id,goods_id,amount) VALUES (?,?,?,?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    sqlite3_bind_int(stmt,1,buyerId);
    sqlite3_bind_int(stmt,2,sellerId);
    sqlite3_bind_int(stmt,3,goodsId);
    sqlite3_bind_int(stmt,4,amount);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (int)sqlite3_last_insert_rowid(db_);
}

//查询某用户的所有交易记录（作为买家或卖家）
std::vector<Transaction> Database::getUserTransactions(int userId){
    std::vector<Transaction> list;
    const char* sql=R"(
        SELECT id,buyer_id,seller_id,goods_id,amount,created_at
        FROM transactions
        WHERE buyer_id=? OR seller_id=?
        ORDER BY created_at DESC
    )";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,sql,-1,&stmt,nullptr);
    sqlite3_bind_int(stmt,1,userId);
    sqlite3_bind_int(stmt,2,userId);
    while(sqlite3_step(stmt)==SQLITE_ROW){
        Transaction t;
        t.id=sqlite3_column_int(stmt,0);
        t.buyerId=sqlite3_column_int(stmt,1);
        t.sellerId=sqlite3_column_int(stmt,2);
        t.goodsId=sqlite3_column_int(stmt,3);
        t.amount=sqlite3_column_int(stmt,4);
        t.createdAt=reinterpret_cast<const char*>(sqlite3_column_text(stmt,5)?:"");
        list.push_back(t);
    }
    sqlite3_finalize(stmt);
    return list;
}