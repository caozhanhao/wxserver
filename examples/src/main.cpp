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
#include <future>

int main()
{
  ws::Server server;
  auto config = ws::parse_config("config.czh");
  server.load_config(config);
  ws_example::HuggingFace hf(config["hugging_face"]["model"].get<std::string>(),
                             config["hugging_face"]["token"].get<std::string>());
  ws_example::ChatGPT chatgpt(config["openai"]["model"].get<std::string>(),
                              config["openai"]["token"].get<std::string>());
  
  server.add_msg_handle(
      [&server, &chatgpt](const ws::Request &req, ws::Response &res)
      {
        if (req.content == "clear conversation")
        {
          chatgpt.clear_conversation();
          res.set_text("Cleared all conversations.");
        }
        else if (!req.content.empty())
        {
          // asynchronous reply
          std::thread([req, &server, &chatgpt]()
                      {
                        auto ret = chatgpt.input(req.content);
                        server.send_text(ret, req.user_id);
                      }).detach();
        }
      });
  server.run();
  return 0;
}
