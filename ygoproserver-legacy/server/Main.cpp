#include "Config.h"
#include <string>


#include "GameserversManager.h"

using namespace ygo;
using namespace std;

volatile bool needReboot = false;
volatile bool isFather = true;
int sfd; //server fd




int main(int argc, char**argv)
{
    Config* config = Config::getInstance();
    if(config->parseCommandLine(argc,argv))
        return EXIT_FAILURE;

    if(!config->LoadConfig())
		return EXIT_FAILURE;


    GameserversManager gsm;
    gsm.StartServer(config->serverport);
    return 0;






}
