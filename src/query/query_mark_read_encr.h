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

    Copyright Vitaly Valtman 2013-2015
    Copyright Topology LP 2016-2017
*/

#pragma once

#include "query.h"

#include "secret_chat.h"
#include "tgl/tgl_update_callback.h"

namespace tgl {
namespace impl {

class message;

class query_mark_read_encr: public query
{
public:
    query_mark_read_encr(user_agent& ua,
            const std::shared_ptr<secret_chat>& sc,
            int32_t max_time,
            const std::function<void(bool, const std::shared_ptr<message>&)>& callback)
        : query(ua, "mark read encrypted", TYPE_TO_PARAM(bool))
        , m_secret_chat(sc)
        , m_max_time(max_time)
        , m_callback(callback)
    { }

    virtual void on_answer(void*) override
    {
        m_user_agent.callback()->mark_messages_read(false, tgl_peer_id_t::from_input_peer(m_secret_chat->id()), m_max_time);
        if (m_callback) {
            m_callback(true, nullptr);
        }
    }

    virtual int on_error(int error_code, const std::string& error_string) override
    {

        TGL_ERROR("mark read failed " << error_string << "max time: " << m_max_time);
        if (m_secret_chat->state() != tgl_secret_chat_state::deleted && error_code == 400 && error_string == "ENCRYPTION_DECLINED") {
            m_secret_chat->set_deleted();
        }

        if (m_callback) {
            m_callback(false, nullptr);
        }

        return 0;
    }

private:
    std::shared_ptr<secret_chat> m_secret_chat;
    int32_t m_max_time;
    std::function<void(bool, const std::shared_ptr<message>&)> m_callback;
};

}
}
