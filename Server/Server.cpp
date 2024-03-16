#include "Server.h"

Server::Server(u_short port,
               const char *log_path, int max_log_size, int max_log_files,
               const char *db_ip, int db_port, const char *db_name, const char *db_user, const char *db_password, int db_maxconn, int db_timeout)
    : _match_scheduler(new MatchScheduler(this)),
    _listen_port(port),
      _acceptor(_io_context, tcp::endpoint(tcp::v4(), port)),
      _conn_pool(db_ip, db_port, db_name, db_user, db_password, db_maxconn, db_timeout)
{
    // log ready
    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_path, max_log_size, max_log_files);
    spdlog::init_thread_pool(5000, 2);
    this->_logger = std::make_shared<spdlog::async_logger>("server", rotating_sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    this->_logger->set_level(spdlog::level::trace);
    this->_logger->flush_on(spdlog::level::trace);
    this->_logger->info("Server start");
}

Server::~Server()
{
    this->_logger->info("Server stop");
}

void Server::start()
{
    // 开始监听
    this->do_listen();

    // 开始运行io事件循环
    this->_io_context.run();
}

void Server::do_listen()
{
    this->_logger->info("Server start listening");
    this->_acceptor.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket client_socket)
        {
            if (!ec)
            {
                this->_logger->info("Server accept a client");
                std::shared_ptr<Client> client = std::make_shared<Client>(nullptr, client_socket);
                {
                    std::lock_guard<std::mutex> lock(this->_clients_mutex);
                    this->_temp_clients.insert(client);
                }
                this->read_header(client);
            }
            else
            {
                perror("listen error");
                this->_logger->error("Server accept a client failed");
            }
            this->do_listen();
        });
}

void Server::read_header(std::shared_ptr<Client> client)
{
    std::shared_ptr<Packet> packet = std::make_shared<Packet>();
    boost::asio::async_read(client->_socket,
                            boost::asio::buffer(&(*packet), Packet::HEADER_LENGTH),
                            [this, client, packet](boost::system::error_code ec, std::size_t length)
                            {
                                if (!ec)
                                {
                                    this->_logger->info("Server read a packet header");
                                    packet->show();
                                    this->read_body(client, packet);
                                }
                                else
                                {
                                    this->_logger->error("Server read a packet header failed,error msg{}", ec.message());
                                    client->_socket.close();

                                    {
                                        std::lock_guard<std::mutex> lock(this->_clients_mutex);
                                        this->delete_user_record(client);
                                    }

                                    if(client->_room_id!=INVALID_ROOM_ID)
                                    {
                                        this->_io_context.post(std::bind(deal_leave_room,client,nullptr,this));
                                    }
                                    this->_match_scheduler->RemoveMatchClient(client);
                                }
                            });
}

void Server::read_body(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet)
{
    if (packet->_pkt_inf._body_length == 0)
    {
        this->_io_context.post(std::bind(dealing_packet, client, packet, this));
        this->read_header(client);
        return;
    }

    // 重置包体大小及开辟空间
    packet->set_body_length(packet->_pkt_inf._body_length);
    boost::asio::async_read(client->_socket,
                            boost::asio::buffer(packet->get_pbody(), packet->_pkt_inf._body_length),
                            [this, client, packet](boost::system::error_code ec, std::size_t length)
                            {
                                if (!ec)
                                {
                                    this->_logger->info("Server read a packet body");
                                    this->_io_context.post(std::bind(dealing_packet, client, packet, this));
                                    this->read_header(client);
                                }
                                else
                                {
                                    this->_logger->error("Server read a packet body failed");
                                    client->_socket.close();
                                    {
                                        std::lock_guard<std::mutex> lock(this->_clients_mutex);
                                        this->delete_user_record(client);
                                    }

                                    if(client->_room_id!=INVALID_ROOM_ID)
                                    {
                                        this->_io_context.post(std::bind(deal_leave_room,client,nullptr,this));
                                    }
                                    this->_match_scheduler->RemoveMatchClient(client);
                                }
                            });
}

void Server::do_write(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet)
{
    packet->encode();
    boost::asio::async_write(client->_socket,
                             boost::asio::buffer(packet->_pdata, packet->_pkt_inf._body_length + Packet::HEADER_LENGTH),
                             [this, client, packet](boost::system::error_code ec, std::size_t length)
                             {
                                 if (!ec)
                                 {
                                     this->_logger->info("Server write a packet,user_id {},packet type {},packet length {},acutal length {}",
                                                         client->_user_id,
                                                         static_cast<int>(packet->_pkt_inf._type),
                                                         static_cast<int>(packet->get_packet_total_size()),
                                                         length);
                                 }
                                 else
                                 {
                                     this->_logger->warn("Server write a packet failed");
                                     client->_socket.close();
                                     {
                                         std::lock_guard<std::mutex> lock(this->_clients_mutex);
                                         this->delete_user_record(client);
                                     }
                                 }
                             });
}

void Server::delete_user_record(std::shared_ptr<Client> client)
{
    if (this->_temp_clients.find(client) != this->_temp_clients.end())
        this->_temp_clients.erase(client);
    if (this->_waiting_clients.find(client) != this->_waiting_clients.end())
        this->_waiting_clients.erase(client);
    if (this->_online_free_clients.find(client) != this->_online_free_clients.end())
        this->_online_free_clients.erase(client);
    if (this->_online_busy_clients.find(client) != this->_online_busy_clients.end())
        this->_online_busy_clients.erase(client);
}