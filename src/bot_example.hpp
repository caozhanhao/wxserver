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
#ifndef WXSERVER_BOT_EXAMPLE_HPP
#define WXSERVER_BOT_EXAMPLE_HPP
#pragma once
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "wxserver.hpp"
#include "cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"
#include <string>
#include <vector>

namespace ws_example
{
  class Bot
  {
  private:
    std::string token;
    std::vector<std::string> past_user_inputs;
    std::vector<std::string> generated_responses;
  public:
    Bot(std::string api_token) : token(std::move(api_token)) {}
    
    std::string input(const std::string &user_input)
    {
      httplib::SSLClient cli("api-inference.huggingface.co");
      nlohmann::json body{{
                              "inputs", {
              {
                  "past_user_inputs", past_user_inputs
              },
              {
                  "generated_responses", generated_responses
              },
              {"text", user_input}
          }}};
      auto res = cli.Post("/models/facebook/blenderbot-400M-distill",
                          httplib::Headers{{"Authorization", "Bearer " + token}},
                          httplib::MultipartFormDataItems
                              {
                                  {"body", body.dump(), "", "text/plain"}
                              });
      if (res == nullptr)
      {
        ws::error(ws::no_fmt, "Bot failed. ", httplib::to_string(res.error()));
        return "An error occurred please try again later.";
      }
      if (res->status != 200)
      {
        ws::error(ws::no_fmt, "Bot failed. ", res->body);
        return "An error occurred please try again later.";
      }
      auto ret = nlohmann::json::parse(res->body)["generated_text"].get<std::string>();
      past_user_inputs.emplace_back(user_input);
      generated_responses.emplace_back(ret);
      ws::info(ws::no_fmt, "Bot | UserInput: ", user_input, " | Generated: ", ret);
      return ret;
    }
  };
}

#endif
