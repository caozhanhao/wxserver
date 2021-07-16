#pragma once

#include <stdexcept>
#include <string>

#define WS_STRINGFY(x) _WS_STRINGFY(x)
#define _WS_STRINGFY(x) #x
#define WS_ERROR_LOCATION  __FILE__ ":" WS_STRINGFY(__LINE__) 
namespace ws
{
  class WXerr: public std::logic_error
  {
  public:
    std::string location;
    std::string func_name;
    std::string details;
    public:
    WXerr(std::string _location, std::string _func_name, std::string _details)
      :logic_error("Wxerr"), location(_location), func_name(_func_name), details(_details) {}
  };
}
