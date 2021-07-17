#pragma once

#include "wxhttp.h"
#include "wxerr.h"

#include <string>
#include <functional>
#include <map>

namespace ws
{
  using WXcmd_ret = std::pair<const std::string, const std::string>;
  using WXcmd_func = std::function<WXcmd_ret(const std::string&)>;
  class WXcmd
  {
  private: 
    std::string access_token; 
    std::string corpid;
    std::string corpsecret;
    std::map<std::string, WXcmd_func> commands;
  public:
    WXcmd(): access_token(""), corpid(""), corpsecret("") {}
    WXcmd(const std::string& _corpid, const std::string& _corpsecret)
      : access_token(""), corpid(_corpid), corpsecret(_corpsecret) {}

    void add_cmd(const std::string& tag, const WXcmd_func& func)
    {
      commands[tag] = func;
    }
    void command(const std::string& stmt, const std::string& UserID)
    {
      auto i = stmt.find(" ");
      std::string cmd = stmt.substr(1, i - 1);
      std::string args = stmt.substr(i + 1);

      if(commands.find(cmd) == commands.end()) return;
      auto ret = commands[cmd](args);
      send(ret.first, ret.second, UserID);
    }

    void send(const std::string& type, const std::string& msg, const std::string& id)
    {
      if (type == "text")
      {
        std::string postdata = R""({
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
        wxpost(postdata);
      }
      else if (type == "file")
      {
        std::string postdata = R""({
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
        wxpost(postdata);
      }
      else
        throw WXerr(WS_ERROR_LOCATION, __func__, "error type '" + type + "'.");
    }

  private:
    std::string wxpost(const std::string& postdata)
    {
      std::string url = "https://qyapi.weixin.qq.com/cgi-bin/message/send?access_token=" + access_token;
      WXhttp h(url);
      std::string res = h.POST(postdata, "");
      int k = res.find(",");
      std::string i = res.substr(0, k);
      if (i == "{\"errcode\":0") return res;
      else if (i == "{\"errcode\":41001" || i == "{\"errcode\":42001" || i == "{\"errcode\":40014")
      {
        get_access_token();
        wxpost(postdata);
      }
      else
        throw WXerr(WS_ERROR_LOCATION, __func__, "res: " + res + "\npostdata: " + postdata + "\n");
      return "";
    }

    std::string get_media_id(const std::string& path)
    {
      std::string url = "https://qyapi.weixin.qq.com/cgi-bin/media/upload?access_token=" + access_token + "&type=file";
      WXhttp h(url);
      std::string res = h.POST("", path);
      int k = res.find(",");
      std::string s = res.substr(0, k);
      if (s == "{\"errcode\":0")
      {
        int a = res.find("media_id");
        std::string media_id = res.substr(a + 11);
        int b = media_id.find("\",\"created_at");
        media_id = media_id.substr(0, b);
        return media_id;
      }
      else
      {
        throw WXerr(WS_ERROR_LOCATION, __func__, "res: " + res + "\npath: " + path + "\n");
      }
    }
    void get_access_token()
      //获取access_token：https://open.work.weixin.qq.com/api/doc/90000/90135/91039
    {
      std::string url = "https://qyapi.weixin.qq.com/cgi-bin/gettoken?corpid=" + corpid + "&corpsecret=" + corpsecret;
      WXhttp h(url);
      std::string res = h.GET();
      int k = res.find("access_token");
      std::string temp = res.substr(k + 15);
      int j = temp.find("\",\"expires_in\"");
      temp = temp.substr(0, j);
      access_token = temp;
    }
  };
}
