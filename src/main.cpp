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
#include "wsserver.hpp"
#include "wscmd.hpp"
#include "wsoption.hpp"
#include <string>
#include <filesystem>
int main(int argc, char **argv)
{
  ws::server::Server server;
  ws::option::Option option(argc, argv);
  option.add("c", "config",
             [&server](ws::option::Option::CallbackArgType args)
             {
               server.init(args[0]);
             }, 10);
  option.parse().run();
  
  server.add_cmd("file",
                 [](const std::string &args) -> ws::cmd::Cmd_ret { return {"file", args}; });
  
  server.add_cmd("time",
                 [](const std::string &args) -> ws::cmd::Cmd_ret
                 {
                   std::string res;
                   if (!args.empty())
                     res += "invalid argument('" + args + "')" + "\n";
                   time_t now = time(nullptr);
                   res += ctime(&now);
                   return {"text", res};
                 });
  server.run();
  return 0;
}
