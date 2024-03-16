/**
 * @file ConnectionPool.h
 * @author XTU@Kzzz3
 * @brief some database operation encapsulation
 * @version 0.1
 * @date 2022-04-13
 *
 * @copyright Copyright (c) 2022 Kzzz3
 */
#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <mysql/mysql.h>
#include <cstring>
#include <cstdio>
#include <atomic>
#include "../BlockDataStruct/BlockQueue.hpp"

/**
 * @brief 封装数据库的操作，包括连接池，数据库连接池管理，数据库操作
 *
 */
class ConnectionPool
{
public:
    int Port;                        // 端口
    unsigned int TimeOut;            // 超时时间
    char Ip[16];                     // 数据库ip
    char DBName[16];                 // 数据库名字
    char UserName[16];               // 数据库登录用户名
    char UserPassword[16];           // 数据库用户密码
    std::atomic<int> MaxConnect;     // 最大连接数
    std::atomic<int> FreeConnect;    // 空闲连接数
    std::atomic<int> WorkingConnect; // 正在使用的连接数
    BlockQueue<MYSQL *> ConnectPool; // 连接池

public:
    ConnectionPool() = delete;
    ConnectionPool(const char *ip, int port, const char *db_name, const char *user_name, const char *password, int maxconn, int timeout = 5);
    ~ConnectionPool();

    MYSQL *GetConnection();
    void ReleaseConnection(MYSQL *conn);
};

#endif // CONNECTIONPOOL_H