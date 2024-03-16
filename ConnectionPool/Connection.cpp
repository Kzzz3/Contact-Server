#include "Connection.h"

Connection::Connection(ConnectionPool &conn_pool)
    : _conn_pool(conn_pool)
{
    _conn = _conn_pool.GetConnection();
}

Connection::~Connection()
{
    _conn_pool.ReleaseConnection(_conn);
}

bool Connection::insert_delete_update(const char *sql)
{
    if (0 != mysql_query(_conn, sql))
    {
        printf("Mysql inser_delte_update error!");
        return false;
    }
    return true;
}

std::shared_ptr<Query_Res> Connection::query(const char *sql)
{
    int row = 0, col = 0;
    std::shared_ptr<Query_Res> ret = std::make_shared<Query_Res>();
    MYSQL_RES *result = nullptr;
    if (0 != mysql_query(_conn, sql))
    {
        printf("Mysql Query error!");
        ret->_is_success = false;
        return ret;
    }

    ret->_is_success = true;
    result = mysql_store_result(_conn);
    row = mysql_num_rows(result);
    col = mysql_num_fields(result);
    for (int i = 0; i < row; ++i)
    {
        std::vector<std::string> row_data;
        MYSQL_ROW mysql_row = mysql_fetch_row(result);
        for (int j = 0; j < col; ++j)
        {
            row_data.push_back(mysql_row[j]);
        }
        ret->_res.push_back(row_data);
    }
    if (result != nullptr)
    {
        mysql_free_result(result);
        result = nullptr;
    }

    return ret;
}