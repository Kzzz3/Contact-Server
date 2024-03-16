/**
 * @file    PacketDefinition.hpp
 * @author  XTU@Kzzz3
 * @brief   Packet Definition
 * @version 1.0
 * @date    2023-1-03
 * @copyright Copyright (c) 2022 Kzzz3
 */
#ifndef PACKETDEFINITION_HPP
#define PACKETDEFINITION_HPP

#include <cstring>
#include <memory>
#include <iostream>
#include <algorithm>
#include <boost/asio.hpp>

#define strncpy_s strncpy
//-----------------------------------------------------------------------------------------------------------------------------------
enum class Pkt_Status
{
    DEFAULT = 1,
    SUCCESSED,
    FAILED
};

enum class Pkt_Type
{
    DEFAULT = 1,
    USER_OP_LOGIN,
    USER_OP_MATCH,
    USER_OP_REGISTER,
    USER_OP_GET_LABEL,
    USER_OP_ADD_LABEL,
    USER_OP_JOIN_ROOM,
    USER_OP_LEAVE_ROOM,
    USER_OP_CREATE_ROOM,
    USER_OP_GIVE_A_LIKE,
    USER_OP_GET_HALL_INF,
    USER_OP_JOIN_ROOM_INF,
    USER_OP_LEAVE_ROOM_INF,
    USER_OP_GET_ROOM_USER_INF,
    USER_DATA_IMG,
    USER_DATA_TEXT,
    USER_DATA_AUDIO,
    USER_STATUS_ALIVE
};
//-----------------------------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push, 1)
class Packet_Inf
{
public:
    Pkt_Type _type;
    Pkt_Status _status;
    unsigned int _body_length;
    Packet_Inf()
        : _type(Pkt_Type::DEFAULT),
          _status(Pkt_Status::DEFAULT),
          _body_length(0)
    {
    }
    Packet_Inf(Pkt_Type type, Pkt_Status status, unsigned int data_length)
        : _type(type),
          _status(status),
          _body_length(data_length)
    {
    }
};
//-----------------------------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------------------------------
class User_Inf
{
public:
    char _user_id[16];
    char _user_passsword[16];
    User_Inf()
        : _user_id(""),
          _user_passsword("")
    {
    }
    User_Inf(const char *id, const char *pwd)
        : _user_id(""),
          _user_passsword("")
    {
        if (id != nullptr)
            strncpy_s(_user_id, id, sizeof(_user_id) - 1);
        if (pwd != nullptr)
            strncpy_s(_user_passsword, pwd, sizeof(_user_passsword) - 1);
    }
    void set_account(const char *id, const char *pwd)
    {
        if (id != nullptr)
            strncpy_s(_user_id, id, sizeof(_user_id) - 1);
        if (pwd != nullptr)
            strncpy_s(_user_passsword, pwd, sizeof(_user_passsword) - 1);
    }
};
//-----------------------------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------------------------------
class Packet
{
public:
    static constexpr unsigned int MAX_BODY_LENGTH = 1024  * 10 * 30;
    static constexpr unsigned int PACKET_INF_LENGTH = sizeof(Packet_Inf);
    static constexpr unsigned int USER_INF_LENGTH = sizeof(User_Inf);
    static constexpr unsigned int HEADER_LENGTH = sizeof(Packet_Inf) + sizeof(User_Inf);

public:
    Packet()
        : _pkt_inf(Pkt_Type::DEFAULT, Pkt_Status::DEFAULT, 0),
          _user_inf("", ""),
          _pdata(new char[HEADER_LENGTH])
    {
    }

    Packet(const char *id, const char *pwd,
           unsigned int data_length, Pkt_Type type = Pkt_Type::DEFAULT, Pkt_Status status = Pkt_Status::DEFAULT)
        : _pkt_inf(type, status, std::min(data_length, MAX_BODY_LENGTH)),
          _user_inf(id, pwd),
          _pdata(new char[HEADER_LENGTH + _pkt_inf._body_length])
    {
    }

    ~Packet()
    {
        delete[] _pdata;
    }

    unsigned int get_packet_total_size()
    {
        return HEADER_LENGTH + _pkt_inf._body_length;
    }

    unsigned int set_body_length(unsigned int new_length, bool copy_whether = false)
    {
        new_length = std::min(new_length, MAX_BODY_LENGTH);

        char *ptemp = new char[new_length + HEADER_LENGTH];
        if (copy_whether)
        {
            std::memcpy(ptemp, _pdata, std::min(new_length, _pkt_inf._body_length) + HEADER_LENGTH);
        }
        delete[] this->_pdata;

        _pdata = ptemp;
        _pkt_inf._body_length = new_length;
        return _pkt_inf._body_length;
    }

    char *get_pbody()
    {
        return _pdata + HEADER_LENGTH;
    }

    void encode()
    {
        std::memcpy(_pdata, this, HEADER_LENGTH);
    }

    void decode()
    {
        std::memcpy(this, this->_pdata, HEADER_LENGTH);
    }

    void show()
    {
        std::cout << static_cast<std::underlying_type<Pkt_Type>::type>(_pkt_inf._type) << " "
                  << static_cast<std::underlying_type<Pkt_Type>::type>(_pkt_inf._status) << " "
                  << _pkt_inf._body_length << " "
                  << _user_inf._user_id << " "
                  << _user_inf._user_passsword << std::endl;
    }

public:
    Packet_Inf _pkt_inf;
    User_Inf _user_inf;
    char *_pdata;
};
#pragma pack(pop)
//-----------------------------------------------------------------------------------------------------------------------------------

//bool BlockingRead(boost::asio::ip::tcp::socket* sockRemote, char* pData, int nSize)
//{
//    int nTotal = 0;
//    while (nTotal < nSize)
//    {
//		int nRead = sockRemote->read_some(boost::asio::buffer(pData + nTotal, nSize - nTotal));
//        if (nRead <= 0)
//        {
//			return false;
//		}
//		nTotal += nRead;
//	}
//    return true;
//}
//
//bool BlockingWrite(boost::asio::ip::tcp::socket* sockRemote, char* pData, int nSize)
//{
//    int nTotal = 0;
//    while (nTotal < nSize)
//    {
//		int nWrite = sockRemote->write_some(boost::asio::buffer(pData + nTotal, nSize - nTotal));
//        if (nWrite <= 0)
//        {
//			return false;
//		}
//		nTotal += nWrite;
//	}
//	return true;
//}

#endif // PACKETDEFINITION_HPP
