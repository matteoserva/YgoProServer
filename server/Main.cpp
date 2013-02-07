#include "GameServer.h"
#include "Config.h"

using namespace ygo;
const unsigned short PRO_VERSION = 0x12f0;
int enable_log = 0;
int main()
{
    #ifdef _WIN32
        evthread_use_windows_threads();
    #else
        evthread_use_pthreads();
    #endif //_WIN32

    Config::getInstance()->LoadConfig();


    ygo::GameServer* gameServer = new ygo::GameServer();
				if(!gameServer->StartServer(Config::getInstance()->serverport))
                ;
    while(1)
    {
        sleep(1);

    }
    return EXIT_SUCCESS;





}
