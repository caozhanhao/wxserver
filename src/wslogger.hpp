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
#ifndef WXSERVER_LOGGER_HPP
#define WXSERVER_LOGGER_HPP
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
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int) ptm->tm_year + 1900, (int) ptm->tm_mon + 1, (int) ptm->tm_mday,
            (int) ptm->tm_hour, (int) ptm->tm_min, (int) ptm->tm_sec);
    return std::string(date);
  }
  
  class Logger
  {
  public:
    std::ostream &get() { return std::cout; }
  
  private:
  };
}
#endif