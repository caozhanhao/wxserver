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
#include "bot.hpp"
#include "wxserver/wxserver.hpp"
#include <string>
#include <map>

int main()
{
  ws::Server server;
  auto config = ws::parse_config("config.czh");
  server.load_config(config);
  
  // ChatGPT
  ws_example::ChatGPT origin_bot(config["openai"]["model"].get<std::string>(),
                                 config["openai"]["token"].get<std::string>());
  if (!config["openai"]["proxy"].is<czh::value::Null>())
  {
    origin_bot.set_proxy(config["openai"]["proxy"].get<std::string>(),
                         config["openai"]["proxy_port"].get<int>());
  }
  //Hugging Face
//  ws_example::HuggingFace origin_bot(config["hugging_face"]["model"].get<std::string>(),
//                          config["hugging_face"]["token"].get<std::string>());
  
  std::map<std::string, ws_example::ChatGPT> bots;
  
  server.add_msg_handle(
      [&bots, origin_bot](const auto &req, auto &res)
      {
        if (bots.find(req.user_id) == bots.end())
        {
          bots[req.user_id] = origin_bot;
        }
        auto bot = bots[req.user_id];
        if (req.content == "clear conversation")
        {
          bot.clear_conversation();
          res.set_content(ws::MsgType::text, "Cleared all conversations.");
        }
        else if (!req.content.empty())
        {
          res.set_content(ws::MsgType::text, bot.input(req.content));
        }
      });
  server.run();
  return 0;
}