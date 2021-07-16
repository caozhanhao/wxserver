#pragma once
#pragma once

#include <string>
namespace ws
{
  class WXxml
  {
  private:
    std::string xml;
  public:
    WXxml(const std::string& _xml) :xml(_xml) {  }
    std::string parse(std::string const& tag, bool type = false)
      //type == true º¬ÓÐ<xxxx> </xxxx> ,false²»º¬
    {
      int a = int(xml.find("<" + tag + ">"));
      int b = int(xml.find("</" + tag + ">"));
      std::string result;
      if (type)
      {
        int i = b + tag.size() + 2;
        result = xml.substr(a, i - a + 1);
      }
      else
      {
        int i = a + tag.size() + 2;
        result = xml.substr(i, b - i);
      }
      return result;
    }
  };
}
