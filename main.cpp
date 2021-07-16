#include "wxerr.h"
#include "wxserver.h"

#include <iostream>
#include <string>
using namespace std;
int main()
{
  ws::WXserver server("config.czh");
  while(true)
  {
    try
    {
      server.run();
    }
    catch (ws::WXerr& err)
    {
      cout << err.func_name << "() at '" << err.location << "':\n" 
        << err.details << "\n";
    }
  }
  return 0;
}
