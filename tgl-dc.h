/*
    This file is part of tgl-library

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Copyright Vitaly Valtman 2014-2015
    Copyright Topology LP 2016
*/
#ifndef __TGL_DC_H__
#define __TGL_DC_H__

#include <array>
#include <cassert>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "types/tgl_peer_id.h"

enum class tgl_dc_state {
    init,
    reqpq_sent,
    reqdh_sent,
    client_dh_sent,
    init_temp,
    reqpq_sent_temp,
    reqdh_sent_temp,
    client_dh_sent_temp,
    authorized,
    error,
};

inline static std::string to_string(tgl_dc_state state)
{
    switch (state) {
    case tgl_dc_state::init:
        return "init";
    case tgl_dc_state::reqpq_sent:
        return "request pq sent";
    case tgl_dc_state::reqdh_sent:
        return "request dh sent";
    case tgl_dc_state::client_dh_sent:
        return "client_dh_sent";
    case tgl_dc_state::init_temp:
        return "init (temp)";
    case tgl_dc_state::reqpq_sent_temp:
        return "request pq sent (temp)";
    case tgl_dc_state::reqdh_sent_temp:
        return "request dh sent (temp)";
    case tgl_dc_state::client_dh_sent_temp:
        return "client_dh_sent (temp)";
    case tgl_dc_state::authorized:
        return "authorized";
    case tgl_dc_state::error:
        return "error";
    default:
        assert(false);
        return "unknown dc state";
    }
}

inline static std::ostream& operator<<(std::ostream& os, tgl_dc_state state)
{
    os << to_string(state);
    return os;
}

struct tgl_dc;
class tgl_connection;
class tgl_timer;
class query;

struct tgl_session {
    std::weak_ptr<tgl_dc> dc;
    int64_t session_id;
    int64_t last_msg_id;
    int32_t seq_no;
    int32_t received_messages;
    std::shared_ptr<tgl_connection> c;
    std::set<int64_t> ack_set;
    std::shared_ptr<tgl_timer> ev;
    tgl_session()
        : dc()
        , session_id(0)
        , last_msg_id(0)
        , seq_no(0)
        , received_messages(0)
        , c()
        , ack_set()
        , ev()
    { }

    void clear();
};

struct tgl_dc_option {
    std::vector<std::pair<std::string, int>> option_list;
};

struct tgl_dc: public std::enable_shared_from_this<tgl_dc> {
    int32_t id;
    int rsa_key_idx;
    tgl_dc_state state;
    std::shared_ptr<tgl_session> session;
    unsigned char auth_key[256];
    unsigned char temp_auth_key[256];
    unsigned char nonce[16];
    unsigned char server_nonce[16];
    unsigned char new_nonce[32];
    int64_t auth_key_id;
    int64_t temp_auth_key_id;
    int64_t temp_auth_key_bind_query_id;

    int64_t server_salt;
    std::shared_ptr<tgl_timer> ev;

    int server_time_delta;
    double server_time_udelta;

    bool auth_transfer_in_process;

    tgl_dc_option ipv6_options;
    tgl_dc_option ipv4_options;

    tgl_dc();
    void reset_authorization();
    void restart_authorization();
    void restart_temp_authorization();

    void increase_active_queries(size_t num = 1);
    void decrease_active_queries(size_t num = 1);

    void add_pending_query(const std::shared_ptr<query>& q);
    void remove_pending_query(const std::shared_ptr<query>& q);
    void send_pending_queries();

    bool is_authorized() const { return m_authorized; }
    void set_authorized(bool b = true) { m_authorized = b; }

    bool is_logged_in() const { return m_logged_in; }
    void set_logged_in(bool b = true) { m_logged_in = b; }

    bool is_configured() const { return m_configured; }
    void set_configured(bool b = true) { m_configured = b; }

    bool is_bound() const { return m_bound; }
    void set_bound(bool b = true) { m_bound = b; }

private:
    void reset_temp_authorization();

private:
    size_t m_active_queries;
    bool m_authorized;
    bool m_logged_in;
    bool m_configured;
    bool m_bound;
    std::list<std::shared_ptr<query>> m_pending_queries;

    void cleanup_timer_expired();
    std::shared_ptr<tgl_timer> m_session_cleanup_timer;
};

#endif
