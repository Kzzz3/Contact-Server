#include "Server.h"

void Server::dealing_packet(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv)
{
    switch (packet->_pkt_inf._type)
    {
    case Pkt_Type::USER_OP_REGISTER:
        serv->_logger->info("Server dealing a register packet");
        deal_register(client, packet, serv);
        break;

    case Pkt_Type::USER_OP_LOGIN:
        serv->_logger->info("Server dealing a login packet");
        deal_login(client, packet, serv);
        break;

    case Pkt_Type::USER_OP_MATCH:
        serv->_logger->info("Server dealing a match packet");
        deal_match(client, packet, serv);
        break;

    case Pkt_Type::USER_OP_JOIN_ROOM:
        serv->_logger->info("Server dealing a join room packet");
        deal_join_room(client, packet, serv);
        break;

    case Pkt_Type::USER_OP_LEAVE_ROOM:
        serv->_logger->info("Server dealing a leave room packet");
        deal_leave_room(client, packet, serv);
        break;

    case Pkt_Type::USER_DATA_IMG:
        serv->_logger->info("Server dealing a image packet");
        deal_data_send(client, packet, serv);
        break;

    case Pkt_Type::USER_DATA_AUDIO:
        serv->_logger->info("Server dealing a audio packet");
        deal_data_send(client, packet, serv);
        break;

    case Pkt_Type::USER_OP_CREATE_ROOM:
        serv->_logger->info("Server dealing a create room packet");
        deal_create_room(client, packet, serv);
        break;

    case Pkt_Type::USER_OP_GET_HALL_INF:
        serv->_logger->info("Server dealing a get hall information packet");
        deal_get_hall_inf(client, packet, serv);
        break;

    default:
        serv->_logger->error("Server dealing a unknown packet");
        client->_socket.close();
        {
            std::lock_guard<std::mutex> lock(serv->_clients_mutex);
            serv->_waiting_clients.erase(client);
        }
        break;
    }
}

void Server::deal_register(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv)
{
    char sql[MAX_SQL_LENGTH] = "";
    Connection conn(serv->_conn_pool);
    std::shared_ptr<Packet> reply = std::make_shared<Packet>(packet->_user_inf._user_id, nullptr,
                                                             0, Pkt_Type::USER_OP_REGISTER, Pkt_Status::DEFAULT);
    sprintf(sql, "INSERT INTO user_table VALUES('%s', '%s')",
            packet->_user_inf._user_id,
            packet->_user_inf._user_passsword);

    bool bret = conn.insert_delete_update(sql);
    if (!bret)
    {
        serv->_logger->error("Server add a user failed");
        reply->_pkt_inf._status = Pkt_Status::FAILED;
    }
    else
    {
        serv->_logger->info("Server add a user success");
        reply->_pkt_inf._status = Pkt_Status::SUCCESSED;
    }

    serv->do_write(client, reply);
}

void Server::deal_login(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv)
{
    char sql[MAX_SQL_LENGTH] = "";
    Connection conn(serv->_conn_pool);
    std::shared_ptr<Packet> reply = std::make_shared<Packet>(packet->_user_inf._user_id, nullptr, 0,
                                                             Pkt_Type::USER_OP_LOGIN, Pkt_Status::DEFAULT);
    sprintf(sql, "SELECT * FROM user_table WHERE user_id='%s' and user_pwd='%s'",
            packet->_user_inf._user_id,
            packet->_user_inf._user_passsword);

    std::shared_ptr<Query_Res> res = conn.query(sql);
    if (!res->_is_success)
    {
        serv->_logger->warn("Server query a user error");
        reply->_pkt_inf._status = Pkt_Status::FAILED;
    }
    else
    {
        if (res->_res.empty())
        {
            serv->_logger->warn("Server query a user failed");
            reply->_pkt_inf._status = Pkt_Status::FAILED;
        }
        else
        {
            serv->_logger->info("Server query a user success");
            reply->_pkt_inf._status = Pkt_Status::SUCCESSED;

            // 将用户加入在线用户
            {
                char time_str[64];
                std::time_t now = std::time(nullptr);
                std::tm *local_time = std::localtime(&now);
                std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);

                sprintf(sql, "insert into login_record values('%s','%s','%s',%d);",
                        time_str,
                        packet->_user_inf._user_id,
                        client->_socket.remote_endpoint().address().to_string().c_str(),
                        client->_socket.remote_endpoint().port());
                conn.insert_delete_update(sql);

                std::lock_guard<std::mutex> lock(serv->_clients_mutex);
                memcpy(client->_user_id, packet->_user_inf._user_id, sizeof(client->_user_id));
                serv->delete_user_record(client);
                serv->_online_free_clients.insert(client);
            }
        }
    }

    serv->do_write(client, reply);
}

void Server::deal_match(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv)
{
    serv->_match_scheduler->AddMatchClient(client, packet);
}

void Server::deal_join_room(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv)
{
    int dst_room_id = *(reinterpret_cast<int *>(packet->get_pbody()));

    serv->_rooms_mutex.lock();
    if (serv->_rooms.find(dst_room_id) == serv->_rooms.end())
    {
        serv->_rooms_mutex.unlock();
        serv->_logger->warn("Server join a room failed");

        // 发送加入房间失败包
        std::shared_ptr<Packet> reply = std::make_shared<Packet>(client->_user_id, nullptr, 0,
                                                                 Pkt_Type::USER_OP_JOIN_ROOM, Pkt_Status::FAILED);
        serv->do_write(client, reply);
        return;
    }
    std::shared_ptr<VideoRoom> room = serv->_rooms[dst_room_id];
    serv->_rooms_mutex.unlock();

    // 将用户加入房间
    if (room->join_room(client))
    {
        // 将用户从空闲列表中删除，加入忙碌列表
        {
            std::lock_guard<std::mutex> lock(serv->_clients_mutex);
            serv->delete_user_record(client);
            serv->_online_busy_clients.insert(client);
        }

        char time_str[64];
        char sql[MAX_SQL_LENGTH] = "";
        Connection conn(serv->_conn_pool);
        std::time_t now = std::time(nullptr);
        std::tm *local_time = std::localtime(&now);
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);

        sprintf(sql, "insert into join_room_record values('%s','%s',%d,'%s');",
                time_str,
                client->_user_id,
                room->_room_info._room_id,
                room->_room_info._room_name);
        conn.insert_delete_update(sql);

        // 发送加入房间成功包
        room->_room_mutex.lock();
        std::shared_ptr<Packet> reply = std::make_shared<Packet>(client->_user_id, nullptr, room->_room_info._room_size * sizeof(User_Inf::_user_id),
                                                                 Pkt_Type::USER_OP_JOIN_ROOM, Pkt_Status::SUCCESSED);
        room->_room_mutex.unlock();
        auto users_id = room->get_room_users_id();
        for (int i = 0; i < users_id.size(); i++)
        {
            memcpy(reply->get_pbody() + i * sizeof(User_Inf::_user_id), users_id[i].data(), sizeof(User_Inf::_user_id));
        }
        serv->do_write(client, reply);

        // 发送房间内其他用户加入房间包
        std::shared_ptr<Packet> notify = std::make_shared<Packet>(client->_user_id, nullptr, sizeof(User_Inf::_user_id),
                                                                  Pkt_Type::USER_OP_JOIN_ROOM_INF, Pkt_Status::SUCCESSED);
        memcpy(notify->get_pbody(), client->_user_id, sizeof(User_Inf::_user_id));

        room->_room_mutex.lock();
        for (auto other_client : room->_room_clients)
        {
            if (other_client != client)
            {
                serv->do_write(other_client, notify);
            }
        }
        room->_room_mutex.unlock();
    }
    else
    {
        serv->_logger->warn("Server join a room failed");

        // 发送加入房间失败包
        std::shared_ptr<Packet> reply = std::make_shared<Packet>(client->_user_id, nullptr, 0,
                                                                 Pkt_Type::USER_OP_JOIN_ROOM, Pkt_Status::FAILED);
        serv->do_write(client, reply);
        return;
    }
}

void Server::deal_leave_room(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv)
{
    serv->_rooms_mutex.lock();
    int room_id = client->_room_id;
    if (room_id == INVALID_ROOM_ID)
    {
        serv->_rooms_mutex.unlock();
        return;
    }
    std::shared_ptr<VideoRoom> room = serv->_rooms[room_id];
    serv->_rooms_mutex.unlock();

    room->leave_room(client);
    {
        std::lock_guard<std::mutex> lock(serv->_clients_mutex);
        serv->delete_user_record(client);
        serv->_online_free_clients.insert(client);
    }
    std::shared_ptr<Packet> reply = std::make_shared<Packet>(client->_user_id, nullptr, 0,
                                                             Pkt_Type::USER_OP_LEAVE_ROOM, Pkt_Status::SUCCESSED);
    serv->do_write(client, reply);

    if (room->_room_info._room_type == VideoRoom::TEMP_ROOM)
    {
        std::shared_ptr<Client> other_client = *(room->_room_clients.begin());
        room->leave_room(other_client);
        {
            std::lock_guard<std::mutex> lock(serv->_clients_mutex);
            serv->delete_user_record(other_client);
            serv->_online_free_clients.insert(other_client);
        }
        std::shared_ptr<Packet> reply = std::make_shared<Packet>(other_client->_user_id, nullptr, 0,
                                                                 Pkt_Type::USER_OP_LEAVE_ROOM, Pkt_Status::SUCCESSED);
        serv->do_write(other_client, reply);
        {
            std::lock_guard<std::mutex> lock(serv->_rooms_mutex);
            serv->_rooms.erase(room_id);
        }
    }
    else if (room->_room_info._room_type == VideoRoom::PERMANENT_ROOM)
    {
        room->_room_mutex.lock();
        if (room->_room_clients.size() == 0)
        {
            std::lock_guard<std::mutex> lock(serv->_rooms_mutex);
            serv->_rooms.erase(room_id);
        }
        for (auto other_client : room->_room_clients)
        {
            std::shared_ptr<Packet> notify = std::make_shared<Packet>(client->_user_id, nullptr, sizeof(User_Inf::_user_id),
                                                                      Pkt_Type::USER_OP_LEAVE_ROOM_INF, Pkt_Status::SUCCESSED);
            memcpy(notify->get_pbody(), client->_user_id, sizeof(User_Inf::_user_id));
            serv->do_write(other_client, notify);
        }
        room->_room_mutex.unlock();
    }
}

void Server::deal_create_room(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv)
{
    // 创建一个房间
    std::shared_ptr<VideoRoom> room = std::make_shared<VideoRoom>(++serv->_room_id, VideoRoom::MAX_ROOM_CAPACITY, packet->get_pbody(), client->_user_id, VideoRoom::PERMANENT_ROOM);

    // 将房间加入房间列表
    {
        std::lock_guard<std::mutex> lock(serv->_rooms_mutex);
        serv->_rooms[room->_room_info._room_id] = room;
    }
    // 将两个用户加入房间列表,并从空闲列表中删除，加入忙碌列表
    {
        std::lock_guard<std::mutex> lock(serv->_clients_mutex);
        serv->delete_user_record(client);
        serv->_online_busy_clients.insert(client);
    }
    // 将两个用户加入房间
    room->join_room(client);

    char time_str[64];
    char sql[MAX_SQL_LENGTH] = "";
    Connection conn(serv->_conn_pool);
    std::time_t now = std::time(nullptr);
    std::tm *local_time = std::localtime(&now);
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);

    sprintf(sql, "insert into create_room_record values('%s',%d,'%s','%s');",
            time_str,
            room->_room_info._room_id,
            room->_room_info._room_name,
            client->_user_id);
    conn.insert_delete_update(sql);
    sprintf(sql, "insert into join_room_record values('%s','%s',%d,'%s');",
            time_str,
            client->_user_id,
            room->_room_info._room_id,
            room->_room_info._room_name);
    conn.insert_delete_update(sql);

    // 发送匹配成功包
    std::shared_ptr<Packet> replyone = std::make_shared<Packet>(client->_user_id, nullptr, sizeof(User_Inf::_user_id),
                                                                Pkt_Type::USER_OP_JOIN_ROOM, Pkt_Status::SUCCESSED);
    memcpy(replyone->get_pbody(), client->_user_id, sizeof(User_Inf::_user_id));
    serv->do_write(client, replyone);
}

void Server::deal_data_send(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv)
{
    int room_id = client->_room_id;
    if (room_id == INVALID_ROOM_ID)
    {
        return;
    }

    serv->_rooms_mutex.lock();
    std::shared_ptr<VideoRoom> room = serv->_rooms[room_id];
    serv->_rooms_mutex.unlock();

    room->_room_mutex.lock();
    for (auto it = room->_room_clients.begin(); it != room->_room_clients.end(); it++)
    {
        if ((*it)->_user_id == client->_user_id)
            continue;
        serv->do_write(*it, packet);
    }
    room->_room_mutex.unlock();
}

void Server::deal_get_hall_inf(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv)
{
    // 遍历房间列表，将房间信息发送给客户端
    std::unique_lock lock(serv->_rooms_mutex);

    int nCount = 0;
    int nNum = std::min((size_t)Server::HALL_INF_NUM, serv->_rooms.size());
    std::shared_ptr<Packet> reply = std::make_shared<Packet>(client->_user_id, nullptr, VideoRoom::ROOM_INFO_LENGTH * nNum,
                                                             Pkt_Type::USER_OP_GET_HALL_INF, Pkt_Status::SUCCESSED);
    printf("%d\n", nNum);
    for (auto &[_, room] : serv->_rooms)
    {
        if (nNum == 0)
            break;

        VideoRoomInfo tempRoomInfo = room->get_room_info();
        memcpy(reply->get_pbody() + nCount * VideoRoom::ROOM_INFO_LENGTH, &tempRoomInfo, VideoRoom::ROOM_INFO_LENGTH);

        nNum--;
        nCount++;
    }
    serv->do_write(client, reply);
}

void Server::deal_get_room_user_inf(std::shared_ptr<Client> client, std::shared_ptr<Packet> packet, Server *serv)
{
    int room_id = client->_room_id;
    if (room_id == INVALID_ROOM_ID)
    {
        return;
    }

    serv->_rooms_mutex.lock();
    std::shared_ptr<VideoRoom> room = serv->_rooms[room_id];
    serv->_rooms_mutex.unlock();

    std::vector<std::string> users_id = room->get_room_users_id();
    std::shared_ptr<Packet> reply = std::make_shared<Packet>(client->_user_id, nullptr, 0,
                                                             Pkt_Type::USER_OP_GET_ROOM_USER_INF, Pkt_Status::SUCCESSED);
    reply->set_body_length(users_id.size() * sizeof(Client::_user_id));
    for (int i = 0; i < users_id.size(); i++)
    {
        memcpy(reply->get_pbody() + i * sizeof(Client::_user_id), users_id[i].c_str(), sizeof(Client::_user_id));
    }
    serv->do_write(client, reply);
}

void Server::match_success(std::shared_ptr<Client> client, std::shared_ptr<Packet> client_packet,
                            std::shared_ptr<Client> matched_client, std::shared_ptr<Packet> matched_client_packet,
                            Server *serv)
{
    // 创建一个房间
    std::shared_ptr<VideoRoom> room = std::make_shared<VideoRoom>(++serv->_room_id, VideoRoom::DOUBLED_ROOM_CAPACITY, "", client->_user_id, VideoRoom::TEMP_ROOM);

    // 将房间加入房间列表
    {
        std::lock_guard<std::mutex> lock(serv->_rooms_mutex);
        serv->_rooms[room->_room_info._room_id] = room;
    }
    // 将两个用户加入房间列表,并从空闲列表中删除，加入忙碌列表
    {
        std::lock_guard<std::mutex> lock(serv->_clients_mutex);
        serv->delete_user_record(client);
        serv->delete_user_record(matched_client);

        serv->_online_busy_clients.insert(client);
        serv->_online_busy_clients.insert(matched_client);
    }
    // 将两个用户加入房间
    room->join_room(client);
    room->join_room(matched_client);

    char time_str[64];
    char sql[MAX_SQL_LENGTH] = "";
    Connection conn(serv->_conn_pool);
    std::time_t now = std::time(nullptr);
    std::tm *local_time = std::localtime(&now);
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);

    sprintf(sql, "insert into match_record values('%s','%s','%s','%s','%s');",
            time_str,
            client->_user_id,
            client_packet->get_pbody(),
            matched_client->_user_id,
            matched_client_packet->get_pbody());
    conn.insert_delete_update(sql);

    // 发送匹配成功包
    std::shared_ptr<Packet> replyone = std::make_shared<Packet>(client->_user_id, nullptr, room->_room_info._room_size * sizeof(User_Inf::_user_id),
                                                                Pkt_Type::USER_OP_MATCH, Pkt_Status::SUCCESSED);
    memcpy(replyone->get_pbody(), client->_user_id, sizeof(User_Inf::_user_id));
    memcpy(replyone->get_pbody() + sizeof(User_Inf::_user_id), matched_client->_user_id, sizeof(User_Inf::_user_id));
    serv->do_write(client, replyone);

    std::shared_ptr<Packet> replytwo = std::make_shared<Packet>(matched_client->_user_id, nullptr, room->_room_info._room_size * sizeof(User_Inf::_user_id),
                                                                Pkt_Type::USER_OP_MATCH, Pkt_Status::SUCCESSED);
    memcpy(replytwo->get_pbody(), matched_client->_user_id, sizeof(User_Inf::_user_id));
    memcpy(replytwo->get_pbody() + sizeof(User_Inf::_user_id), client->_user_id, sizeof(User_Inf::_user_id));
    serv->do_write(matched_client, replytwo);
}