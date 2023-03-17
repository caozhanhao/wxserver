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
#ifndef WXSERVER_SERVER_HPP
#define WXSERVER_SERVER_HPP
#pragma once

#include "internal/logger.hpp"
#include "internal/msgcrypto.hpp"
#include "wxcli.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "cpp-httplib/httplib.h"

#include "libczh/include/libczh/czh.hpp"
#include "libczh/czh.hpp"

#include <functional>
#include <set>
#include <functional>

namespace ws
{
  namespace details
  {
    std::string xml_parse(const std::string &xml, const std::string &tag)
    {
      auto a = xml.find("<" + tag + ">");
      auto b = xml.find("</" + tag + ">");
      std::string ret;
      auto i = a + tag.size() + 2;
      ret = xml.substr(i, b - i);
      
      if (ret.substr(0, 9) == "<![CDATA[")
      {
        auto tmp = ret.substr(9);
        return tmp.substr(0, tmp.size() - 3);
      }
      return ret;
    }
  }
  class Request
  {
  public:
    enum class MsgType
    {
      text, unknown
    };
    std::string user_id;
    std::string content;
    MsgType msg_type;
  };
  
  class Response
  {
  public:
    enum class MsgType
    {
      none, text, file
    };
    
    MsgType msg_type;
    std::string data;
    std::string to_user;
  public:
    Response() : msg_type(MsgType::none) {};
    
    void set_file(const std::string &path)
    {
      msg_type = MsgType::file;
      data = path;
    }
    
    void set_text(const std::string &res)
    {
      msg_type = MsgType::text;
      data = res;
    }
    
    void set_user(const std::string &user)
    {
      to_user = user;
    }
  };
  
  class Server
  {
  private:
    using HandleType = std::function<void(const Request &, Response &)>;
    bool inited;
    int port;
    Crypto crypto;
    Cli wxcli;
    httplib::Server svr;
    HandleType handle;
  public:
    Server() : inited(false), port(8345) {}
    
    Server &init(const std::string &config_path, const HandleType &handle_)
    {
      handle = handle_;
      init_logger(Severity::NONE, Output::console);
      czh::Czh config_czh(config_path, ::czh::InputMode::file);
      ::czh::Node config;
      try
      {
        config = config_czh.parse();
      }
      catch (::czh::Error &e)
      {
        critical(no_fmt, "Failed to load config.", e.get_content());
      }
      catch (::czh::CzhError &e)
      {
        critical(no_fmt, "Failed to load config.", e.get_content());
      }
      
      auto token = config["config"]["Token"].get<std::string>();
      auto encoding_aes_key = config["config"]["EncodingAESKey"].get<std::string>();
      auto corp_id = config["config"]["CorpID"].get<std::string>();
      auto corp_secret = config["config"]["CorpSecret"].get<std::string>();
      
      port = config["config"]["Port"].get<int>();
      crypto = Crypto(token, encoding_aes_key, corp_id);
      wxcli.set_corp(corp_id, corp_secret);
      
      inited = true;
      return *this;
    }
    
    Server &run()
    {
      if (!inited)
      {
        critical(no_fmt, "Not inited.");
      }
      svr.Get("/",
              [this](const httplib::Request &req, httplib::Response &res)
              {
                auto msg_sig = req.get_param_value("msg_signature");
                auto timestamp = req.get_param_value("timestamp");
                auto nonce = req.get_param_value("nonce");
                auto req_echostr = req.get_param_value("echostr");
                auto echostr = crypto.verify_url(msg_sig, timestamp, nonce, req_echostr);
                res.set_content(echostr, "text/plain");
                info(no_fmt, "Verify url successfully.");
              });
      svr.Post("/",
               [this](const httplib::Request &req, httplib::Response &res)
               {
                 auto a = req.body.find("<xml>");
                 auto b = req.body.find("</xml>");
                 auto req_xml = req.body.substr(a, b + 6);
        
                 auto msg_encrypt = details::xml_parse(req_xml, "Encrypt");
                 auto plain_xml = crypto.decrypt_msg(req.get_param_value("msg_signature"),
                                                     req.get_param_value("timestamp"),
                                                     req.get_param_value("nonce"),
                                                     details::xml_parse(req_xml, "Encrypt"));
        
                 Response wxres;
                 Request wxreq;
        
                 wxreq.content = details::xml_parse(plain_xml, "Content");
                 wxreq.user_id = details::xml_parse(plain_xml, "FromUserName");
                 if (auto type = details::xml_parse(plain_xml, "MsgType"); type == "text")
                 {
                   wxreq.msg_type = Request::MsgType::text;
                 }
                 else
                 {
                   wxreq.msg_type = Request::MsgType::unknown;
                 }
        
                 handle(wxreq, wxres);
        
                 auto to_user_id = wxres.to_user.empty() ? wxreq.user_id : wxres.to_user;
                 switch (wxres.msg_type)
                 {
                   case Response::MsgType::text:
                     wxcli.send_text(wxres.data, to_user_id);
                     break;
                   case Response::MsgType::file:
                     wxcli.send_file(wxres.data, to_user_id);
                     break;
                   case Response::MsgType::none:
                     break;
                   default:
                     critical(no_fmt, "Unknown response type");
                     break;
                 }
        
                 info(no_fmt, "From: ", wxreq.user_id,
                      " | Content: ", wxreq.content,
                      " | Respond to: ", to_user_id,
                      " | Response: ", wxres.data);
               });
      info(no_fmt, "Server started.");
      svr.listen("127.0.0.1", port);
      return *this;
    }
    
    auto &get_cli() { return wxcli; }
  };
}
#endif