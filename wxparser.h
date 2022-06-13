#pragma once
#include "tinyxml2.h"
#include "rapidjson/document.h"
#include "czh-cpp/czh.h"
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
      Json(const std::string &json_)
      {
        json.Parse(json_.c_str());
      }
  
      int get_errcode() const
      {
        return json["errcode"].GetInt();
      }
      
      std::string operator[](const std::string& tag) const
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
      Xml(const std::string &xml_)
      {
        xml.Parse(xml_.c_str());
      }
      
      std::string operator[](const std::string& tag)
      {
        tinyxml2::XMLElement* e = xml.RootElement();
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
      Url(const std::string& req_url) : url(req_url) {deescape();};
      
      std::string operator[](const std::string& tag)
      {
        int a = int(url.find(tag));
        if (a == -1)
          return "can not find";
        std::string temp = url.substr(a);
        int b = int(temp.find("&"));
        int c = int(temp.find("="));
        return  temp.substr(c + 1, b - c - 1);
      }
      std::string get()
      {
        return url;
      }
    private:
      Url deescape() {
        std::string result;
        for (unsigned int i = 0; i < url.length(); i++) {
          char c = url[i];
          if (c != '%') {
            result += c;
          }
          else {
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
      short int hexChar2dec(char c)
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
