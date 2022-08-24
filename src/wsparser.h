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
#ifndef WXSERVER_WSPARSER_H
#define WXSERVER_WSPARSER_H
#include "tinyxml2.h"
#include "rapidjson/document.h"
#include "../libczh/src/czh.hpp"
#include <string>
namespace ws
{
  namespace czh
  {
    using Czh = ::czh::Czh;
  }
  namespace json
  {
    class Json
    {
    private:
      rapidjson::Document json;
    public:
      explicit Json(const std::string &json_)
      {
        json.Parse(json_.c_str());
      }
      
      [[nodiscard]] int get_errcode() const
      {
        return json["errcode"].GetInt();
      }
      
      std::string operator[](const std::string &tag) const
      {
        return json[tag.c_str()].GetString();
      }
    };
  }
  namespace xml
  {
    class Xml
    {
    private:
      tinyxml2::XMLDocument xml;
    public:
      explicit Xml(const std::string &xml_)
      {
        xml.Parse(xml_.c_str());
      }
      
      std::string operator[](const std::string &tag)
      {
        tinyxml2::XMLElement *e = xml.RootElement();
        return e->FirstChildElement(tag.c_str())->GetText();
      }
    };
  }
  namespace url
  {
    class Url
    {
    private:
      std::string url;
    public:
      explicit Url(std::string req_url) : url(std::move(req_url)) { deescape(); };
      
      std::string operator[](const std::string &tag)
      {
        int a = int(url.find(tag));
        if (a == -1)
          return "can not find";
        std::string temp = url.substr(a);
        int b = int(temp.find('&'));
        int c = int(temp.find('='));
        return temp.substr(c + 1, b - c - 1);
      }
      
      std::string get()
      {
        return url;
      }
    
    private:
      Url deescape()
      {
        std::string result;
        for (unsigned int i = 0; i < url.length(); i++)
        {
          char c = url[i];
          if (c != '%')
          {
            result += c;
          }
          else
          {
            char c1 = url[++i];
            char c0 = url[++i];
            int num = 0;
            num += hexChar2dec(c1) * 16 + hexChar2dec(c0);
            result += char(num);
          }
        }
        url = result;
        return *this;
      }
      
      static short int hexChar2dec(char c)
      {
        if ('0' <= c && c <= '9')
        {
          return short(c - '0');
        }
        else if ('a' <= c && c <= 'f')
        {
          return (short(c - 'a') + short(10));
        }
        else if ('A' <= c && c <= 'F')
        {
          return (short(c - 'A') + short(10));
        }
        else
        {
          return -1;
        }
      }
      
    };
  }
}
#endif