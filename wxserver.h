#pragma once

#include "wxerr.h"
#include "wxmsg.h"
#include "wxurl.h"
#include "wxxml.h"
#include "wxhttp.h"
#include "czh-cpp/czh.h"

#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <map>
#include <functional>
//企业微信api：https://work.weixin.qq.com/api/doc/90001/90143/90372
namespace ws
{
  class WXserver
  {
  private:
    int port;
    WXmsg wxmsg;
    std::map<std::string, std::string> tags;
    std::map<std::string, std::string> admin;
    std::map<std::string, std::string> command;
    std::string access_token;
    std::string corpid;
    std::string corpsecret;
  public:
    WXserver(const std::string& path)
    {
      CZH::CZH config(path);

      std::string token = config.in("config")["Token"].get_value<std::string>();
      std::string encoding_aes_key = config.in("config")["EncodingAESKey"].get_value<std::string>();

      corpid = config.in("config")["CorpID"].get_value<std::string>();
      corpsecret = config.in("config")["CorpSecret"].get_value<std::string>();
      port = config.in("config")["Port"].get_value<int>();

      wxmsg = WXmsg(token, encoding_aes_key, corpid);

      tags = config.in("tags").value_map<std::string>();
      admin = config.in("admin").value_map<std::string>();
      command = config.in("command").value_map<std::string>();
    }
    void run()
    {
      int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//建立套接字，失败返回-1
      sockaddr_in addr;
      addr.sin_family = AF_INET; //指定地址族
      addr.sin_addr.s_addr = INADDR_ANY;//IP初始化
      addr.sin_port = htons(port);//端口号初始化

      bind(sock, (sockaddr*)&addr, sizeof(addr));//分配IP和端口
      listen(sock, 0);//设置监听

      //设置客户端
      sockaddr_in clientAddr;
      int clientAddrSize = sizeof(clientAddr);
      int clientSock;
      //接受客户端请求
      std::cout << "Server started successfully\n";
      while (-1 != (clientSock = accept(sock, (sockaddr*)&clientAddr, (socklen_t*)&clientAddrSize)))
      {
        // 收请求
        std::string requestStr;
        int bufSize = 4096;
        requestStr.resize(bufSize);
        //接受数据
        recv(clientSock, &requestStr[0], bufSize, 0);

        //发送响应头
        std::string response =
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html; charset=gbk\r\n"
          "Connection: close\r\n"
          "\r\n";
        send(clientSock, response.c_str(), response.length(), 0);

        // std::cout << "requestStr: \n" << requestStr << "\n";
        url_router(clientSock, requestStr);

        time_t now = time(0);
        char* dt = ctime(&now);
        std::cout << "time: " << dt;

        std::cout << "---------------completed---------------\n";
        close(clientSock);//关闭客户端套接字
      }
      close(sock);//关闭服务器套接字
    }
    void wxsend(std::string type, const std::string& msg, const std::string& id)
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
    inline bool check_user(const std::string& id)
    {
      if (admin.find(id) == admin.end())
        return false;
      else
      {
        if (admin[id] == "true")
          return true;
        else
          return false;
      }
    }
    void wxcmd(const std::string& command, const std::string& id)
    {
      int i = int(command.find(" "));
      std::string cmd = command.substr(0, i);
      if (cmd == "/file")
      {
        std::string path = command.substr(i + 1);
        wxsend("file", path, id);
      }
    }
    void url_router(int clientSock, std::string& requestStr)
    {
      std::string firstLine = requestStr.substr(0, requestStr.find("\r\n"));
      firstLine = firstLine.substr(firstLine.find(" ") + 1);
      std::string url = firstLine.substr(0, firstLine.find(" "));

      WXurl req_url(url);
      req_url.deescape();
      std::string req_type = requestStr.substr(0, 4);
      if (req_type == "GET ")//应该有空格，GET是验证URL，详情见api文档
      {

        std::string msg_sig = req_url.parse("msg_signature");
        std::string timestamp = req_url.parse("timestamp");
        std::string nonce = req_url.parse("nonce");
        std::string req_echostr = req_url.parse("echostr");
        std::string echostr = wxmsg.verify_url(msg_sig, timestamp, nonce, req_echostr);
        send(clientSock, echostr.c_str(), echostr.length(), 0);
        std::cout << "verify url success\n";
      }
      if (req_type == "POST")//POST是收到回复，详情见api文档
      {
        auto a = requestStr.find("<xml>");
        auto b = requestStr.find("</xml>");
        requestStr = requestStr.substr(a, b + 6);
        WXxml req_xml(requestStr);

        std::string msg_sig = req_url.parse("msg_signature");
        std::string timestamp = req_url.parse("timestamp");
        std::string nonce = req_url.parse("nonce");
       
        auto cut = [](const std::string& str)->std::string{return str.substr(9, str.size() - 3 - 9);};

        std::string msg_encrypt = cut(req_xml.parse("Encrypt"));
        std::string req_msg = wxmsg.decrypt_msg(msg_sig, timestamp, nonce, msg_encrypt);
        WXxml plain_xml(req_msg);

        std::string content = cut(plain_xml.parse("Content"));
        std::cout << "content: " << content << "\n";

        std::string UserID = cut(plain_xml.parse("FromUserName"));
        std::cout << "UserID: " << UserID << "\n";

        if (tags.find(content) != tags.end())
        {
          wxsend("text", tags[content], UserID);
          std::cout << "res: " << tags[content] << "\n";
        }
        else if (content[0] == '/')
        {
          if (check_user(UserID))
            wxcmd(content, UserID);
          else
          {
            wxsend("text", "Permission denied", UserID);
            std::cout << "res: Permission denied" << "\n";
            return;
          }
        }
        else
          return;
      }
    }
  };
}
