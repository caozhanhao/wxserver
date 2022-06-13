#pragma once

#include "wxerr.h"
#include "wxmsg.h"
#include "wxparser.h"
#include "wxthpool.h"
#include "wxcmd.h"

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
//��ҵ΢��api��https://work.weixin.qq.com/api/doc/90001/90143/90372
namespace ws::server
{
  class Server
  {
  private:
    bool inited;
    int port;
    msg::Msg wxmsg;
    cmd::Cmd wxcmd;
    std::map<std::string, std::string> tags;
    std::map<std::string, bool> admin;
  public:
    Server():inited(false), port(8345) {}
    Server& init(const std::string& path)
    {
      czh::Czh config_czh(path);
      auto config = *config_czh.parse();

      auto token = config["config"]["Token"].get<std::string>();
      auto encoding_aes_key = config["config"]["EncodingAESKey"].get<std::string>();
      auto corpid = config["config"]["CorpID"].get<std::string>();
      auto corpsecret = config["config"]["CorpSecret"].get<std::string>();
      
      port = config["config"]["Port"].get<int>();

      wxmsg = msg::Msg(token, encoding_aes_key, corpid);
      wxcmd = cmd::Cmd(corpid, corpsecret);

      tags = *config["tags"].value_map<std::string>();
      admin = *config["admin"].value_map<bool>();
      inited = true;
      return *this;
    }
    void run()
    {
      if(!inited) init("config.czh");
      int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//�����׽��֣�ʧ�ܷ���-1
      sockaddr_in addr {};
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = INADDR_ANY;
      addr.sin_port = htons(port);

      if(bind(sock, (sockaddr*)&addr, sizeof(addr)) != 0)
        throw error::Error(WS_ERROR_LOCATION, __func__, "bind() failed.");
      listen(sock, 0);//���ü���

      //���ÿͻ���
      sockaddr_in clientAddr{};
      int clientAddrSize = sizeof(clientAddr);
      int clientSock;
      //���ܿͻ�������
      thpool::Thpool thpool(16);
      std::function<void(const int,const std::string&)> func =
        [this](const int clientSock, const std::string& requestStr)
        {
          try
          {
            this->url_router(clientSock, requestStr);
          }
          catch (error::Error &err)
          {
            std::string out = "---\nError:" + err.func_name + "() at '" + err.location + "':\n" 
                              + err.details + "\n---\n";
            std::cout << out;
            if(!err.can_continue) exit(-1);
          }
        };
      std::cout << "Server started successfully\n";
      
      while (-1 != (clientSock = accept(sock, (sockaddr*)&clientAddr, (socklen_t*)&clientAddrSize)))
      {
        // ������
        std::string requestStr;
        int bufSize = 4096;
        requestStr.resize(bufSize);
        //��������
        recv(clientSock, &requestStr[0], bufSize, 0);

        //������Ӧͷ
        std::string response =
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html; charset=gbk\r\n"
          "Connection: close\r\n"
          "\r\n";
        send(clientSock, response.c_str(), response.length(), 0);

        thpool.add_task(func, clientSock, requestStr);
      }
      close(sock);//�رշ������׽���
    }
    void add_cmd(const std::string& tag, const cmd::Cmd_func& func)
    {
      wxcmd.add_cmd(tag, func);
    }
  private:
    void url_router(const int clientSock, const std::string& requestStr)
    {
      std::string out;
      std::string firstLine = requestStr.substr(0, requestStr.find("\r\n"));
      firstLine = firstLine.substr(firstLine.find(' ') + 1);
      std::string url = firstLine.substr(0, firstLine.find(' '));
    
      url::Url req_url(url);
      std::string req_type = requestStr.substr(0, 4);
      if (req_type == "GET ")//Ӧ���пո�GET����֤URL�������api�ĵ�
      {
      
        std::string msg_sig = req_url["msg_signature"];
        std::string timestamp = req_url["timestamp"];
        std::string nonce = req_url["nonce"];
        std::string req_echostr = req_url["echostr"];
        std::string echostr = wxmsg.verify_url(msg_sig, timestamp, nonce, req_echostr);
        send(clientSock, echostr.c_str(), echostr.length(), 0);
        out += "verify url success\n";
      }
      if (req_type == "POST")//POST���յ��ظ��������api�ĵ�
      {
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
        out += "content: " + content + "\n";
      
        std::string UserID = plain_xml["FromUserName"];
        out += "UserID: " + UserID + "\n";
      
        if (tags.find(content) != tags.end())
        {
          wxcmd.send("text", tags[content], UserID);
          out += "res: " + tags[content] += "\n";
        }
        else if (content[0] == '/')
        {
          if (check_user(UserID))
            out += "res: " + wxcmd.command(content, UserID) + "\n";
          else
          {
            wxcmd.send("text", "Permission denied", UserID);
            out += "res: Permission denied\n";
          }
        }
      }
      
      time_t now = time(nullptr);
      std::string dt = ctime(&now);
      dt.pop_back();//'\n'
      out += "---------------" + dt + "---------------\n";
      std::cout << out;
      close(clientSock);
    }
  
    inline bool check_user(const std::string& id)
    {
      if (admin.find(id) == admin.end())
        return false;
      return admin[id];
    }
  };
}
