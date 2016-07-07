#ifndef __TGL_USER_H__
#define __TGL_USER_H__

#include "tgl_file_location.h"
#include "tgl_peer_id.h"

#include <memory>
#include <string>

class tgl_timer;
struct tgl_message;

enum class tgl_user_online_status: int32_t {
    unknown = 0,
    online = 1,
    offline = 2,
    recent = 3,
    last_week = 4,
    last_month
};

struct tgl_user_status {
    tgl_user_online_status online;
    int32_t when;
    tgl_user_status(): online(tgl_user_online_status::unknown), when(0) { }
};

struct tgl_user {
    tgl_input_peer_t id;
    int32_t flags;
    int64_t access_hash;
    struct tgl_user_status status;
    std::string username;
    std::string firstname;
    std::string lastname;
    std::string phone;
    tgl_user(): flags(0), access_hash(0) { }
};

#endif
