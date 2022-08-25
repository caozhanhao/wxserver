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
#ifndef WXSERVER_WSHTTP_HPP
#define WXSERVER_WSHTTP_HPP
#include "wslogger.hpp"

#include <curl/curl.h>
#include <iostream>
#include <string>
namespace ws::http
{
  size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
  {
    ((std::string *) userp)->append((char *) contents, size * nmemb);
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
    explicit Http(std::string _url) : url(std::move(_url)) {}
    
    std::string POST(const std::string &str, bool postdata_or_path)//true postdata
    {
      if (str.empty())
        WS_FATAL(std::string(postdata_or_path ? "postdata" : "path") + "is empty", -1);
      
      CURL *curl = nullptr;
      std::string readBuffer;
      curl = curl_easy_init();
      if (curl == nullptr)
        WS_FATAL("curl_easy_init() failed", -1);
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      if (postdata_or_path)
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str.c_str());
      curl_easy_setopt(curl, CURLOPT_POST, 1);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
      
      struct curl_slist *headerlist = nullptr;
      struct curl_httppost *curlFormPost = nullptr;
      struct curl_httppost *curlLastPtrFormPost = nullptr;
      
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
        WS_FATAL("CURLcode is not CURLE_OK", -1);
      }
      return readBuffer;
    }
    
    std::string GET()
    {
      CURL *curl = curl_easy_init();
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
#endif