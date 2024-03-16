/**
 * @file    Server.h
 * @author  XTU@Kzzz3
 * @brief   Server
 * @version 1.0
 * @date    2023-1-03
 * @copyright Copyright (c) 2022 Kzzz3
 */
#ifndef SERVER_H
#define SERVER_H
#define FMT_HEADER_ONLY

#include <boost/asio.hpp>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <map>
#include <set>
#include "../Session/VideoRoom.h"
#include "../ConnectionPool/Connection.h"
#include "../MatchScheduler/MatchScheduler.h"

class MatchScheduler;

using boost::asio::ip::tcp;
using boost::asio::ip::udp;
class Server
{
public:
    static constexpr unsigned int MAX_LABEL_NUM = 5;
    static constexpr unsigned int HALL_INF_NUM = 10;
    static constexpr unsigned int MAX_SQL_LENGTH = 1024;

    /*net*/
    u_short _listen_port;
    boost::asio::io_context _io_context;
    boost::asio::ip::tcp::acceptor _acceptor;

    /*mutex*/
    std::mutex _rooms_mutex;
    std::mutex _clients_mutex;

    /*data*/
    std::atomic<unsigned long long> _room_id;
    std::map<int, std::shared_ptr<VideoRoom>> _rooms;
    std::set<std::shared_ptr<Client>> _temp_clients;
    std::set<std::shared_ptr<Client>> _waiting_clients;
    std::set<std::shared_ptr<Client>> _online_free_clients;
    std::set<std::shared_ptr<Client>> _online_busy_clients;

    /*database*/
    ConnectionPool _conn_pool;

    /*log*/
    std::shared_ptr<spdlog::logger> _logger;

    /*match scheduler*/
    MatchScheduler *_match_scheduler;

public:
    Server(u_short port,
           const char *log_path, int max_log_size, int max_log_files,
           const char *db_ip, int db_port, const char *db_name, const char *db_user, const char *db_password, int db_maxconn, int db_timeout);
    ~Server();

    void start();
    void do_listen();
    void read_header(std::shared_ptr<Client> client);
    void read_body(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet);
    void do_write(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet);

    /*user data deal*/
    void delete_user_record(std::shared_ptr<Client> client);

    /*packet dealing*/
    static void dealing_packet(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv);
    static void deal_register(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv);
    static void deal_login(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv);
    static void deal_match(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv);
    static void deal_join_room(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv);
    static void deal_leave_room(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv);
    static void deal_create_room(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv);
    static void deal_data_send(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv);
    static void deal_get_hall_inf(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv);
    static void deal_get_room_user_inf(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv);

    /*scheduler msg dealing*/
    static void match_success(std::shared_ptr<Client> client, std::shared_ptr<Packet> client_packet,
                              std::shared_ptr<Client> client_matched, std::shared_ptr<Packet> client_matched_packet,
                              Server *serv);
};

#endif // SERVER_H