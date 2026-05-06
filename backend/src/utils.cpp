#include "utils.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>
#include <openssl/sha.h>
std::string sha256(const std::string& input){
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, input.c_str(), input.size());
    SHA256_Final(hash, &ctx);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string hashPassword(const std::string& password, const std::string& salt) {
    return sha256(password + salt);
}

std::string generateSalt(int length) {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    std::string salt;
    for (int i = 0; i < length; i++) {
        salt += chars[dis(gen)];
    }
    return salt;
}
std::string generateToken(int userId, const std::string& secret) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    std::string raw = std::to_string(userId) + ":" + std::to_string(timestamp) + ":" + secret;
    return sha256(raw) + ":" + std::to_string(userId) + ":" + std::to_string(timestamp);
}