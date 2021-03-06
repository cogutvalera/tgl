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

#include "query_messages_get_dh_config.h"

#include "auto/auto.h"
#include "auto/auto_types.h"
#include "secret_chat.h"

namespace tgl {
namespace impl {

query_messages_get_dh_config::query_messages_get_dh_config(user_agent& ua,
        const std::shared_ptr<secret_chat>& sc,
        const std::function<void(const std::shared_ptr<secret_chat>&,
                const std::function<void(bool, const std::shared_ptr<secret_chat>&)>&)>& callback,
        const std::function<void(bool, const std::shared_ptr<secret_chat>&)>& final_callback,
        double timeout)
    : query(ua, "get dh config", TYPE_TO_PARAM(messages_dh_config))
    , m_secret_chat(sc)
    , m_callback(callback)
    , m_final_callback(final_callback)
    , m_timeout(timeout)
{
}

void query_messages_get_dh_config::on_answer(void* D)
{
    tl_ds_messages_dh_config* DS_MDC = static_cast<tl_ds_messages_dh_config*>(D);

    bool fail = false;
    if (DS_MDC->magic == CODE_messages_dh_config) {
        if (DS_MDC->p && DS_MDC->p->len == 256 && DS_MDC->random && DS_MDC->random->len == 256) {
            if (!m_secret_chat->set_dh_parameters(DS_LVAL(DS_MDC->version), DS_LVAL(DS_MDC->g),
                    reinterpret_cast<unsigned char*>(DS_MDC->p->data), reinterpret_cast<unsigned char*>(DS_MDC->random->data))) {
                fail = true;
            }
        } else {
            TGL_WARNING("the prime got from the server is not of size 256");
            fail = true;
        }
    } else if (DS_MDC->magic == CODE_messages_dh_config_not_modified) {
        TGL_NOTICE("secret chat dh config version not modified");
        if (m_secret_chat->encryption_version() != DS_LVAL(DS_MDC->version)) {
            TGL_WARNING("encryption parameter versions mismatch");
            fail = true;
        }
    } else {
        TGL_WARNING("the server sent us something wrong");
        fail = true;
    }

    if (fail) {
        m_secret_chat->set_deleted();
        if (m_final_callback) {
            m_final_callback(false, m_secret_chat);
        }
        return;
    }

    if (m_callback) {
        m_callback(m_secret_chat, m_final_callback);
    }
}

int query_messages_get_dh_config::on_error(int error_code, const std::string& error_string)
{
    TGL_ERROR("RPC_CALL_FAIL " << error_code << " " << error_string);
    m_secret_chat->set_deleted();
    if (m_final_callback) {
        m_final_callback(false, m_secret_chat);
    }
    return 0;
}

void query_messages_get_dh_config::on_timeout()
{
    TGL_ERROR("timed out for query #" << msg_id() << " (" << name() << ")");
    if (m_final_callback) {
        m_final_callback(false, m_secret_chat);
    }
}

double query_messages_get_dh_config::timeout_interval() const
{
    if (m_timeout > 0) {
        return m_timeout;
    }

    return query::timeout_interval();
}

bool query_messages_get_dh_config::should_retry_on_timeout() const
{
    if (m_timeout > 0) {
        return false;
    }

    return query::should_retry_on_timeout();
}

void query_messages_get_dh_config::will_be_pending()
{
    if (m_timeout > 0) {
        timeout_within(timeout_interval());
    }
}

}
}
