// db.h
#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>

struct User {
    int id;
    std::string username;
    std::string passwordHash;
    std::string salt;
    int balance;
    std::string nickname;
};

struct Goods {
    int id;
    int sellerId;
    std::string title;
    std::string description;
    int price;
    std::string status;
    std::string createdAt;
    std::string sellerNickname;
};

struct Transaction {
    int id;
    int buyerId;
    int sellerId;
    int goodsId;
    int amount;
    std::string createdAt;
};

class Database {
public:
    Database(const std::string& dbPath);
    ~Database();
    bool initTables();

    bool createUser(const std::string& username, const std::string& passwordHash,
                    const std::string& salt, const std::string& nickname);
    User getUserByUsername(const std::string& username);
    User getUserById(int userId);
    bool updateBalance(int userId, int newBalance);

    int createGoods(int sellerId, const std::string& title,
                    const std::string& description, int price);
    Goods getGoodsById(int goodsId);
    std::vector<<Goods> getOnSaleGoods();
    std::vector<<Goods> getUserGoods(int sellerId);
    bool updateGoodsStatus(int goodsId, const std::string& status);
    bool updateGoods(int goodsId, const std::string& title,
                     const std::string& description, int price);
    bool deleteGoods(int goodsId);

    int createTransaction(int buyerId, int sellerId, int goodsId, int amount);
    std::vector<<Transaction> getUserTransactions(int userId);

private:
    sqlite3* db_;
};