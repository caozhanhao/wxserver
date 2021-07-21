#pragma once

#include "wxerr.h"

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
    using CallbackArgType = std::vector<std::string>;
    using CallbackType = std::function<void(CallbackArgType)>;
  private:
    class Token
    {
    public:
      std::string arg;
      std::vector<std::string> values;
    public:
      Token(const std::string& arg_, const std::initializer_list<std::string>& values_ = {})
          : arg(arg_), values(values_) {  }
      void add(const std::string& v) { values.emplace_back(v); }
    };
    class Packed
    {
    private:
      std::function<void()> packed;
    public:
      int priority;
    public:
      Packed(const std::function<void()>& packed, int priority_)
          : packed(packed), priority(priority_) {}
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
      Callback(const CallbackType& func_, int priority_)
          : func(func_), priority(priority_) {}
      Callback() : func([](CallbackArgType) {}), priority(-1) {  }
      Packed pack(CallbackArgType arg)
      {
        return Packed(std::bind(func, arg), priority);
      }
    };
  private:
    std::vector<Token> tokens;
    std::map<const std::string, const std::string> alias;
    std::map<const std::string, Callback> funcs;
    std::vector<Packed> tasks;
    int argc;
    char** argv;
    bool parsed;
  public:
    Option(int argc_, char** argv_)
        : argc(argc_), argv(argv_), parsed(false) { }
    Option& add(const std::string& arg, const CallbackType& func, int priority = -1)
    {
      if (parsed)
        throw error::Error(WS_ERROR_LOCATION, __func__, "Can not add() after parse().");
      funcs.insert(std::make_pair(arg, Callback(func, priority)));
      return *this;
    }
    Option& add(const std::string& arg, const std::string& alias_, const CallbackType& func, int priority = -1)
    {
      if (parsed)
        throw error::Error(WS_ERROR_LOCATION, __func__, "Can not add() after parse().");
      funcs.insert(std::make_pair(arg, Callback(func, priority)));
      alias.insert(std::make_pair(alias_, arg));
      return *this;
    }
    Option& run()
    {
      if (!parsed)
        throw error::Error(WS_ERROR_LOCATION, __func__, "Option has not parsed.");
      for (auto& r : tasks)
        r();
      return *this;
    }
    Option& parse()
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
      
      for (auto& r : tokens)
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
      std::sort(tasks.begin(), tasks.end(), [](const Packed& p1, const Packed& p2) {return p1.priority > p2.priority; });
      
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
              it = 1 + tokens.insert(it, Token(std::string(1, it->arg[i]), { it->values[i] }));
            else
              it = 1 + tokens.insert(it, Token(std::string(1, it->arg[i])));
          }
          tokens.erase(it);
        }
      }
    }
    bool is_multi(const std::string& str)
    {
      if (funcs.find(str) != funcs.cend() || alias.find(str) != alias.cend())
        return false;
      for (auto& r : str)
      {
        if (funcs.find(std::string(1, r)) == funcs.cend() && alias.find(std::string(1, r)) == alias.cend())
          return false;
      }
      return true;
    }
  };
}