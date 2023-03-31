//   Copyright 2021 - 2023 wxserver - caozhanhao
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
#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <chrono>
#include <thread>
#include <experimental/source_location>

namespace ws
{
  std::string location_to_str(const std::experimental::source_location &l)
  {
    return std::string(l.file_name()) + ":" + std::to_string(l.line()) +
           ":" + l.function_name() + "()";
  }
  
  enum class Severity
  {
    NONE,
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERR,
    CRITICAL,
  };
  
  enum class Output
  {
    file, console, file_and_console, none
  };
  
  std::string get_severity_str(Severity severity)
  {
    switch (severity)
    {
      case Severity::TRACE:
        return "[TRACE]";
      case Severity::DEBUG:
        return "[DEBUG]";
      case Severity::INFO:
        return "[INFO]";
      case Severity::WARN:
        return "\x1B[93m[WARNING]\x1B[0m\x1B[0K";
      case Severity::ERR:
        return "\x1B[91m[ERROR]\x1B[0m\x1B[0K";
      case Severity::CRITICAL:
        return "\x1B[97m\x1B[41m[CRITICAL]\x1B[0m\x1B[0K";
      default:
        return "[NONE]";
    }
  }
  
  std::string time_to_str(const std::chrono::system_clock::time_point &now)
  {
    auto tt = std::chrono::system_clock::to_time_t(now);
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int) ptm->tm_year + 1900, (int) ptm->tm_mon + 1, (int) ptm->tm_mday,
            (int) ptm->tm_hour, (int) ptm->tm_min, (int) ptm->tm_sec);
    return {date};
  }
  
  class Record
  {
  private:
    std::chrono::system_clock::time_point time_point;
    Severity severity;
    size_t thread_id;
    std::experimental::source_location location;
    std::string message;
  public:
    Record(std::chrono::system_clock::time_point time_point_, Severity severity_,
           std::experimental::source_location location_)
        : time_point(time_point_), severity(severity_), location(location_),
          thread_id(std::hash<std::thread::id>{}(std::this_thread::get_id()))
    {
    }
    
    Severity get_severity() const { return severity; }
    
    std::string get_message() const { return message; }
    
    std::string get_location() const { return location_to_str(location); }
    
    auto get_thread_id() const { return thread_id; }
    
    auto get_time_point() const { return time_point; }
    
    
    template<typename ...Args>
    void add_fmt(const std::string &fmt, Args &&...args)
    {
      // unfinished
      throw std::runtime_error("Unfinished.");
    }
    
    template<typename T>
    void add(T &&data)
    {
      using DataType = std::remove_cvref_t<T>;
      if constexpr (std::is_same_v<char, DataType>)
      {
        message += data;
      }
      else if constexpr (std::is_same_v<DataType, std::string>)
      {
        message += data;
      }
      else if constexpr (std::is_same_v<std::decay_t<DataType>, const char *>
                         || std::is_same_v<std::decay_t<DataType>, char *>
          )
      {
        message += data;
      }
      else
      {
        message += std::to_string(data);
      }
    }
    
    template<typename ...Args>
    void add(Args &&...args)
    {
      add_helper(std::forward<Args>(args)...);
    }
  
  private:
    template<typename T, typename ...Args>
    void add_helper(T &&f, Args &&...args)
    {
      add(std::forward<T>(f));
      add_helper(std::forward<Args>(args)...);
    }
    
    template<typename T>
    void add_helper(T &&f)
    {
      add(std::forward<T>(f));
    }
  };
  
  
  class Logger
  {
  private:
    Severity min_severity;
    Output mode;
    std::unique_ptr<std::ofstream> os;
  public:
    Logger()
        : min_severity(Severity::NONE), mode(Output::none), os(nullptr) {}
    
    
    void init(Severity min_severity_, Output mode_, const std::string &filename = "")
    {
      min_severity = min_severity_;
      mode = mode_;
      if (mode == Output::file_and_console || mode == Output::file)
      {
        os = std::make_unique<std::ofstream>(filename);
        if (!os->good())
        {
          throw std::runtime_error("Open log file failed");
        }
      }
    }
    
    
    void add(const Record &record)
    {
      if (record.get_severity() < min_severity) return;
      std::string str;
      std::string time = time_to_str(record.get_time_point());
      str += get_severity_str(record.get_severity()) + " ";
      str += time + " ";
      if (record.get_severity() != Severity::INFO)
      {
        str += std::to_string(record.get_thread_id()) + " ";
        str += record.get_location() + " ";
      }
      str += record.get_message() + "\n";
      
      if (mode == Output::file || mode == Output::file_and_console)
      {
        *os << str;
        os->flush();
      }
      
      if (mode == Output::console || mode == Output::file_and_console)
      {
        std::cout << str;
        std::cout << std::flush;
      }
      
      if (record.get_severity() == Severity::CRITICAL)
      {
        std::terminate();
      }
    }
    
    auto get_mode() const { return mode; }
  };
  
  static Logger &get_logger_instance()
  {
    static Logger instance;
    return instance;
  }
  
  inline void init_logger(Severity min_severity, Output mode, std::string filename = "")
  {
    get_logger_instance().init(min_severity, mode, filename);
  }
  
  class FormatWithLoc
  {
  private:
    std::string fmt;
    std::experimental::source_location loc;
  public:
    FormatWithLoc(const std::string &fmt_, std::experimental::source_location loc_
    = std::experimental::source_location::current())
        : fmt(fmt_), loc(loc_)
    {
    }
    
    FormatWithLoc(const char *fmt_, std::experimental::source_location loc_
    = std::experimental::source_location::current())
        : fmt(fmt_), loc(loc_)
    {
    }
    
    std::string get_fmt() const
    {
      return fmt;
    }
    
    std::experimental::source_location get_loc() const
    {
      return loc;
    }
  };
  
  template<typename ...Args>
  void log_helper(Severity severity, const FormatWithLoc &fmt, Args &&...args)
  {
    if (get_logger_instance().get_mode() == Output::none)
    {
      return;
    }
    Record rec(std::chrono::system_clock::now(), severity, fmt.get_loc());
    if (fmt.get_fmt().empty())
    {
      rec.add(std::forward<Args>(args)...);
    }
    else
    {
      rec.template add_fmt(fmt.get_fmt(), std::forward<Args>(args)...);
    }
    get_logger_instance().add(rec);
  }
  
  const std::string no_fmt = "";
  
  template<typename ...Args>
  void trace(const FormatWithLoc &fmt, Args &&...args)
  {
    log_helper(Severity::TRACE, fmt, std::forward<Args>(args)...);
  }
  
  template<typename ...Args>
  void debug(const FormatWithLoc &fmt, Args &&...args)
  {
    log_helper(Severity::DEBUG, fmt, std::forward<Args>(args)...);
  }
  
  template<typename ...Args>
  void info(const FormatWithLoc &fmt, Args &&...args)
  {
    log_helper(Severity::INFO, fmt, std::forward<Args>(args)...);
  }
  
  template<typename ...Args>
  void warn(const FormatWithLoc &fmt, Args &&...args)
  {
    log_helper(Severity::WARN, fmt, std::forward<Args>(args)...);
  }
  
  template<typename ...Args>
  void error(const FormatWithLoc &fmt, Args &&...args)
  {
    log_helper(Severity::ERR, fmt, std::forward<Args>(args)...);
  }
  
  template<typename ...Args>
  void critical(const FormatWithLoc &fmt, Args &&...args)
  {
    log_helper(Severity::CRITICAL, fmt, std::forward<Args>(args)...);
  }
}
#endif