#include "ConnectionPool.h"

/**
 * @brief Construct a new  ConnectionPool:: ConnectionPool object
 *
 * @param ip            连接的数据库ip
 * @param port          连接数据库的端口
 * @param db_name       数据库名字
 * @param user_name     用户名
 * @param password      用户密码
 * @param maxconn       线程池最大连接数
 * @param timeout       超时时间
 */
ConnectionPool::ConnectionPool(const char *ip, int port, const char *db_name, const char *user_name, const char *password, int maxconn, int timeout)
{
    strncpy(this->Ip, ip, sizeof(this->Ip));
    strncpy(this->DBName, db_name, sizeof(this->DBName));
    strncpy(this->UserName, user_name, sizeof(this->UserName));
    strncpy(this->UserPassword, password, sizeof(this->UserPassword));
    this->Port = port;
    this->TimeOut = timeout;
    this->WorkingConnect = 0;
    this->MaxConnect = maxconn;

    for (int i = 0; i < maxconn; ++i)
    {
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);
        conn = mysql_real_connect(
            conn,
            this->Ip,
            this->UserName,
            this->UserPassword,
            this->DBName,
            this->Port,
            nullptr,
            0);
        if (conn == nullptr)
        {
            printf("ConnectionPool ConnectPool init timeout!");
            exit(1);
        }

        ++this->FreeConnect;
        this->ConnectPool.push(conn);
    }

    printf("database initialize successed!\n");
}

/**
 * @brief Destroy the  Data Base:: Data Base object
 *
 */
ConnectionPool::~ConnectionPool()
{
    MYSQL *conn;
    while (this->ConnectPool.size())
    {
        conn = this->ConnectPool.pop();
        mysql_close(conn);
    }
}

/**
 * @brief 从连接池中获取一个连接
 *
 * @return MYSQL* 返回的连接句柄
 */
MYSQL *ConnectionPool::GetConnection()
{
    MYSQL *conn = nullptr;

    while (conn == nullptr)
    {
        conn = this->ConnectPool.pop();

        --this->FreeConnect;
        ++this->WorkingConnect;
    }
    return conn;
}

/**
 * @brief       释放一个连接，即将改连接加入到连接池中
 *
 * @param conn  连接句柄
 */
void ConnectionPool::ReleaseConnection(MYSQL *conn)
{
    if (conn == nullptr)
        return;
    this->ConnectPool.push(conn);

    ++this->FreeConnect;
    --this->WorkingConnect;
}