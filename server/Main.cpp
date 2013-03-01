#include "GameServer.h"
#include "Config.h"
#include <string>

#include <sys/socket.h>

using namespace ygo;
using namespace std;


int main(int argc, char**argv)
{
    Config* config = Config::getInstance();
    if(config->parseCommandLine(argc,argv))
        return EXIT_SUCCESS;

    config->LoadConfig();

    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(config->serverport);

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (::bind(sfd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) == -1)
    {
        printf("errore bind\n");
        return false;
    }
    evutil_make_socket_nonblocking(sfd);

    ygo::GameServer* gameServer = new ygo::GameServer(sfd);
    //ygo::GameServer* gameServer2 = new ygo::GameServer();
    //gameServer2->StartServer(sfd);
    if(!gameServer->StartServer())
    {
        printf("cannot bind the server to port %d\n",config->serverport);
        exit(1);
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
