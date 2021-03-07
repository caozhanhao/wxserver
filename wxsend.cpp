#include <iostream>
#include <curl/curl.h>
#include <string>
#include <fstream>
#include <ctime>
#include <yaml-cpp/yaml.h>
//企业微信api：https://work.weixin.qq.com/api/doc/90001/90143/90372
//获取access_token：https://open.work.weixin.qq.com/api/doc/90000/90135/91039
//libcurl：https://curl.se/libcurl/c/libcurl-tutorial.html
//C++把libcurl获取到的CURLcode转为std::string：https://blog.csdn.net/qq_42311391/article/details/105159480

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
void get_access_token(std::string & access_token, std::string & corpid, std::string & corpsecret)//不能频繁调用gettoken接口，否则会受到频率拦截
{
    std::string url = "https://qyapi.weixin.qq.com/cgi-bin/gettoken?corpid="+ corpid + "&corpsecret=" + corpsecret;

    CURL *curl = curl_easy_init();
    std::string readBuffer;
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(curl, CURLOPT_URL,url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(curl);

    int k = readBuffer.find("access_token");
    std::string temp = readBuffer.substr(k + 15);
    int j = temp.find("\",\"expires_in\"");
    temp = temp.substr(0,j);
    access_token = temp;

    YAML::Node config = YAML::LoadFile("config.yaml");
    std::ofstream fout("config.yaml");
    config["access_token"] = access_token;
    fout << config;
    fout.close();
}
int wxsend_postdata(std::string & postdata, std::string & access_token)
{

    std::string url = "https://qyapi.weixin.qq.com/cgi-bin/message/send?access_token=" + access_token;

    CURL* curl = NULL;
    std::string readBuffer;
    curl = curl_easy_init();
    if(curl == NULL) 
        return -400;
    // 设置URL
    curl_easy_setopt(curl, CURLOPT_URL,url.c_str());
    // 设置参数
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,postdata.c_str());
    // 设置为Post
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        //可能由于access_token错误导致"curl_easy_perform() failed: URL using bad/illegal format or missing URL"
        if(access_token == "")
            return -2;
        else
            return -400;
    }
    // 清空
    curl_easy_cleanup(curl);
    
   //std::cout << readBuffer << "\n";
    
    //判断是否因access_token失效而发送失败
    int k = readBuffer.find(",");
    std::string i = readBuffer.substr(0,k);
    if (i == "{\"errcode\":41001"||i == "{\"errcode\":42001"||i == "{\"errcode\":40014")
    {
        //std::cout << "access_token已失效" << "\n";
        return -2;
    }
    else
    {
        if(i == "{\"errcode\":0")
            return 0;
        else
        {
            std::cout << "readBuffer: " << readBuffer << "\n";
            std::cout << "postdata: " << postdata << "\n";
            return -1;
        }
    }
}
int get_media_id(std::string type, std::string & path, std::string & access_token, std::string & media_id)
{

    std::string url = "https://qyapi.weixin.qq.com/cgi-bin/media/upload?access_token=" + access_token + "&type=" + type;

    CURL* curl = NULL;
    std::string readBuffer;
    curl = curl_easy_init();
    if(curl == NULL) 
        return -400;
    // 设置URL
    curl_easy_setopt(curl, CURLOPT_URL,url.c_str());
    // 设置为Post
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
   
	struct curl_slist *headerlist	= NULL; 
    struct curl_httppost *curlFormPost = 0;
    struct curl_httppost *curlLastPtrFormPost = 0;

	curl_formadd(&curlFormPost, &curlLastPtrFormPost, 
		CURLFORM_COPYNAME, "image", 
		CURLFORM_FILE, path.c_str(), CURLFORM_END);

    curl_easy_setopt(curl, CURLOPT_HTTPPOST, curlFormPost);
  
	headerlist = curl_slist_append(headerlist, "Content-Type: multipart/form-data");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);  

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        //可能由于access_token错误导致"curl_easy_perform() failed: URL using bad/illegal format or missing URL"
        if(access_token == "")
            return -2;
        else
            return -400;
    }
    // 清空
    curl_easy_cleanup(curl);
    // 释放表单
	curl_formfree(curlFormPost);
	// 释放表头
	curl_slist_free_all (headerlist);  

    //std::cout << readBuffer << "\n";
    
    
    //判断是否因access_token失效而发送失败
    int k = readBuffer.find(",");
    std::string i = readBuffer.substr(0,k);
    if (i == "{\"errcode\":41001"||i == "{\"errcode\":42001"||i == "{\"errcode\":40014")
    {
        std::cout << "access_token已失效" << "\n";
        return -2;
    }
    else
    {    
        if(i == "{\"errcode\":0")
        {
            int a = readBuffer.find("media_id");
            media_id = readBuffer.substr(a + 11);
            int b = media_id.find("\",\"created_at");
            media_id = media_id.substr(0,b);
            return 0;
        }
        else
        {
            std::cout << "readBuffer: " << readBuffer << "\n";
            std::cout << "path: " << path << "\n";
            return -1;
        }
    }   
}

int wxsend(std::string type, std::string item, std::string UserID)
{
    YAML::Node config = YAML::LoadFile("config.yaml");
    std::string corpid = config["CorpID"].as<std::string>();
    std::string corpsecret = config["CorpSecret"].as<std::string>();
    std::string access_token = config["access_token"].as<std::string>();

    std::string postdata;
    std::string send_error;
    if(type == "text")
    {
        postdata = R""({
           "touser" : ")"" + UserID + R""(",
            "msgtype" : "text",
            "agentid" : 1000002,                                                                                                                                                              
            "text" : {
                "content" : ")"" + item + R""("
                     },   
            "safe":0,
            "enable_id_trans": 0,
            "enable_duplicate_check": 0,
            "duplicate_check_interval": 1800
            })"";
    }
    else
    {
        std::string media_id;
        int ret = get_media_id(type, item, access_token, media_id);
        switch (ret)
        {
        case 0:
            std::cout << "get_media_id successfully" << "\n";
            break;
        case -2:
            get_access_token(access_token, corpid, corpsecret);
            ret = get_media_id(type, item, access_token, media_id);
            if (ret != 0)
            {
                std::cout << "get_media_id failed, error_code: " << ret << "\n";
                return -1;
            }
            break;
        case -1:
            std::cout << "get_media_id failed, error_code: -1" << "\n";
            return -1;
            break;
        case -400:
            std::cout << "curl failed, error_code: -400" << "\n";
            return -400;
            break;
        default:
            std::cout << "undefined error code" << "\n";
            return -3;
            break;
        }


        postdata = R""({
           "touser" : ")"" + UserID + R""(",
            "msgtype" : ")"" + type + R""(",
            "agentid" : 1000002,                                                                                                                                                              
            ")"" + type + R""(" : {
                "media_id" : ")"" + media_id + R""("
                     },   
            "safe":0,
            "enable_id_trans": 0,
            "enable_duplicate_check": 0,
            "duplicate_check_interval": 1800
            })"";
    
    }
    
        
    int ret = wxsend_postdata(postdata, access_token);
    switch (ret)
    {
    case 0:
        std::cout << "send_postdata successfully" << "\n";
        return 0;
        break;
    case -2:
        get_access_token(access_token, corpid, corpsecret);
        ret = wxsend_postdata(postdata, access_token);
        if (ret != 0)
        {
            std::cout << "send_postdata failed, error_code: " << ret << "\n";
            return -1;
        }
        break;
    case -1:
        std::cout << "send_postdata failed, error_code: -1" << "\n";
        return -1;
        break;
    case -400:
        std::cout << "curl failed, error_code: -400" << "\n";
        return -400;
        break;
    default:
        std::cout << "undefined error code" << "\n";
        return -3;
        break;
    }
}
int main(int argc, char* argv[])
{
    std::string type = argv[1];
    std::string item = argv[2];
    std::string UserID = argv[3];
    int ret = wxsend(type, item, UserID);
    if (ret != 0)
        return ret;
    else
        return 0;
}
