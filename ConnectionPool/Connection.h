#ifndef CONNECTION_H
#define CONNECTION_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <memory>
#include "ConnectionPool.h"

//-----------------------------------------------------------------------------------------------------------------------------------
class Query_Res
{
public:
    bool _is_success;
    std::vector<std::vector<std::string>> _res;

public:
    Query_Res() : _is_success(false) {}
    ~Query_Res() {}
};
//-----------------------------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------------------------------
class Connection
{
public:
    MYSQL *_conn;
    ConnectionPool &_conn_pool;

public:
    Connection() = delete;
    Connection(ConnectionPool &conn_pool);
    ~Connection();

    bool insert_delete_update(const char* sql);
    std::shared_ptr<Query_Res> query(const char *sql);
};
//-----------------------------------------------------------------------------------------------------------------------------------

#endif // CONNECTION_H