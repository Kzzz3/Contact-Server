#include "VideoRoom.h"

VideoRoom::VideoRoom(int room_id, int capacity, const char *name, const char *owner_id, bool room_type)
    : _room_info(room_id,capacity,name,owner_id,room_type)
{
}

VideoRoom::~VideoRoom()
{
}

bool VideoRoom::join_room(std::shared_ptr<Client> &client)
{
    std::lock_guard<std::mutex> lock(_room_mutex);
    if (_room_info._room_size < _room_info._room_capacity)
    {
        _room_info._room_size++;
        client->_room_id = _room_info._room_id;
        _room_clients.insert(client);
        return true;
    }
    return false;
}

void VideoRoom::leave_room(std::shared_ptr<Client> &client)
{
    std::lock_guard<std::mutex> lock(_room_mutex);
    _room_info._room_size--;
    client->_room_id = INVALID_ROOM_ID;
    _room_clients.erase(client);

    // 房主离开房间
    if (_room_info._room_size && strcmp(_room_info._room_owner_id, client->_user_id) == 0)
    {
        strncpy(_room_info._room_owner_id, _room_clients.begin()->get()->_user_id, sizeof(_room_info._room_owner_id));
    }
}

VideoRoomInfo VideoRoom::get_room_info()
{
    std::unique_lock lock(_room_mutex);
    return _room_info;
}

std::vector<std::string> VideoRoom::get_room_users_id()
{
    std::vector<std::string> users_id;
    std::unique_lock lock(_room_mutex);
    for (auto &client : _room_clients)
    {
        users_id.push_back(client->_user_id);
    }
    return users_id;
}