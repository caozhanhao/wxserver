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

#include "logger.hpp"
#include "msgcrypto.hpp"
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
  
  czh::Node parse_config(const std::string &path)
  {
    czh::Czh config_czh(path, ::czh::InputMode::file);
    ::czh::Node config;
    try
    {
      config = config_czh.parse();
    }
    catch (::czh::Error &e)
    {
      critical(no_fmt, "Failed to parse config.", e.get_content());
    }
    catch (::czh::CzhError &e)
    {
      critical(no_fmt, "Failed to parse config.", e.get_content());
    }
    return std::move(config);
  }
  
  class Server
  {
  private:
    using MsgHandle = std::function<void(const Request &, Response &)>;
    bool inited;
    int port;
    Crypto crypto;
    Cli wxcli;
    httplib::Server svr;
    MsgHandle msg_handle;
  public:
    Server() : inited(false), port(-1) {}
    
    Server(const Server &) = delete;
    
    Server &load_config(const czh::Node &config)
    {
      auto agent_id = config["weixin"]["AgentId"].get<int>();
      auto token = config["weixin"]["Token"].get<std::string>();
      auto encoding_aes_key = config["weixin"]["EncodingAESKey"].get<std::string>();
      auto corp_id = config["weixin"]["CorpID"].get<std::string>();
      auto corp_secret = config["weixin"]["CorpSecret"].get<std::string>();
      port = config["server"]["port"].get<int>();
      if (!config["server"]["logging_path"].is<czh::value::Null>())
      {
        init_logger(Severity::NONE, Output::file_and_console, config["server"]["logging_path"].get<std::string>());
      }
      else
      {
        init_logger(Severity::NONE, Output::console);
      }
  
      crypto = Crypto(token, encoding_aes_key, corp_id);
      wxcli.set_corp(corp_id, corp_secret, agent_id);
      
      inited = true;
      return *this;
    }
    
    Server &load_config(const std::string &config_path)
    {
      auto config = parse_config(config_path);
      load_config(config);
      return *this;
    }
    
    Server &add_msg_handle(const MsgHandle &handle)
    {
      msg_handle = handle;
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
  
                 if (msg_handle)
                 {
                   msg_handle(wxreq, wxres);
                 }
  
                 std::string notice;
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
  
                 notice += "From: " + wxreq.user_id +
                           +" | Content: " + wxreq.content;
  
                 if (wxres.msg_type != Response::MsgType::none)
                 {
                   notice +=
                       " | To: " + to_user_id
                       + " | Response "
                       + (wxres.msg_type == Response::MsgType::file ? "File: " : "Text: ")
                       + wxres.data;
                 }
                 info(no_fmt, notice);
               });
      info(no_fmt, "Server started.");
      svr.listen("127.0.0.1", port);
      return *this;
    }
  
    Server &send_text(const std::string &msg, const std::string &id)
    {
      info(no_fmt,
           "To: ", id,
           " | Response Text: ", msg);
      wxcli.send_text(msg, id);
      return *this;
    }
  
    Server &send_file(const std::string &path, const std::string &id)
    {
      info(no_fmt,
           "To: ", id,
           " | Response File: ", path);
      wxcli.send_file(path, id);
      return *this;
    }
  };
}
#endif