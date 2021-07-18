#include "wxerr.h"
#include "wxserver.h"
#include "wxcmd.h"

#include <iostream>
#include <string>

int main(int argc, char **argv)
{ 
  std::string path;
  for (int i = 1; i < argc; i++)
  {
    if(std::string("-c") == argv[i])
    {
      if(i + 1 < argc && argv[i + 1][0] != '-')
      {
        path = argv[i + 1];
        i++;
        continue;
      }
    }
    else 
    {
      std::cout << "invalid argument('" + std::string(argv[i])  + "')";
      return -1;
    }
  }
  if(path == "")
  {
    std::cout << "Please use '-c' to define config file";
    return -1;
  }
  
  ws::WXserver server(path);

// 添加命令
  server.add_cmd("file",
  [](const std::string& args) -> ws::WXcmd_ret
  { return {"file", args}; });    
  
  server.add_cmd("time", 
  [](const std::string& args) -> ws::WXcmd_ret
  {
    std::string res = "";
    if(args != "")
      res += "invalid argument('" + args + "')" + "\n";
    time_t now = time(0);
    res += ctime(&now);
    return {"text", res};
  });

  // 
 
  server.run();
  return 0;
}
