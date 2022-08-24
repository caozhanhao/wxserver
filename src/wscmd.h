//   Copyright 2021-2022 wxserver - caozhanhao
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
#ifndef WXSERVER_WSCMD_H
#define WXSERVER_WSCMD_H
#include "wshttp.h"
#include "wslogger.h"
#include "wsparser.h"

#include <string>
#include <functional>
#include <map>

namespace ws::cmd
{
  using Cmd_ret = std::pair<const std::string, const std::string>;
  using Cmd_func = std::function<
  
  Cmd_ret(const std::string &)
  
  >;
  using Rehabilitative_cmd_func = std::function<void(const std::string &)>;
  
  class Cmd
  {
  private:
    std::string access_token;
    std::string corpid;
    std::string corpsecret;
    std::map <std::string, Cmd_func> commands;
    std::map <std::string, Rehabilitative_cmd_func> rehabilitative_commands;
  public:
    Cmd() = default;
    
    Cmd(std::string corpid_, std::string corpsecret_)
        : corpid(std::move(corpid_)), corpsecret(std::move(corpsecret_)) {}
    
    void set_corp(const std::string &corpid_, const std::string corpsecret_)
    {
      corpid = corpid_;
      corpsecret = corpsecret_;
    }
    
    void add_cmd(const std::string &tag, const Cmd_func &func)
    {
      commands[tag] = func;
    }
    
    void add_rehabilitative_cmd(const std::string &tag, const Rehabilitative_cmd_func &func)
    {
      rehabilitative_commands[tag] = func;
    }
    
    std::string command(const std::string &stmt, const std::string &UserID)
    {
      auto i = stmt.find(' ');
      std::string cmd;
      std::string args;
      if (i == std::string::npos)
      {
        cmd = stmt.substr(1);
      }
      else
      {
        cmd = stmt.substr(1, i - 1);
        args = stmt.substr(i + 1);
      }
      
      if (commands.find(cmd) == commands.end())
      {
        std::string temp = "command('" + cmd + "') not found";
        send("text", temp, UserID);
        return temp;
      }
      Cmd_ret ret = commands[cmd](args);
      send(ret.first, ret.second, UserID);
      if (rehabilitative_commands.find(cmd) != rehabilitative_commands.end())
      {
        rehabilitative_commands[cmd](args);
      }
      return ret.second + "(" + ret.first + ")";
    }
    
    void send(const std::string &type, const std::string &msg, const std::string &id)
    {
      std::string postdata;
      if (type == "text")
      {
        postdata = R""({
           "touser" : ")"" + id + R""(",
            "msgtype" : "text",
            "agentid" : 1000002,                                                                                                                                                              
            "text" : {
                "content" : ")"" + msg + R""("
                     },   
            "safe":0,
            "enable_id_trans": 0,
            "enable_duplicate_check": 0,
            "duplicate_check_interval": 1800
            })"";
      }
      else if (type == "file")
      {
        postdata = R""({
           "touser" : ")"" + id + R""(",
            "msgtype" : ")"" + type + R""(",
            "agentid" : 1000002,                                                                                                                                                              
            ")"" + type + R""(" : {
                "media_id" : ")"" + get_media_id(msg) + R""("
                     },   
            "safe":0,
            "enable_id_trans": 0,
            "enable_duplicate_check": 0,
            "duplicate_check_interval": 1800
            })"";
      }
      else
        WS_FATAL("error type '" + type + "'.", -1);
      wspost(postdata);
    }
  
  private:
    std::string wspost(const std::string &postdata, bool failed = false)
    {
      std::string url = "https://qyapi.weixin.qq.com/cgi-bin/message/send?access_token=" + access_token;
      http::Http h(url);
      std::string res = h.POST(postdata, http::Http::is_postdata);
      auto errcode = json::Json(res).get_errcode();
      if (errcode == 0) return res;
      else if ((errcode == 41001 || errcode == 42001 || errcode == 40014) && !failed)
      {
        get_access_token();
        wspost(postdata, true);
      }
      else
        WS_FATAL("errcode: " + std::to_string(errcode) + "\nres: " + res + "\npostdata: " + postdata + "\n", -1);
      return "";
    }
    
    std::string get_media_id(const std::string &path) const
    {
      std::string url = "https://qyapi.weixin.qq.com/cgi-bin/media/upload?access_token=" + access_token + "&type=file";
      http::Http h(url);
      std::string res = h.POST(path, http::Http::is_path);
      auto errcode = json::Json(res).get_errcode();
      if (errcode == 0)
      {
        auto a = res.find("media_id");
        std::string media_id = res.substr(a + 11);
        auto b = media_id.find("\",\"created_at");
        media_id = media_id.substr(0, b);
        return media_id;
      }
      else
      {
        WS_FATAL("errcode: " + std::to_string(errcode) + "\nres: " + res + "\npath: " + path + "\n", -1);
      }
    }
    
    void get_access_token()
    //获取access_token：https://open.work.weixin.qq.com/api/doc/90000/90135/91039
    {
      std::string url = "https://qyapi.weixin.qq.com/cgi-bin/gettoken?corpid=" + corpid + "&corpsecret=" + corpsecret;
      http::Http h(url);
      std::string res = h.GET();
      access_token = json::Json(res)["access_token"];
    }
  };
}
#endif