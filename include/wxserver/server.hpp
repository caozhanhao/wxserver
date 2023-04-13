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

#include "cpp-httplib/httplib.h"
#include "libczh/czh.hpp"

#include <functional>
#include <set>

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
    using MsgHandle = std::function<void(const Request &, Message &)>;
  private:
    bool inited;
    std::string host;
    int port;
    Crypto crypto;
    Cli wxcli;
    httplib::Server svr;
    MsgHandle msg_handle;
    std::function<httplib::TaskQueue *(void)> new_task_queue;
  public:
    Server() : inited(false), port(-1),
               new_task_queue([] { return new httplib::ThreadPool(4); }) {}
  
    Server(std::string host_, int port_, int agent_id, const std::string &token, const std::string encoding_aes_key,
           const std::string corp_id, const std::string corp_secret,
           bool enable_console_logger, const std::string &logging_path = "")
        : inited(true), host(std::move(host_)), port(port_), crypto(token, encoding_aes_key, corp_id),
          wxcli(corp_id, corp_secret, agent_id),
          new_task_queue([] { return new httplib::ThreadPool(4); })
    {
      if (!logging_path.empty())
      {
        if (enable_console_logger)
        {
          init_logger(Severity::NONE, Output::file_and_console, logging_path);
        }
        else
        {
          init_logger(Severity::NONE, Output::file);
        }
      }
      else
      {
        if (enable_console_logger)
        {
          init_logger(Severity::NONE, Output::console);
        }
      }
    }
  
    Server(const Server &) = delete;
  
    Server &load_config(const czh::Node &config)
    {
      auto agent_id = config["weixin"]["AgentId"].get<int>();
      auto token = config["weixin"]["Token"].get<std::string>();
      auto encoding_aes_key = config["weixin"]["EncodingAESKey"].get<std::string>();
      auto corp_id = config["weixin"]["CorpID"].get<std::string>();
      auto corp_secret = config["weixin"]["CorpSecret"].get<std::string>();
      port = config["server"]["port"].get<int>();
      host = config["server"]["host"].get<std::string>();
  
      auto enable_console_logger = config["server"]["enable_console_logger"].get<bool>();
      if (!config["server"]["logging_path"].is<czh::value::Null>())
      {
        if (enable_console_logger)
        {
          init_logger(Severity::NONE, Output::file_and_console, config["server"]["logging_path"].get<std::string>());
        }
        else
        {
          init_logger(Severity::NONE, Output::file, config["server"]["logging_path"].get<std::string>());
        }
      }
      else
      {
        if (enable_console_logger)
        {
          init_logger(Severity::NONE, Output::console);
        }
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
      std::unique_ptr<httplib::TaskQueue> task_queue(new_task_queue());
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
               [this, &task_queue](const httplib::Request &req, httplib::Response &res)
               {
                 auto a = req.body.find("<xml>");
                 auto b = req.body.find("</xml>");
                 auto req_xml = req.body.substr(a, b + 6);
  
                 auto msg_encrypt = details::xml_parse(req_xml, "Encrypt");
                 std::string plain_xml;
                 try
                 {
                   plain_xml = crypto.decrypt_msg(req.get_param_value("msg_signature"),
                                                  req.get_param_value("timestamp"),
                                                  req.get_param_value("nonce"),
                                                  details::xml_parse(req_xml, "Encrypt"));
                 }
                 catch (MsgCryptoError &err)
                 {
                   error(FormatWithLoc(no_fmt, err.location), err.what());
                   return;
                 }
  
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
  
                 info(no_fmt, "From: ", wxreq.user_id, " | Content: ", wxreq.content);
  
                 if (msg_handle)
                 {
                   task_queue->enqueue([this, wxreq]()
                                       {
                                         Message wxres;
                                         msg_handle(wxreq, wxres);
                                         if (wxres.to_user.empty())
                                         {
                                           wxres.set_user(wxreq.user_id);
                                         }
                                         send_message(wxres);
                                       });
                 }
               });
      info(no_fmt, "Server started. Host: ", host, ", Port: ", port);
      svr.listen(host, port);
      task_queue->shutdown();
      return *this;
    }
  
    Server &send_message(const Message &msg)
    {
      info(no_fmt, "To: ", msg.to_user, " | Response ", msg_type_to_str(msg.msg_type), ": ", msg.data);
      wxcli.send(msg);
      return *this;
    }
  };
}
#endif