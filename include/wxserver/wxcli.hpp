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
#ifndef WXSERVER_WXCLI_HPP
#define WXSERVER_WXCLI_HPP
#pragma once
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "cpp-httplib/httplib.h"
#include "logger.hpp"

#include "nlohmann/json.hpp"

#include <sstream>
#include <fstream>
#include <string>
#include <functional>
#include <map>
#include <filesystem>

namespace ws
{
  class Cli
  {
  private:
    std::string access_token;
    std::string corp_id;
    std::string corp_secret;
    int agent_id;
  public:
    Cli() = default;
  
    Cli(const Cli &) = delete;
  
    Cli(std::string corp_id_, std::string corp_secret_, int agent_id_)
        : corp_id(std::move(corp_id_)), corp_secret(std::move(corp_secret_)), agent_id(agent_id_) {}
  
    void set_corp(const std::string &corpid_, const std::string corpsecret_, int agent_id_)
    {
      corp_id = corpid_;
      corp_secret = corpsecret_;
      agent_id = agent_id_;
    }
  
    void send_text(const std::string &msg, const std::string &id)
    {
      nlohmann::json postdata{
          {"touser",                   id},
          {"msgtype",                  "text"},
          {"agentid",                  agent_id},
          {"text",
                                       {
                                           {"content", msg}
                                       }
          },
          {"safe",                     0},
          {"enable_id_trans",          0},
          {"enable_duplicate_check",   0},
          {"duplicate_check_interval", 1800}
      };
      wxpost(postdata.dump());
    }
    
    void send_file(const std::string &path, const std::string &id)
    {
      nlohmann::json postdata{
          {"touser",                   id},
          {"msgtype",                  "file"},
          {"agentid",                  agent_id},
          {"file",
                                       {
                                           {"media_id", get_media_id(path)}
                                       }
          },
          {"safe",                     0},
          {"enable_id_trans",          0},
          {"enable_duplicate_check",   0},
          {"duplicate_check_interval", 1800}
      };
      wxpost(postdata.dump());
    }
  
  private:
    std::string wxpost(const std::string &postdata, bool no_retry = false)
    {
      if (access_token.empty()) get_access_token();
      httplib::SSLClient cli("qyapi.weixin.qq.com");
      auto res = cli.Post("/cgi-bin/message/send?access_token=" + access_token, postdata, "text/plain");
      
      if (res == nullptr || res->status != 200)
      {
        critical(no_fmt, "wxpost failed.", "Error: ", httplib::to_string(res.error()));
      }
      
      auto json = nlohmann::json::parse(res->body);
      
      if (auto errcode = json["errcode"].get<int>(); errcode == 0)
      {
        return res->body;
      }
      else if ((errcode == 41001 || errcode == 42001 || errcode == 40014) && !no_retry)
      {
        get_access_token();
        wxpost(postdata, true);
      }
      else
      {
        critical(no_fmt,
                 "wxpost() failed."
                 "errcode: ", std::to_string(errcode), ",res: ", res->body, ",postdata: ", postdata
        );
      }
      return "";
    }
  
    std::string get_media_id(const std::string &path, bool no_retry = false)
    {
      if (access_token.empty()) get_access_token();
      std::ifstream file{path, std::ios::binary};
      if (!std::filesystem::exists(path) || !file.good())
      {
        critical(no_fmt, "No such file.");
      }
    
      file.ignore(std::numeric_limits<std::streamsize>::max());
      size_t file_size = file.gcount();
      file.clear();
      file.seekg(std::ios_base::beg);
      if (file_size > 20 * 1024 * 1024)
      {
        critical(no_fmt, "The file is too big.");
      }
      
      std::stringstream file_ss;
      file_ss << file.rdbuf();
      
      httplib::SSLClient cli("qyapi.weixin.qq.com");
      httplib::MultipartFormDataItems items
          {
              httplib::MultipartFormData
                  {"media", file_ss.str(), std::filesystem::path(path).filename().string(), "application/octet-stream"}
          };
      httplib::Headers headers
          {
              {"Content-Type", "multipart/form-data"}
          };
      auto res = cli.Post("/cgi-bin/media/upload?access_token=" + access_token + "&type=file", headers, items);
      if (res == nullptr || res->status != 200)
      {
        critical(no_fmt, "Get access token failed.", "Error: ", httplib::to_string(res.error()));
      }
      
      auto json = nlohmann::json::parse(res->body);
      if (auto errcode = json["errcode"].get<int>(); errcode != 0)
      {
        if ((errcode == 41001 || errcode == 42001 || errcode == 40014) && !no_retry)
        {
          get_access_token();
          get_media_id(path, true);
        }
        else
        {
          critical(no_fmt,
                   "Get media id failed.",
                   "errcode: ", std::to_string(errcode), ",res: ", res->body, ",path: ", path);
        }
      }
      return json["media_id"].get<std::string>();
    }
    
    void get_access_token()
    // see https://open.work.weixin.qq.com/api/doc/90000/90135/91039
    {
      httplib::SSLClient cli("qyapi.weixin.qq.com");
      httplib::Params params
          {
              {"corpid",     corp_id},
              {"corpsecret", corp_secret}
          };
      auto res = cli.Get("/cgi-bin/gettoken", params, httplib::Headers{}, httplib::Progress{});
      if (res == nullptr || res->status != 200)
      {
        critical(no_fmt, "Get access token failed.", "Error: ", httplib::to_string(res.error()));
      }
      else
      {
        access_token = nlohmann::json::parse(res->body)["access_token"].get<std::string>();
      }
    }
  };
}
#endif