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
#ifndef WXSERVER_BOT_HPP
#define WXSERVER_BOT_HPP
#pragma once
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "wxserver/wxserver.hpp"
#include "cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"
#include <string>
#include <vector>

namespace ws_example
{
  class BotCli
  {
  protected:
    std::string token;
    std::string model;
    std::string proxy;
    int proxy_port;
  public:
    BotCli(std::string model_, std::string api_token)
      : token(std::move(api_token)), model(std::move(model_)), proxy_port(-1) {}

    void set_proxy(const std::string proxy_, int port)
    {
      proxy = proxy_;
      proxy_port = port;
    }
    
    void change_model(const std::string& m)
    {
      model = m;
    }

    virtual void clear_conversation(const std::string& userid) = 0;
    virtual std::string input(const std::string& userid, const std::string& user_input) = 0;
  };

  class HuggingFace : public BotCli
  {
  private:
    struct HFMsg
    {
      std::vector<std::string> past_user_inputs;
      std::vector<std::string> generated_responses;
    };
    std::map<std::string, HFMsg> messages;
  public:
    using BotCli::BotCli;
    virtual void clear_conversation(const std::string& userid) override
    {
      messages[userid].past_user_inputs.clear();
      messages[userid].generated_responses.clear();
    }
    
    virtual std::string input(const std::string& userid, const std::string& user_input) override
    {
      httplib::SSLClient cli("api-inference.huggingface.co");
      nlohmann::json body{{
                              "inputs", {
              {
                  "past_user_inputs", messages[userid].past_user_inputs
              },
              {
                  "generated_responses", messages[userid].generated_responses
              },
              {"text", user_input}
          }}};
      if (proxy_port != -1)
      {
        cli.set_proxy(proxy, proxy_port);
      }
      cli.set_read_timeout(50, 0);
      auto res = cli.Post("/models/" + model,
                          httplib::Headers{{"Authorization", "Bearer " + token}},
                          body.dump(), "application/json");
      if (res == nullptr)
      {
        ws::error(ws::no_fmt, "Bot failed. ", httplib::to_string(res.error()));
        return "An error occurred, please try again later.";
      }
      if (res->status != 200)
      {
        ws::error(ws::no_fmt, "Bot failed. ", res->body);
        return "An error occurred, please try again later.";
      }
      auto ret = nlohmann::json::parse(res->body)["generated_text"].get<std::string>();
      messages[userid].past_user_inputs.emplace_back(user_input);
      messages[userid].generated_responses.emplace_back(ret);
      ws::info(ws::no_fmt, "Bot | UserInput: ", user_input, " | Generated: ", ret);
      return ret;
    }
  };
  class ChatGPT : public BotCli
  {
  private:
    std::map<std::string, std::vector<nlohmann::json>> messages;
  public:
    using BotCli::BotCli;
    virtual void clear_conversation(const std::string& userid) override
    {
      messages[userid].clear();
    }
  
    virtual std::string input(const std::string& userid, const std::string& user_input) override
    {
      messages[userid].emplace_back(nlohmann::json{{"role",    "user"},
                                           {"content", user_input}});
      httplib::SSLClient cli("api.openai.com");
      nlohmann::json body{
          {"model",    model},
          {"messages", messages[userid]}
      };
      if (proxy_port != -1)
      {
        cli.set_proxy(proxy, proxy_port);
      }
      cli.set_read_timeout(50, 0);
      auto res = cli.Post("/v1/chat/completions",
                          httplib::Headers{{"Authorization", "Bearer " + token}},
                          body.dump(), "application/json");
      if (res == nullptr)
      {
        ws::error(ws::no_fmt, "Bot failed. ", httplib::to_string(res.error()));
        messages[userid].pop_back();
        return "An error occurred, please try again later. Error: \n" + httplib::to_string(res.error());
      }
      if (res->status != 200)
      {
        ws::error(ws::no_fmt, "Bot failed. ", res->body);
        messages[userid].pop_back();
        return "An error occurred, please try again later. Error: \n" + res->body;
      }
      auto ret = nlohmann::json::parse(res->body)["choices"][0]["message"]["content"].get<std::string>();
      messages[userid].emplace_back(nlohmann::json::parse(res->body)["choices"][0]["message"]);
      ws::info(ws::no_fmt, "Bot | UserInput: ", user_input, " | Generated: ", ret);
      return ret;
    }
  };
}
#endif
