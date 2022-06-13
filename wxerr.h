#pragma once

#include <stdexcept>
#include <string>

#define WS_STRINGFY(x) _WS_STRINGFY(x)
#define _WS_STRINGFY(x) #x
#define WS_ERROR_LOCATION  __FILE__ ":" WS_STRINGFY(__LINE__) 
namespace ws::error
{
  const bool cant_continue = false;
  class Error: public std::logic_error
  {
  public:
    std::string location;
    std::string func_name;
    std::string details;
    bool can_continue;
    public:
    Error(std::string _location, std::string _func_name, std::string _details, bool _can_continue = true)
      :logic_error("Wxerr"), location(std::move(_location)), func_name(std::move(_func_name)), details(std::move(_details)), can_continue(_can_continue) {}
  };
}
