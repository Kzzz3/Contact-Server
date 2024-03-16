/**
 * @file    VedioRoom.h
 * @author  XTU@Kzzz3
 * @brief   Description of VedioRoom
 * @version 1.0
 * @date    2023-1-05
 *
 * @copyright Copyright (c) 2022 Kzzz3
 */
#ifndef VIDEOROOM_H
#define VIDEOROOM_H

#include <set>
#include <memory>
#include "Client.h"
#include "../Server/PacketDefinition.hpp"

#pragma pack(push, 1)
struct VideoRoomInfo
{
    bool _room_type;
    int _room_id;
    int _room_size;
    int _room_capacity;
    char _room_name[32];
    char _room_owner_id[16];

    VideoRoomInfo(int room_id, int capacity, const char *name, const char *owner_id, bool room_type)
    :_room_type(room_type),_room_id(room_id),_room_size(0),_room_capacity(capacity)
    {
        std::strncpy(_room_name, name, sizeof(_room_name));
        std::strncpy(_room_owner_id, owner_id, sizeof(_room_owner_id));
    }
};
#pragma pack(pop)

class VideoRoom
{
public:
    static constexpr bool TEMP_ROOM = true;
    static constexpr bool PERMANENT_ROOM = false;
    static constexpr unsigned int MAX_ROOM_CAPACITY = 16;
    static constexpr unsigned int DOUBLED_ROOM_CAPACITY = 2;
    static constexpr unsigned int ROOM_INFO_LENGTH=sizeof(VideoRoomInfo);

    std::mutex _room_mutex;
    VideoRoomInfo _room_info;
    std::set<std::shared_ptr<Client>> _room_clients;

public:
    VideoRoom() = delete;
    VideoRoom(int _room_id, int capacity, const char *name, const char *owner_id, bool room_type = VideoRoom::PERMANENT_ROOM);
    ~VideoRoom();

    bool join_room(std::shared_ptr<Client> &client);
    void leave_room(std::shared_ptr<Client> &client);

    VideoRoomInfo get_room_info();
    std::vector<std::string> get_room_users_id();
};

#endif // VIDEOROOM_H