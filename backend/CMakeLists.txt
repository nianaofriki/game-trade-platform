cmake_minimum_required(VERSION 3.10)
project(GameTradeBackend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#这一行解决 httplib 的 Windows 版本检查
add_definitions(-D_WIN32_WINNT=0x0A00)

set(OPENSSL_DIR "E:/tool/OpenSSL/OpenSSL-Win64")

add_executable(server
    src/main.cpp
    src/utils.cpp
    src/db.cpp
    src/handlers.cpp
    include/sqlite3.c
)

target_include_directories(server PRIVATE
    src
    include
    "${OPENSSL_DIR}/include"
)
target_link_directories(server PRIVATE "${OPENSSL_DIR}/lib/VC/x64/MT")
target_link_libraries(server
    libcrypto.lib
    libssl.lib
    ws2_32
    crypt32
)