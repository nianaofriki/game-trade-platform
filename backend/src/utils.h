#pragma once
#include <string>
std::string sha256(const std::string& input);
std::string hashPassword(const std::string& password, const std::string& salt);
std::string generateSalt(int length = 16);
std::string generateToken(int userId, const std::string& secret);