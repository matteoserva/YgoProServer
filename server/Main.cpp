#include "GameServer.h"
#include "game.h"
#include "data_manager.h"
const unsigned short PRO_VERSION = 0x12f0;
int enable_log = 0;
int main()
{
    #ifdef _WIN32
        evthread_use_windows_threads();
    #else
        evthread_use_pthreads();
    #endif //_WIN32

    ygo::deckManager.LoadLFList();
if(!ygo::dataManager.LoadDB("cards.cdb"))
		return false;
	if(!ygo::dataManager.LoadStrings("strings.conf"))
        return false;


    ygo::GameServer* gameServer = new ygo::GameServer();
				if(!gameServer->StartServer(9999))
                ;
    while(1)
    {
        sleep(1);

    }
    return EXIT_SUCCESS;





}
