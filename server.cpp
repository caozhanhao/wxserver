#include <iostream>
#include <string>
#include <fstream>
#include <ctime>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <yaml-cpp/yaml.h>
#include <curl/curl.h>

#include "WXBizMsgCrypt.h"
#include "Url.h"


std::string cut_xml(std::string const & requestStr,std::string const & tag, bool type) 
    //type == true 含有<xxxx> </xxxx> ,false不含
{
    int a = requestStr.find("<" + tag + ">");
    int b = requestStr.find("</" + tag + ">");
    std::string result;
    if(type)
    {
        int i = b + tag.size() + 2;
        result = requestStr.substr(a,i - a + 1);
    }
    else
    {
        int i = a + tag.size() + 2;
        result = requestStr.substr(i, b - i);
    }
    return result;
}
void wxsend(const std::string type,const std::string item,const std::string UserID)//别动空格
{
    if (type != "text")
    {
        std::string c1 = "./wxsend text " + type + ":" + item + " " + UserID;
        int r1 = system(c1.c_str());
        if (r1 != 0)
        {
            std::cout << "Send error: " << r1 << "\n";
            return;
        }
    }
    std::string c2 = "./wxsend " + type + " " + item + " " + UserID;
    int r2 = system(c2.c_str());
    if (r2 != 0)
    {
        std::cout << "Send error: " << r2 << "\n";
        std::string c3 = "./wxsend text send_error " + UserID;
        system(c3.c_str());
        return;
    }
}
int make_res(std::string &msg, std::string type, std::string item,std::string &resp)
{
    if (type == "text")
    {
        std::string temp1 = cut_xml(msg, "FromUserName", false);  // 被动回复xml FormUserName改为ToUserName
        std::string ToUserName = "<ToUserName>" + temp1 + "</ToUserName>";

        std::string temp2 = cut_xml(msg, "ToUserName", false);
        std::string FromUserName = "<FromUserName>" + temp2 + "</FromUserName>";

        std::string CreateTime = cut_xml(msg, "CreateTime", true);
        std::string MsgType = "<MsgType><![CDATA[" + type + "]]></MsgType>";
        std::string Content = "<Content><![CDATA[" + item + "]]></Content>";

        resp = "<xml>" + ToUserName + FromUserName + CreateTime + MsgType + Content + "</xml>";

        return 0;
    }
    else
    {
        return -1;
    }
}
void wx_command(std::string UserID, std::string &content, 
    std::map<std::string, std::string> &admin, 
    std::map<std::string, std::string> &command)
{
    int i = content.find(" ");
    std::string com = content.substr(0, i);
    if (command.find(com) != command.end() && command[com] == "true")//第一个不符合不会检查 command[com] == "true" 也就不会插入UserID
    {
        if (admin.find(UserID) != admin.end())//在config.yaml中    
        {
            if (admin[UserID] == "true")//在config.yaml且值为true    
            {
                std::cout << " (管理员)" << "\n";
                std::string path = content.substr(i + 1);
                wxsend(com, path, UserID);
            }
            else //在config.yaml但值不为true    
            {
                std::cout << " (非管理员)" << "\n";
                std::string is_not_admin = "权限不足，请与管理员联系以获得相应权限。";
                wxsend("text", is_not_admin, UserID);
            }
        }
        else//不在config.yaml    
        {
            std::cout << " (未知)" << "\n";
            std::string is_not_admin = "找不到您的UserID，请与管理员联系。";
            wxsend("text", is_not_admin, UserID);
        }
    }
    else
    {
        wxsend("text", "undefined", UserID);
        if (admin.find(UserID) == admin.end())
        {
            std::cout << " (未知)" << "\n";
            return;
        }
        if (admin[UserID] == "true")
        {
            std::cout << " (管理员)" << "\n";
            return;
        }
        if (admin[UserID] != "true")
        {
            std::cout << " (非管理员)" << "\n";
            return;
        }
    }
}
void url_router(
        int clientSock,std::string & requestStr,Tencent::WXBizMsgCrypt wxcpt, 
        std::map<std::string, std::string> tags, 
        std::map<std::string, std::string> admin,
        std::map<std::string, std::string> command)
{
    std::string firstLine = requestStr.substr(0, requestStr.find("\r\n"));
    firstLine = firstLine.substr(firstLine.find(" ") + 1);
    std::string url = firstLine.substr(0, firstLine.find(" "));

    Url::Url req_url = { url };
    req_url.deescape();
    std::string req_type = requestStr.substr(0,4);
    if(req_type == "GET ")//应该有空格，GET是验证URL，详情见api文档
    {
        int ret = 0;
        std::string echostr;
        
        std::string msg_sig = req_url.parse("msg_signature");
        std::string timestamp = req_url.parse("timestamp");
        std::string nonce = req_url.parse("nonce");
        std::string req_echostr = req_url.parse("echostr");

        std::cout << "msg_sig: " << msg_sig << "\n";
        std::cout << "timestamp: " << timestamp << "\n";
        std::cout << "nonce: " << nonce << "\n";
        std::cout << "req_echostr: " << req_echostr << "\n";

        ret = wxcpt.VerifyURL(msg_sig, timestamp, nonce, req_echostr, echostr);
        if( ret!=0 )
        {
            std::cout<<"ERR: VerifyURL ret: "<< ret << "\n";
            return;
        }
        send(clientSock, echostr.c_str(), echostr.length(), 0);
    }
    if(req_type == "POST")//POST是收到回复，详情见api文档
    {
        int ret = 0;
        std::string msg_sig = req_url.parse("msg_signature");
        std::string timestamp = req_url.parse("timestamp");
        std::string nonce = req_url.parse("nonce");

        std::string req_msg = cut_xml(requestStr, "xml", true);

        std::string msg;  // 解密之后的明文

        ret = wxcpt.DecryptMsg(msg_sig, timestamp, nonce, req_msg, msg);
        if( ret!=0 )
        {
            std::cout << "ERR: DecryptMsg ret: " << ret << "\n";
            return;
        }
        std::string content;
        if( 0!= wxcpt.GetXmlField(msg, "Content", content) )
        {
            std::cout<< "Post data Format ERR: " << Tencent::WXBizMsgCrypt_ParseXml_Error << "\n";
            return;
        }
        std::cout<<"content: "<<content<< "\n";
        
        std::string UserID = cut_xml(msg, "FromUserName", false);
        int j = UserID.find("[");
        UserID = UserID.substr(j + 7);
        int k = UserID.find("]");
        UserID = UserID.substr(0,k);
        std::cout << "UserID: " << UserID;//不换行，与wx_command()配合

        if(tags.find(content) != tags.end())
        {
            std::string resp; // 被动回复回复明文
            int make_res_r = make_res(msg, "text", tags[content], resp);
            if (make_res_r != 0)
                std::cout << "make_res_error_code: " << make_res_r << "\n";

            std::string sEncryptMsg;// 被动回复密文
            ret = wxcpt.EncryptMsg(resp, timestamp, nonce, sEncryptMsg);
            if( ret!=0 )
            {
                std::cout<<"ERR: EncryptMsg ret: "<< ret << "\n";;
                return;
            }
            
            send(clientSock, sEncryptMsg.c_str(), sEncryptMsg.length(), 0);
            std::cout << "res: " << tags[content] << "\n";
        }
        else
        {
            wx_command(UserID, content, admin, command);
        }
    }
}

Tencent::WXBizMsgCrypt load_config(int & PORT, 
    std::map<std::string, std::string> & tags,                                            
    std::map<std::string, std::string>& admin,
    std::map<std::string, std::string>& command)
{
    
    YAML::Node config = YAML::LoadFile("config.yaml"); 
    for(YAML::const_iterator it = config["tags"].begin(); it != config["tags"].end(); ++it)    
    {
        tags.insert({it->first.as<std::string>(), it->second.as<std::string>()});
    }
    for (YAML::const_iterator it = config["admin"].begin(); it != config["admin"].end(); ++it)
    {
        admin.insert({ it->first.as<std::string>(), it->second.as<std::string>() });
    }
    for (YAML::const_iterator it = config["command"].begin(); it != config["command"].end(); ++it)
    {
        command.insert({ it->first.as<std::string>(), it->second.as<std::string>() });
    }
    std::string sToken = config["Token"].as<std::string>();
    std::string sCorpID = config["CorpID"].as<std::string>();
    std::string sCorpSecret = config["CorpSecret"].as<std::string>();
    std::string sEncodingAESKey = config["EncodingAESKey"].as<std::string>();

    PORT= config["Port"].as<int>();
    Tencent::WXBizMsgCrypt wxcpt(sToken, sEncodingAESKey, sCorpID);
    return wxcpt;

}
int main()
{
    int PORT; 
    std::map<std::string, std::string> tags;    
    std::map<std::string, std::string> admin;    
    std::map<std::string, std::string> command;
   
    Tencent::WXBizMsgCrypt wxcpt = load_config(PORT, tags, admin, command);
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//建立套接字，失败返回-1
    sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET; //指定地址族
    addr.sin_addr.s_addr = INADDR_ANY;//IP初始化
    addr.sin_port = htons(PORT);//端口号初始化

    int rc;
    rc = bind(sock, (sockaddr*)&addr, sizeof(addr));//分配IP和端口

    rc = listen(sock, 0);//设置监听

    //设置客户端
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    int clientSock;
    //接受客户端请求
    std::cout << "Server started successfully\n";
    while (-1 != (clientSock = accept(sock,(sockaddr*)&clientAddr, (socklen_t*)&clientAddrSize)))
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
        url_router(clientSock, requestStr, wxcpt, tags, admin, command);
        
        time_t now = time(0);
        char* dt = ctime(&now);
        std::cout << "time: " << dt;
        
        std::cout << "---------------completed---------------\n";
        close(clientSock);//关闭客户端套接字
    }
    close(sock);//关闭服务器套接字
    return 0;
}
