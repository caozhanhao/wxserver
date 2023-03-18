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
#include "wxserver.hpp"
#include <string>
#include <future>
int main()
{
  ws::ConversationalBot bot(czh::Czh("hugging_face.czh", czh::InputMode::file).parse()["token"].get<std::string>());
  ws::Server server("config.czh");
  server.add_msg_handle(
      [&server, &bot](const ws::Request &req, ws::Response &res)
      {
        if (req.content == "license")
        {
          res.set_file("LICENSE");
        }
        if (req.content == "clear conversation")
        {
          bot.clear_conversation();
          res.set_text("cleared.");
        }
        else if (!req.content.empty())
        {
          // asynchronous reply
          std::thread([req, &server, &bot]()
                      {
                        auto ret = bot.input(req.content);
                        server.send_text(ret, req.user_id);
                      }).detach();
        }
      });
  server.run();
  return 0;
}
