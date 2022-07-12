#pragma once
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdio>

#define WS_STRINGFY(x) _WS_STRINGFY(x)
#define _WS_STRINGFY(x) #x
#define WS_LOCATION  __FILE__ ":" WS_STRINGFY(__LINE__)

#define _WS_LOGGER(level, msg) \
ws::logger::Logger().get()\
<< "[" << level << "] " << ws::logger::get_time()\
<< " " << WS_LOCATION \
<< " " << msg << "\n";

#define WS_WARNING(msg) do{_WS_LOGGER("WARNING", msg);}while(0)
#define WS_DEBUG(msg) do{_WS_LOGGER("DEBUG", msg);}while(0)
#define WS_FATAL(msg, errorcode) do{_WS_LOGGER("FATAL", msg); exit(errorcode);} while(0)
#define WS_NOTICE(msg) do{ws::logger::Logger().get()\
<< "[NOTICE] " << ws::logger::get_time() << " " << msg << "\n"; }while(0)
namespace ws::logger
{
  std::string get_time()
  {
    auto tt = std::chrono::system_clock::to_time_t
        (std::chrono::system_clock::now());
    struct tm* ptm = localtime(&tt);
    char date[60] = { 0 };
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
  }
  class Logger
  {
  public:
    std::ostream& get() { return std::cout; }
  private:
  };
}