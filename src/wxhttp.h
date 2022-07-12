#pragma once

#include "wxerr.h"

#include <curl/curl.h>
#include <iostream>
#include <string>
//libcurl��https://curl.se/libcurl/c/libcurl-tutorial.html
//C++��libcurl��ȡ����CURLcodeתΪstd::string��https://blog.csdn.net/qq_42311391/article/details/105159480
namespace ws::http
{
  size_t WriteCallback(char* contents, size_t size, size_t nmemb, void* userp)
  {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
  }
  

  class Http
  {
  public:
    static const bool is_postdata = true;
    static const bool is_path = false;
  private:
    std::string url;
  public:
    explicit Http(std::string _url) : url(std::move(_url)) {  }
    std::string POST(const std::string& str, bool postdata_or_path)//true postdata
    {
      if (str.empty())
        throw error::Error(WS_ERROR_LOCATION, __func__, std::string(postdata_or_path ? "postdata" : "path") + "is empty");

      CURL* curl = nullptr;
      std::string readBuffer;
      curl = curl_easy_init();
      if (curl == nullptr)
        throw error::Error(WS_ERROR_LOCATION, __func__, "curl_easy_init() failed");
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      if (postdata_or_path)
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str.c_str());
      curl_easy_setopt(curl, CURLOPT_POST, 1);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
      struct curl_slist* headerlist = nullptr;
      struct curl_httppost* curlFormPost = nullptr;
      struct curl_httppost* curlLastPtrFormPost = nullptr;

      if (!postdata_or_path)
      {
        curl_formadd(&curlFormPost, &curlLastPtrFormPost,
          CURLFORM_COPYNAME, "image",
          CURLFORM_FILE, str.c_str(), CURLFORM_END);

        curl_easy_setopt(curl, CURLOPT_HTTPPOST, curlFormPost);

        headerlist = curl_slist_append(headerlist, "Content-Type: multipart/form-data");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

      }

      CURLcode res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);

      if (!postdata_or_path)
      {
        curl_formfree(curlFormPost);
        curl_slist_free_all(headerlist);
      }
      
      if (res != CURLE_OK)
      {
        throw error::Error(WS_ERROR_LOCATION, __func__, "CURLcode is not CURLE_OK");
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
