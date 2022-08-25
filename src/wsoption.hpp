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
#ifndef WXSERVER_WSOPTION_HPP
#define WXSERVER_WSOPTION_HPP
#include "wslogger.hpp"

#include <vector>
#include <functional>
#include <map>
#include <string>
#include <algorithm>
#include <cstring>
#include <iostream>

namespace ws::option
{
  class Option
  {
  public:
    using CallbackArgType = std::vector <std::string> &;
    using CallbackType = std::function<void(CallbackArgType)>;
  private:
    class Token
    {
    public:
      std::string arg;
      std::vector <std::string> values;
    public:
      explicit Token(std::string arg, const std::initializer_list <std::string> &values_ = {})
          : arg(std::move(arg)), values(values_) {}
      
      void add(const std::string &v) { values.emplace_back(v); }
    };
    
    class Packed
    {
    private:
      std::function<void()> packed;
    public:
      int priority;
    public:
      Packed(std::function<void()> packed, int priority_)
          : packed(std::move(packed)), priority(priority_) {}
      
      void operator()()
      {
        packed();
      }
    };
    
    class Callback
    {
    private:
      CallbackType func;
      int priority;
    public:
      Callback(CallbackType func, int priority)
          : func(std::move(func)), priority(priority) {}
      
      Callback() : func([](CallbackArgType) {}), priority(-1) {}
      
      Packed pack(CallbackArgType arg)
      {
        return {std::bind(func, arg), priority};
      }
    };
  
  private:
    std::vector <Token> tokens;
    std::map<const std::string, const std::string> alias;
    std::map<const std::string, Callback> funcs;
    std::vector <Packed> tasks;
    int argc;
    char **argv;
    bool parsed;
  public:
    Option(int argc_, char **argv_)
        : argc(argc_), argv(argv_), parsed(false) {}
    
    Option &add(const std::string &arg, const CallbackType &func, int priority = -1)
    {
      if (parsed)
        WS_FATAL("Can not add() after parse().", -1);
      funcs.insert(std::make_pair(arg, Callback(func, priority)));
      return *this;
    }
    
    Option &add(const std::string &arg, const std::string &alias_, const CallbackType &func, int priority = -1)
    {
      if (parsed)
        WS_FATAL("Can not add() after parse().", -1);
      funcs.insert(std::make_pair(arg, Callback(func, priority)));
      alias.insert(std::make_pair(alias_, arg));
      return *this;
    }
    
    Option &run()
    {
      if (!parsed)
        WS_FATAL("Option has not parsed.", -1);
      for (auto &r: tasks)
        r();
      return *this;
    }
    
    Option &parse()
    {
      tokens.emplace_back(Token(argv[0]));
      for (int i = 1; i < argc; i++)
      {
        if (argv[i][0] == '-' && strlen(argv[i]) != 1)
        {
          if (argv[i][1] == '-' && strlen(argv[i]) != 2)
            tokens.emplace_back(Token(std::string(std::string(argv[i]), 2)));//--x
          else
            tokens.emplace_back(Token(std::string(std::string(argv[i]), 1)));//-x
        }
        else
          tokens.back().add(std::string(argv[i]));//-
      }
      funcs.insert(std::make_pair(argv[0], Callback()));
      parse_multi();
      
      for (auto &r: tokens)
      {
        if (funcs.find(r.arg) != funcs.cend())
        {
          tasks.emplace_back(funcs[r.arg].pack(r.values));
          continue;
        }
        if (alias.find(r.arg) != alias.cend())
        {
          tasks.emplace_back(funcs[alias[r.arg]].pack(r.values));
          continue;
        }
        std::cout << "Unrecognized option '" << r.arg << "'." << std::endl;
      }
      std::sort(tasks.begin(), tasks.end(),
                [](const Packed &p1, const Packed &p2) { return p1.priority > p2.priority; });
      
      parsed = true;
      return *this;
    }
  
  private:
    void parse_multi()
    {
      for (auto it = tokens.cbegin(); it < tokens.cend(); it++)
      {
        if (is_multi(it->arg))
        {
          for (std::size_t i = 0; i < it->arg.size(); i++)
          {
            if (i < it->values.size())
              it = 1 + tokens.insert(it, Token(std::string(1, it->arg[i]), {it->values[i]}));
            else
              it = 1 + tokens.insert(it, Token(std::string(1, it->arg[i])));
          }
          tokens.erase(it);
        }
      }
    }
    
    bool is_multi(const std::string &str)
    {
      if (funcs.find(str) != funcs.cend() || alias.find(str) != alias.cend())
        return false;
      std::all_of(str.begin(), str.end(),
                  [this](char r)
                  {
                    return !(funcs.find(std::string(1, r)) == funcs.cend() &&
                             alias.find(std::string(1, r)) == alias.cend());
                  }
      );
      return true;
    }
  };
}
#endif