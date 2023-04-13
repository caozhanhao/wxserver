//   Copyright 2021 - 2023 wxserver - caozhanhao
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
#ifndef WXSERVER_MSGCRYPTO_HPP
#define WXSERVER_MSGCRYPTO_HPP
#pragma once

#include "logger.hpp"

#include "openssl/aes.h"
#include "openssl/sha.h"
#include "openssl/evp.h"

#if __has_include(<arpa/inet.h>)

#include <arpa/inet.h>

#elif __has_include(<winsock2.h>)
#pragma comment(lib,"ws2_32.lib")
#include <winsock2.h>
#else
#error "wxserver: Can not find ntohl()"
#endif

#include <iostream>
#include <array>
#include <algorithm>
#include <vector>
#include <string>
#include <cstring>
#include <map>
#include <experimental/source_location>


namespace ws
{
  class MsgCryptoError : public std::runtime_error
  {
  public:
    std::experimental::source_location location;
    
    MsgCryptoError(std::string what, std::experimental::source_location location_
    = std::experimental::source_location::current())
        : std::runtime_error(std::move(what)), location(location_)
    {
    }
  };
  
  std::string wx_decode_base64(const std::string &str_encrypt)
  {
    std::string ret;
    int eq = 0;
    for (auto r = str_encrypt.crbegin(); r < str_encrypt.crend(); r++)
    {
      if (*r == '=')
      {
        eq++;
      }
      else
      {
        break;
      }
    }
    
    int osize = int(str_encrypt.size());
    char *out = (char *) malloc(osize);
    if (out == nullptr)
    {
      throw MsgCryptoError("malloc error");
    }
    
    int rsize = 0;
    rsize = EVP_DecodeBlock((unsigned char *) out, (const unsigned char *) str_encrypt.c_str(),
                            int(str_encrypt.size()));
    if (rsize > eq && rsize < osize)
    {
      ret.assign(out, rsize - eq);
    }
    else
    {
      throw MsgCryptoError("EVP_DecodeBlock() error");
    }
    
    free(out);
    out = nullptr;
    return ret;
  }
  
  std::string wx_sha1(const std::string &str)
  {
    unsigned char out[SHA_DIGEST_LENGTH] = {0};
    if (SHA1((const unsigned char *) str.c_str(), str.size(), out) == nullptr)
    {
      throw MsgCryptoError("sha1 error");
    }
    
    std::string ret;
    char tmp[8] = {0};
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
      snprintf(tmp, sizeof(tmp), "%02x", 0xff & out[i]);
      ret.append(tmp);
    }
    return ret;
  }
  
  std::string wx_decrypt_aes(const std::string &str_encrypt, const std::string &key)
  {
    std::string result;
  
  
    auto out = (unsigned char *) malloc(str_encrypt.size());
    if (out == nullptr)
    {
      throw MsgCryptoError("malloc error");
    }
  
    unsigned char ckey[32] = {0};
    unsigned char iv[16] = {0};
    memcpy(ckey, key.c_str(), key.size() > 32 ? 32 : key.size());
    //memcpy(iv, ckey, sizeof(iv) < sizeof(ckey) ? sizeof(iv) : sizeof(ckey));
    memcpy(iv, ckey, sizeof(iv));
    
    AES_KEY aesKey;
    AES_set_decrypt_key(ckey, 8 * 32, &aesKey);
    AES_cbc_encrypt((unsigned char *) str_encrypt.c_str(), out, str_encrypt.size(), &aesKey, iv, AES_DECRYPT);
    if (out[str_encrypt.size() - 1] > 0 && out[str_encrypt.size() - 1] <= 32 &&
        (str_encrypt.size() - out[str_encrypt.size() - 1]) > 0)
      result.append((char *) out, str_encrypt.size() - out[str_encrypt.size() - 1]);
    else
    {
      throw MsgCryptoError("wx_decrypt_aes failed.");
    }
  
    free(out);
    return result;
  }
  
  std::string wx_sort(std::initializer_list <std::string> args)
  {
    std::vector <std::string> temp;
    for (auto &r: args)
    {
      temp.push_back(r);
    }
    std::sort(temp.begin(), temp.end());
    std::string ret;
    for (auto &r: temp)
    {
      ret.append(r);
    }
    return ret;
  }
  
  
  class Crypto
  {
  private:
    std::string token;
    std::string encoding_aes_key;
    std::string corpid;
  
  public:
    Crypto() = default;
  
    Crypto(const Crypto &) = delete;
    Crypto(std::string token_,
           std::string encoding_aes_key_,
           std::string corpid_)
        : token(std::move(token_)), encoding_aes_key(std::move(encoding_aes_key_)), corpid(std::move(corpid_)) {}
    
    int verify_sign(const std::string &msg_sign, const std::string &time_stamp,
                    const std::string &nonce, const std::string &msg_encrypt)
    {
      std::string dev_msg_sign = wx_sha1(wx_sort({token, time_stamp, nonce, msg_encrypt}));
      if (dev_msg_sign == msg_sign)
      {
        return 0;
      }
      else
        return -1;
    }
    
    std::string verify_url(const std::string &msg_sign, const std::string &time_stamp,
                           const std::string &nonce, const std::string &echo_str)
    {
      if (verify_sign(msg_sign, time_stamp, nonce, echo_str) != 0)
      {
        throw MsgCryptoError("verify sign failed.");
      }
      return decrypt(echo_str);
    }
    
    std::string decrypt_msg(const std::string &msg_sign, const std::string &time_stamp,
                            const std::string &nonce, const std::string &msg_encrypt)
    {
      if (verify_sign(msg_sign, time_stamp, nonce, msg_encrypt) != 0)
      {
        throw MsgCryptoError("verify sign failed.");
      }
      return decrypt(msg_encrypt);
    }
  
  private:
    std::string decrypt(const std::string &msg_encrypt)
    {
      std::string aes_key = wx_decode_base64(encoding_aes_key + "=");
      std::string msg_decrypt = wx_decrypt_aes(wx_decode_base64(msg_encrypt), aes_key);
  
      uint32_t net_len = *((const uint32_t *) (msg_decrypt.c_str() + 16));
      uint32_t msg_len = ntohl(net_len);
      std::string ret = msg_decrypt.substr(16 + 4, msg_len);
  
      std::string receiveid = msg_decrypt.substr(16 + 4 + msg_len);
      if (corpid != receiveid)
      {
        throw MsgCryptoError("receiveid('" + receiveid + "') != corpid('" + corpid + "')");
      }
      return ret;
    }
  };
}
#endif