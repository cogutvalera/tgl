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

#include "query_send_messages.h"

#include "updater.h"

namespace tgl {
namespace impl {

query_send_messages::query_send_messages(user_agent& ua,
        const std::function<void(bool)>& callback)
    : query(ua, "send messages", TYPE_TO_PARAM(updates))
    , m_callback(callback)
    , m_message(nullptr)
{ }

void query_send_messages::on_answer(void* D)
{
    const tl_ds_updates* DS_U = static_cast<const tl_ds_updates*>(D);
    m_user_agent.updater().work_any_updates(DS_U, update_context(m_message));
    if (m_callback) {
        m_callback(true);
    }
}

int query_send_messages::on_error(int error_code, const std::string& error_string)
{
    TGL_ERROR("RPC_CALL_FAIL " <<  error_code << " " << error_string);
    if (m_callback) {
        m_callback(false);
    }
    return 0;
}

void query_send_messages::set_message(const std::shared_ptr<class message>& message)
{
    m_message = message;
}

}
}
