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

#include "chat.h"
#include "query.h"
#include "tgl/tgl_log.h"
#include "user.h"

#include <functional>
#include <string>

namespace tgl {
namespace impl {

class query_resolve_username: public query
{
public:
    query_resolve_username(user_agent& ua, const std::function<void(bool)>& callback)
        : query(ua, "contact resolve username", TYPE_TO_PARAM(contacts_resolved_peer))
        , m_callback(callback)
    { }

    virtual void on_answer(void* D) override
    {
        const tl_ds_contacts_resolved_peer* DS_CRU = static_cast<const tl_ds_contacts_resolved_peer*>(D);
        int32_t n = DS_LVAL(DS_CRU->users->cnt);
        for (int32_t i = 0; i < n; ++i) {
            if (auto u = user::create(DS_CRU->users->data[i])) {
                m_user_agent.user_fetched(u);
            }
        }
        n = DS_LVAL(DS_CRU->chats->cnt);
        for (int32_t i = 0; i < n; ++i) {
            if (auto c = chat::create(DS_CRU->chats->data[i])) {
                m_user_agent.chat_fetched(c);
            }
        }
        if (m_callback) {
            m_callback(true);
        }
    }

    virtual int on_error(int error_code, const std::string& error_string) override
    {
        TGL_ERROR("RPC_CALL_FAIL " << error_code << " " << error_code);
        if (m_callback) {
            m_callback(false);
        }
        return 0;
    }

private:
    std::function<void(bool)> m_callback;
};

}
}
