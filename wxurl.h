#pragma once
#pragma once

#include <iostream>
#include <string>

namespace ws
{
  class WXurl
  {
  private:
    std::string url;
  public:
    WXurl(const std::string& req_url) :url(req_url) {};

    std::string parse(std::string const& tag)
    {
      int a = int(url.find(tag));
      if (a == -1)
        return "can not find";
      std::string temp = url.substr(a);
      int b = int(temp.find("&"));
      int c = int(temp.find("="));
      return  temp.substr(c + 1, b - c - 1);
    }
    WXurl deescape() {
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
    std::string get_url()
    {
      return url;
    }
  private:
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
