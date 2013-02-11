#include "GameServer.h"
#include "Config.h"
#include <string>

using namespace ygo;
using namespace std;
const unsigned short PRO_VERSION = 0x12f0;
int enable_log = 0;
int main(int argc, char**argv)
{
#ifdef _WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif //_WIN32

    Config* config = Config::getInstance();
    if(config->parseCommandLine(argc,argv))
        return EXIT_SUCCESS;

    config->LoadConfig();


    ygo::GameServer* gameServer = new ygo::GameServer();
    if(!gameServer->StartServer(config->serverport))
    {
        printf("cannot bind the server to port %d\n",config->serverport);
    }
    else
    {
        while(1)
        {
            //sleep(1);
            std::string command;
            cin >>command;
            if (command == "reboot")
                {
                    cout<<"rebooting";
                    gameServer->StopServer();
                    exit(0);
                }

        }
    }
    return EXIT_SUCCESS;
}
