#include "wxerr.h"
#include "wxserver.h"
#include "wxcmd.h"

#include <iostream>
#include <string>

int main(int argc, char **argv)
{ 
  while(true)
  {
    try
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
          throw ws::WXerr(WS_ERROR_LOCATION, __func__, "invalid argument('" + std::string(argv[i])  + "')", ws::cant_continue);
      }
      if(path == "")
      throw ws::WXerr(WS_ERROR_LOCATION, __func__, "Please use '-c' to define config file", ws::cant_continue);

      ws::WXserver server(path);
/*
 *
 * 添加命令
 *
 */
    server.add_cmd("file",
    [](const std::string& args) -> ws::WXcmd_ret
    { return {"file", args}; });    

/**/ 
    server.run();
    }
    catch (ws::WXerr& err)
    {
      std::cout << err.func_name << "() at '" << err.location << "':\n" 
        << err.details << std::endl;
      if(!err.can_continue) return -1;
    }
  }
  return 0;
}
