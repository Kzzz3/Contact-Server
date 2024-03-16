/**
 * @file    Client.h
 * @author  XTU@Kzzz3
 * @brief   Description of Client
 * @version 1.0
 * @date    2023-1-05
 *
 * @copyright Copyright (c) 2022 Kzzz3
 */
#ifndef CLIENT_H
#define CLIENT_H

#include <boost/asio.hpp>
#include <cstring>
#include <iostream>
#define INVALID_ROOM_ID -1

using boost::asio::ip::tcp;
class Client
{
public:
    /* data */
    int _room_id;
    char _user_id[16];
    tcp::socket _socket;

public:
    Client(const char *id, tcp::socket &socket)
        : _room_id(INVALID_ROOM_ID),
          _socket(std::move(socket))
    {
        if (id != nullptr)
            std::strncpy(_user_id, id, sizeof(_user_id));
    }
    ~Client()
    {
    }
    bool operator==(const Client &client)
    {
        return std::strncmp(_user_id, client._user_id, sizeof(_user_id)) == 0;
    }
    void set_room_id(int room_id)
    {
        _room_id = room_id;
    }
    void set_user_id(const char *id)
    {
        std::strncpy(_user_id, id, sizeof(_user_id));
    }
};

#endif // CLIENT_H