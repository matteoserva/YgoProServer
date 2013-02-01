#include "config.h"
#include "game.h"
#include "data_manager.h"
#include <event2/thread.h>
#include "GameServer.h"


int enable_log = 0;
bool exit_on_return = false;

int main(int argc, char* argv[]) {
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);
	evthread_use_windows_threads();
#else
	evthread_use_pthreads();
#endif //_WIN32


	ygo::Game _game;
	ygo::mainGame = &_game;


	if(!ygo::mainGame->Initialize())
		return 0;

	                ygo::GameServer* gameServer = new ygo::GameServer();
				if(!gameServer->StartServer(9999))


                    ;

                    while(1)
                        sleep(1);


	//ygo::mainGame->MainLoop();
#ifdef _WIN32
	WSACleanup();
#else

#endif //_WIN32
	return EXIT_SUCCESS;
}
