#include "wxserver.h"
#include "wxcmd.h"
#include "wxoption.h"
#include <string>
int main(int argc, char **argv)
{
  ws::server::Server server;
  std::string path;
  ws::option::Option option(argc, argv);
  option.add("c", "config",
             [&server](ws::option::Option::CallbackArgType args)
             {
                server.init(args[0]);
             }, 10);
  option.parse().run();
  
  server.add_cmd("file",
                 [](const std::string& args) -> ws::cmd::Cmd_ret
                 { return {"file", args}; });
  
  server.add_cmd("time",
                 [](const std::string& args) -> ws::cmd::Cmd_ret
                 {
                   std::string res;
                   if(!args.empty())
                     res += "invalid argument('" + args + "')" + "\n";
                   time_t now = time(nullptr);
                   res += ctime(&now);
                   return {"text", res};
                 });
  
  server.run();
  return 0;
}
