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

    Copyright Nikolay Durov, Andrey Lopatin 2012-2013
              Vitaly Valtman 2013-2015
*/
#ifndef __MTPROTO_COMMON_H__
#define __MTPROTO_COMMON_H__

#include <string.h>
#include "crypto/rsa_pem.h"
#include "crypto/bn.h"
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdexcept>
#include <memory>
#include <string>
#include <vector>

#if defined(_MSC_VER) || defined(__MINGW32__)
#define INT64_PRINTF_MODIFIER "I64"
#else
#define INT64_PRINTF_MODIFIER "ll"
#endif

#include "tools.h"

#include "constants.h"

//#include "tgl-inner.h"
/* DH key exchange protocol data structures */
#define	CODE_req_pq			0x60469778
#define CODE_resPQ			0x05162463
#define CODE_req_DH_params		0xd712e4be
#define CODE_p_q_inner_data		0x83c95aec
#define CODE_p_q_inner_data_temp		0x3c6a84d4
#define CODE_server_DH_inner_data	0xb5890dba
#define CODE_server_DH_params_fail	0x79cb045d
#define CODE_server_DH_params_ok	0xd0e8075c
#define CODE_set_client_DH_params	0xf5045f1f
#define CODE_client_DH_inner_data	0x6643b654
#define CODE_dh_gen_ok			0x3bcbf734
#define CODE_dh_gen_retry		0x46dc1fb9
#define CODE_dh_gen_fail		0xa69dae02 

#define CODE_bind_auth_key_inner 0x75a3f765

/* service messages */
#define CODE_rpc_result			0xf35c6d01
#define CODE_rpc_error			0x2144ca19
#define CODE_msg_container		0x73f1f8dc
#define CODE_msg_copy			0xe06046b2
#define CODE_msgs_ack			0x62d6b459
#define CODE_bad_msg_notification	0xa7eff811
#define	CODE_bad_server_salt		0xedab447b
#define CODE_msgs_state_req		0xda69fb52
#define CODE_msgs_state_info		0x04deb57d
#define CODE_msgs_all_info		0x8cc0d131
#define CODE_new_session_created	0x9ec20908
#define CODE_msg_resend_req		0x7d861a08
#define CODE_ping			0x7abe77ec
#define CODE_pong			0x347773c5
#define CODE_destroy_session		0xe7512126
#define CODE_destroy_session_ok		0xe22045fc
#define CODE_destroy_session_none      	0x62d350c9
#define CODE_destroy_sessions		0x9a6face8
#define CODE_destroy_sessions_res	0xa8164668
#define	CODE_get_future_salts		0xb921bd04
#define	CODE_future_salt		0x0949d9dc
#define	CODE_future_salts		0xae500895
#define	CODE_rpc_drop_answer		0x58e4a740
#define CODE_rpc_answer_unknown		0x5e2ad36e
#define CODE_rpc_answer_dropped_running	0xcd78e586
#define CODE_rpc_answer_dropped		0xa43ad8b7
#define	CODE_msg_detailed_info		0x276d3ec6
#define	CODE_msg_new_detailed_info	0x809db6df
#define CODE_ping_delay_disconnect	0xf3427b8c
#define CODE_gzip_packed 0x3072cfa1

#define CODE_input_peer_notify_settings_old 0x3cf4b1be
#define CODE_peer_notify_settings_old 0xddbcd4a5
#define CODE_user_profile_photo_old 0x990d1493
#define CODE_config_old 0x232d5905

#define CODE_msg_new_detailed_info 0x809db6df

#define CODE_msg_detailed_info 0x276d3ec6
#define MAX_PROTO_MESSAGE_INTS	1048576

void tgl_prng_seed (const char *password_filename, int password_length);
int tgl_serialize_bignum (const TGLC_bn *b, char *buffer, int maxlen);
long long tgl_do_compute_rsa_key_fingerprint (TGLC_rsa *key);

class mtprotocol_serializer
{
public:
    explicit mtprotocol_serializer(size_t initial_buffer_capacity = 256 /*int32_ts*/)
    {
        m_data.reserve(initial_buffer_capacity);
    }

    void out_i32s(const int32_t* ints, size_t num)
    {
        m_data.resize(m_data.size() + num);
        memcpy(m_data.data() + m_data.size() - num, ints, num * 4);
    }

    void out_i32s_at(size_t at, const int32_t* ints, size_t num)
    {
        memcpy(m_data.data() + at, ints, num * 4);
    }

    void out_i32_at(size_t at, int32_t i)
    {
        out_i32s_at(at, &i, 1);
    }

    void out_i32(int32_t i)
    {
        out_i32s(&i, 1);
    }

    void out_i64(int64_t i)
    {
        m_data.resize(m_data.size() + 2);
        *reinterpret_cast<int64_t*>(m_data.data() + m_data.size() - 2) = i;
    }

    void out_i64_at(size_t at, int64_t i)
    {
        *reinterpret_cast<int64_t*>(m_data.data() + at) = i;
    }

    void out_double(double d)
    {
        static_assert(sizeof(double) == 8, "We assume double is 8 bytes");
        m_data.resize(m_data.size() + 2);
        *reinterpret_cast<double*>(m_data.data() + m_data.size() - 2) = d;
    }

    void out_string(const char* str, size_t size)
    {
        if (size >= (1 << 24)) {
            throw std::invalid_argument("string is too big");
        }
        char* dest = nullptr;
        if (size < 0xfe) {
            size_t num = ((1 + size) + 3) / 4;
            m_data.resize(m_data.size() + num);
            dest = reinterpret_cast<char*>(m_data.data() + m_data.size() - num);
            *dest++ = static_cast<char>(size);
        } else {
            size_t num = ((4 + size) + 3) / 4;
            m_data.resize(m_data.size() + num);
            dest = reinterpret_cast<char*>(m_data.data() + m_data.size() - num);
            *reinterpret_cast<int32_t*>(dest) = static_cast<int32_t>((size << 8) + 0xfe);
            dest += 4;
        }

        memcpy(dest, str, size);
        dest += size;
        while (reinterpret_cast<intptr_t>(dest) & 3) {
            *dest++ = 0;
        }
    }

    void out_string(const char* str)
    {
        out_string(str, strlen(str));
    }

    void out_std_string (const std::string& str)
    {
        out_string(str.c_str(), str.size());
    }

    void out_bignum(TGLC_bn* n)
    {
        int required_size = -tgl_serialize_bignum(n, nullptr, -1);
        if (required_size <= 0) {
            throw std::invalid_argument("bad big number");
        }
        assert(!(required_size & 3));
        int num = required_size / 4;
        m_data.resize(m_data.size() + num);
        int actual_size = tgl_serialize_bignum(n, reinterpret_cast<char*>(m_data.data() + m_data.size() - num), required_size);
        assert(required_size == actual_size);
    }

    void out_random(int length)
    {
        std::unique_ptr<unsigned char[]> buffer(new unsigned char[length]);
        tglt_secure_random(buffer.get(), length);
        out_string(reinterpret_cast<const char*>(buffer.get()), length);
    }

    size_t reserve_i32s(size_t num_of_i32)
    {
        size_t old_size = m_data.size();
        m_data.resize(old_size + num_of_i32);
        return old_size;
    }

    size_t ensure_char_size(size_t bytes)
    {
        size_t new_size = (bytes + 3) / 4;
        size_t old_size = m_data.size();
        if (old_size < new_size) {
            m_data.resize(new_size, 0);
        }
        return old_size * 4;
    }

    void clear() { m_data.clear(); }

    const int32_t* i32_data() const { return m_data.data(); }
    size_t i32_size() const { return m_data.size(); }
    const char* char_data() const { return reinterpret_cast<const char*>(m_data.data()); }
    size_t char_size() const { return m_data.size() * 4; }

    int32_t* mutable_i32_data() { return m_data.data(); }
    char* mutable_char_data() { return reinterpret_cast<char*>(m_data.data()); }

private:
    std::vector<int32_t> m_data;
};

struct tgl_in_buffer {
    int* ptr;
    int* end;
};

static inline int prefetch_strlen(struct tgl_in_buffer* in) {
  if (in->ptr >= in->end) {
    return -1; 
  }
  unsigned l = *in->ptr;
  if ((l & 0xff) < 0xfe) { 
    l &= 0xff;
    return (in->end >= in->ptr + (l >> 2) + 1) ? (int)l : -1;
  } else if ((l & 0xff) == 0xfe) {
    l >>= 8;
    return (l >= 254 && in->end >= in->ptr + ((l + 7) >> 2)) ? (int)l : -1;
  } else {
    return -1;
  }
}

static inline char *fetch_str (struct tgl_in_buffer* in, int len) {
  assert (len >= 0);
  if (len < 254) {
    char *str = (char *) in->ptr + 1;
    in->ptr += 1 + (len >> 2);
    return str;
  } else {
    char *str = (char *) in->ptr + 4;
    in->ptr += (len + 7) >> 2;
    return str;
  }
} 

static inline char *fetch_str_dup (struct tgl_in_buffer* in) {
  int l = prefetch_strlen (in);
  assert (l >= 0);
  int i;
  char *s = fetch_str (in, l);
  for (i = 0; i < l; i++) {
    if (!s[i]) { break; }
  }
  char *r = (char *)malloc (i + 1);
  memcpy (r, s, i);
  r[i] = 0;
  return r;
}

static inline int fetch_update_str (struct tgl_in_buffer* in, char **s) {
  if (!*s) {
    *s = fetch_str_dup (in);
    return 1;
  }
  int l = prefetch_strlen (in);
  char *r = fetch_str (in, l);
  if (memcmp (*s, r, l) || (*s)[l]) {
    tfree_str (*s);
    *s = (char *)malloc (l + 1);
    memcpy (*s, r, l);
    (*s)[l] = 0;
    return 1;
  }
  return 0;
}

static inline int fetch_update_int (struct tgl_in_buffer* in, int *value) {
  if (*value == *in->ptr) {
    in->ptr ++;
    return 0;
  } else {
    *value = *(in->ptr ++);
    return 1;
  }
}

static inline int fetch_update_long (struct tgl_in_buffer* in, long long int *value) {
  if (*value == *(long long int *)in->ptr) {
    in->ptr += 2;
    return 0;
  } else {
    *value = *(long long int*)(in->ptr);
    in->ptr += 2;
    return 1;
  }
}

static inline int set_update_int (int *value, int new_value) {
  if (*value == new_value) {
    return 0;
  } else {
    *value = new_value;
    return 1;
  }
}

static inline void fetch_skip (struct tgl_in_buffer* in, int n) {
  in->ptr += n;
  assert (in->ptr <= in->end);
}

static inline void fetch_skip_str (struct tgl_in_buffer* in) {
  int l = prefetch_strlen (in);
  assert (l >= 0);
  fetch_str (in, l);
}

static inline long have_prefetch_ints (struct tgl_in_buffer* in) {
  return in->end - in->ptr;
}

int tgl_fetch_bignum (struct tgl_in_buffer* in, TGLC_bn *x);

static inline int fetch_bignum(struct tgl_in_buffer* in, TGLC_bn *x)
{
    return tgl_fetch_bignum(in, x);
}

static inline int fetch_int (struct tgl_in_buffer* in) {
  assert (in->ptr + 1 <= in->end);
  return *(in->ptr ++);
}

static inline int fetch_bool (struct tgl_in_buffer* in) {
  assert (in->ptr + 1 <= in->end);
  assert (*(in->ptr) == (int)CODE_bool_true || *(in->ptr) == (int)CODE_bool_false);
  return *(in->ptr ++) == (int)CODE_bool_true;
}

static inline int prefetch_int (struct tgl_in_buffer* in) {
  assert (in->ptr < in->end);
  return *(in->ptr);
}

static inline void prefetch_data (struct tgl_in_buffer* in, void *data, int size) {
  assert (in->ptr + (size >> 2) <= in->end);
  memcpy (data, in->ptr, size);
}

static inline void fetch_data (struct tgl_in_buffer* in, void *data, int size) {
  assert (in->ptr + (size >> 2) <= in->end);
  memcpy (data, in->ptr, size);
  assert (!(size & 3));
  in->ptr += (size >> 2);
}

static inline long long fetch_long (struct tgl_in_buffer* in) {
  assert (in->ptr + 2 <= in->end);
  long long r = *(long long *)in->ptr;
  in->ptr += 2;
  return r;
}

static inline double fetch_double (struct tgl_in_buffer* in) {
  assert (in->ptr + 2 <= in->end);
  double r = *(double *)in->ptr;
  in->ptr += 2;
  return r;
}

static inline void fetch_ints (struct tgl_in_buffer* in, void *data, int count) {
  assert (in->ptr + count <= in->end);
  memcpy (data, in->ptr, 4 * count);
  in->ptr += count;
}
    
static inline int in_remaining (struct tgl_in_buffer* in) {
  return 4 * (in->end - in->ptr);
}

//int get_random_bytes (unsigned char *buf, int n);

int tgl_pad_rsa_encrypt (const char *from, int from_len, char *to, int size, TGLC_bn *N, TGLC_bn *E);
int tgl_pad_rsa_decrypt (const char *from, int from_len, char *to, int size, TGLC_bn *N, TGLC_bn *D);

static inline int tgl_pad_rsa_encrypt_dest_buffer_size(int src_buffer_size)
{
    return tgl_pad_rsa_encrypt(NULL, src_buffer_size, NULL, 0, NULL, NULL);
}

//extern long long rsa_encrypted_chunks, rsa_decrypted_chunks;

//extern unsigned char aes_key_raw[32], aes_iv[32];
//extern TGLC_aes_key aes_key;

#define AES_DECRYPT 0
#define AES_ENCRYPT 1

struct TGLC_aes_key;

void tgl_init_aes_unauth(TGLC_aes_key* aes_key, unsigned char aes_iv[32], const unsigned char server_nonce[16], const unsigned char hidden_client_nonce[32], int encrypt);
void tgl_init_aes_auth(TGLC_aes_key* aes_key, unsigned char aes_iv[32], const unsigned char auth_key[192], const unsigned char msg_key[16], int encrypt);
int tgl_pad_aes_encrypt(const TGLC_aes_key* aes_key, unsigned char aes_iv[32], const unsigned char *from, int from_len, unsigned char *to, int size);
int tgl_pad_aes_decrypt(const TGLC_aes_key* aes_key, unsigned char aes_iv[32], const unsigned char *from, int from_len, unsigned char *to, int size);

static inline int tgl_pad_aes_encrypt_dest_buffer_size(int src_buffer_size)
{
    return tgl_pad_aes_encrypt(NULL, NULL, NULL, src_buffer_size, NULL, 0);
}

static inline int tgl_pad_aes_decrypt_dest_buffer_size(int src_buffer_size)
{
    assert(src_buffer_size > 0);
    return src_buffer_size;
}

void *tgl_alloc0 (size_t size);

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#endif
#endif
