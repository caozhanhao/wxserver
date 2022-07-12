#include "wsserver.h"
#include "wscmd.h"
#include "wsoption.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <filesystem>
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
  server.add_cmd("cam",
                 [](const std::string& args) -> ws::cmd::Cmd_ret
                 {
                   cv::VideoCapture capture(0);
                   cv::Mat Frame;
                   capture>>Frame;
                   std::string filename = args + ".jpg";
                   imwrite(filename.c_str(), Frame);
                   return {"file", filename};
                 })
                 .add_rehabilitative_cmd("cam",
                 [](const std::string& args)
                 {
                   std::filesystem::path p(args + ".jpg");
                   std::filesystem::remove(p);
                 });
  server.run();
  return 0;
}
