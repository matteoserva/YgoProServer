#include "common/Config.h"
#include <string>
#include "tests.h"

#include "GameserversManager.h"
#include "GameServer.h"
#include "common/connector.h"
#include "datautils.h"
using namespace ygo;
using namespace std;

volatile bool needReboot = false;
volatile bool isFather = true;
int sfd; //server fd


void create_manager()
{
	protocol::Connector manager_connector;
	
	manager_connector.createServer(Config::getInstance()->manager_ip,Config::getInstance()->managerport);
	GameserversManager gsm;
	gsm.StartServer(&manager_connector);
	
}


void create_gameserver()
{
	DataUtils utils;
	if(!utils.LoadCards())
		exit (EXIT_FAILURE);
		
	protocol::Connector manager_connector;
	protocol::Connector user_connector;
	int server_fd = user_connector.createServer("0.0.0.0",Config::getInstance()->serverport);
	std::string manager_ip = Config::getInstance()->manager_ip;
	if(manager_ip == "0.0.0.0")
		manager_ip = "127.0.0.1";
	int manager_fd = manager_connector.connect(manager_ip,Config::getInstance()->managerport);
	if(manager_fd < 0)
	{
	   printf("unable to connect to manager\n"); 
	   exit(EXIT_FAILURE);
	}
	
    ygo::GameServer* gameServer = new ygo::GameServer();
  
    if(!gameServer->StartServer(server_fd,&manager_connector))
    {
        printf("cannot start the gameserver\n");
        exit(1);
    }
	
    GameServer::ServerThread(gameServer);
    delete gameServer;

    exit(EXIT_SUCCESS);
}


int main(int argc, char**argv)
{
	bool tests_passed = run_tests();
	if(!tests_passed)
	{
		printf("tests failed\n");
		return EXIT_FAILURE;
	}
	if(argc < 2)
	{
	    printf("missing arguments\n");
	    return EXIT_FAILURE;
	}
	
    Config* config = Config::getInstance();
    if(config->parseCommandLine(argc,argv))
        return EXIT_FAILURE;

    if(!config->LoadConfig())
		return EXIT_FAILURE;

    if(config->start_manager)
    {
        if(fork() > 0)
	  create_manager();
    }
    
    if(config->start_server)
    {
         if(config->start_manager)
	      usleep(500000);
	  create_gameserver();
	
    }
   
    
    return 0;






}
