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
#ifndef WXSERVER_WSSERVER_H
#define WXSERVER_WSSERVER_H
#include "wslogger.h"
#include "wsmsg.h"
#include "wsparser.h"
#include "wsthpool.h"
#include "wscmd.h"

#include <ctime>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <map>
#include <functional>
namespace ws::server
{
  class Server
  {
  private:
    bool inited;
    int port;
    msg::Msg wxmsg;
    cmd::Cmd wxcmd;
    std::map <std::string, std::string> tags;
    std::map<std::string, bool> admin;
  public:
    Server() : inited(false), port(8345) {}
    
    Server &init(const std::string &path)
    {
      czh::Czh config_czh(path, ::czh::InputMode::nonstream);
      auto configptr = config_czh.parse();
      if (configptr == nullptr)
      {
        WS_FATAL("Failed loading config.", -1);
      }
      auto &config = *configptr;
      auto token = config["config"]["Token"].get<std::string>();
      auto encoding_aes_key = config["config"]["EncodingAESKey"].get<std::string>();
      auto corpid = config["config"]["CorpID"].get<std::string>();
      auto corpsecret = config["config"]["CorpSecret"].get<std::string>();
      
      port = config["config"]["Port"].get<int>();
      
      wxmsg = msg::Msg(token, encoding_aes_key, corpid);
      wxcmd.set_corp(corpid, corpsecret);
      
      tags = config["tags"].value_map<std::string>();
      admin = config["admin"].value_map<bool>();
      inited = true;
      return *this;
    }
    
    auto get_admins() const
    {
      return admin;
    }
    
    void send_file(const std::string &UserID, const std::string &path)
    {
      wxcmd.send("file", path, UserID);
    }
    
    void send_text(const std::string &UserID, const std::string &str)
    {
      wxcmd.send("text", str, UserID);
    }
    
    Server &run()
    {
      if (!inited) init("config.czh");
      int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      sockaddr_in addr{};
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = INADDR_ANY;
      addr.sin_port = htons(port);
      
      int on = 1;
      setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
      
      if (bind(sock, (sockaddr * ) & addr, sizeof(addr)) != 0)
        WS_FATAL("bind() failed.", -1);
      listen(sock, 0);//���ü���
      
      sockaddr_in clientAddr{};
      int clientAddrSize = sizeof(clientAddr);
      int clientSock;
      thpool::Thpool thpool(16);
      std::function<void(const int, const std::string &)> func =
          [this](const int clientSock, const std::string &requestStr)
          {
            this->url_router(clientSock, requestStr);
          };
      WS_NOTICE("Server started successfully");
      for (auto &r: get_admins())
      {
        if (r.second)
        {
          send_text(r.first, "Server started successfully");
        }
      }
      while (-1 != (clientSock = accept(sock, (sockaddr * ) & clientAddr, (socklen_t * ) & clientAddrSize)))
      {
        std::string requestStr;
        int bufSize = 4096;
        requestStr.resize(bufSize);
        recv(clientSock, &requestStr[0], bufSize, 0);
        
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=gbk\r\n"
            "Connection: close\r\n"
            "\r\n";
        send(clientSock, response.c_str(), response.length(), 0);
        
        thpool.add_task(func, clientSock, requestStr);
      }
      close(sock);//�رշ������׽���
      return *this;
    }
    
    Server &add_cmd(const std::string &tag, const cmd::Cmd_func &func)
    {
      wxcmd.add_cmd(tag, func);
      return *this;
    }
    
    Server &add_rehabilitative_cmd(const std::string &tag, const cmd::Rehabilitative_cmd_func &func)
    {
      wxcmd.add_rehabilitative_cmd(tag, func);
      return *this;
    }
  
  private:
    void url_router(const int clientSock, const std::string &requestStr)
    {
      std::string firstLine = requestStr.substr(0, requestStr.find("\r\n"));
      firstLine = firstLine.substr(firstLine.find(' ') + 1);
      std::string url = firstLine.substr(0, firstLine.find(' '));
      
      url::Url req_url(url);
      std::string req_type = requestStr.substr(0, 4);
      if (req_type == "GET ")
      {
        std::string msg_sig = req_url["msg_signature"];
        std::string timestamp = req_url["timestamp"];
        std::string nonce = req_url["nonce"];
        std::string req_echostr = req_url["echostr"];
        std::string echostr = wxmsg.verify_url(msg_sig, timestamp, nonce, req_echostr);
        send(clientSock, echostr.c_str(), echostr.length(), 0);
        WS_NOTICE("verify url successfully\n");
      }
      if (req_type == "POST")
      {
        std::string notice;
        auto a = requestStr.find("<xml>");
        auto b = requestStr.find("</xml>");
        std::string temp = requestStr.substr(a, b + 6);
        xml::Xml req_xml(temp);
        
        std::string msg_sig = req_url["msg_signature"];
        std::string timestamp = req_url["timestamp"];
        std::string nonce = req_url["nonce"];
        
        std::string msg_encrypt = req_xml["Encrypt"];
        std::string req_msg = wxmsg.decrypt_msg(msg_sig, timestamp, nonce, msg_encrypt);
        xml::Xml plain_xml(req_msg);
        
        std::string content = plain_xml["Content"];
        std::string UserID = plain_xml["FromUserName"];
        notice += "UserID: " + UserID;
        notice += "|Content: " + content;
        if (tags.find(content) != tags.end())
        {
          wxcmd.send("text", tags[content], UserID);
          notice += "|Response: " + tags[content];
        }
        else if (content[0] == '/')
        {
          if (check_user(UserID))
            notice += "|Response: " + wxcmd.command(content, UserID);
          else
          {
            wxcmd.send("text", "Permission denied", UserID);
            notice += "|Response: Permission denied";
          }
        }
        WS_NOTICE(notice);
      }
      time_t now = time(nullptr);
      std::string dt = ctime(&now);
      dt.pop_back();//'\n'
      close(clientSock);
    }
    
    inline bool check_user(const std::string &id)
    {
      if (admin.find(id) == admin.end())
        return false;
      return admin[id];
    }
  };
}
#endif