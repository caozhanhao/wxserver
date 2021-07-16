#pragma once

#include "wxerr.h"

#include <curl/curl.h>
#include <iostream>
#include <string>
//libcurl：https://curl.se/libcurl/c/libcurl-tutorial.html
//C++把libcurl获取到的CURLcode转为std::string：https://blog.csdn.net/qq_42311391/article/details/105159480
namespace ws
{
  size_t WriteCallback(char* contents, size_t size, size_t nmemb, void* userp)
  {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
  }
  class WXhttp
  {
  private:
    std::string url;

  public:
    WXhttp(const std::string& _url) :url(_url) {  }
    std::string POST(const std::string& postdata, const std::string& path)
    {
      if (postdata == "" && path == "")
        throw WXerr(WS_ERROR_LOCATION, __func__, "postdata and path are empty");
      CURL* curl = NULL;
      std::string readBuffer;
      curl = curl_easy_init();
      if (curl == NULL)
        throw WXerr(WS_ERROR_LOCATION, __func__, "curl_easy_init failed");
      // 设置URL
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      // 设置参数
      if (postdata != "")
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
      // 设置为Post
      curl_easy_setopt(curl, CURLOPT_POST, 1);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
      struct curl_slist* headerlist = NULL;
        struct curl_httppost* curlFormPost = 0;
        struct curl_httppost* curlLastPtrFormPost = 0;
      if (path != "")
      {
        curl_formadd(&curlFormPost, &curlLastPtrFormPost,
          CURLFORM_COPYNAME, "image",
          CURLFORM_FILE, path.c_str(), CURLFORM_END);

        curl_easy_setopt(curl, CURLOPT_HTTPPOST, curlFormPost);

        headerlist = curl_slist_append(headerlist, "Content-Type: multipart/form-data");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

      }
      CURLcode res = curl_easy_perform(curl);
      if (res != CURLE_OK)
      {
        throw WXerr(WS_ERROR_LOCATION, __func__, "CURLcode is not CURLE_OK.");
      }
      // 清空
      curl_easy_cleanup(curl);
      if (path != "")
      {
        curl_formfree(curlFormPost);
        curl_slist_free_all(headerlist);
      }
      return readBuffer;
    }
    std::string GET()
    {
      CURL* curl = curl_easy_init();
      std::string readBuffer;
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
      curl_easy_perform(curl);
      return readBuffer;
    }
  };
}
