#include "GameserversManager.h"
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "common/Config.h"
//#include "GameServer.h"
#include "common/debug.h"
#include "Statistics.h"
#include <time.h>
#include "common/connector.h"
#include "ExternalChat.h"
#include "MySqlWrapper.h"


#include "common/server_protocol.h"
using namespace protocol;
using namespace std;
namespace ygo
{






bool GameserversManager::serversAlmostFull()
{
	int threeshold = Config::getInstance()->max_users_per_process/10;
	threeshold = max(threeshold,1);
	threeshold = min(threeshold,5);
	log(VERBOSE,"high threeshold = %d\n",threeshold);
	bool isAlmostFull = getNumPlayersInAliveChildren()+threeshold > getNumAliveChildren()*Config::getInstance()->max_users_per_process;
	return isAlmostFull;
}
bool GameserversManager::serversAlmostEmpty()
{
	int threeshold = 0.5*Config::getInstance()->max_users_per_process;
	threeshold = min(threeshold,30);
	threeshold = max(threeshold,1);
	log(VERBOSE,"low threeshold = %d\n",threeshold);
	bool isAlmostEmpty = getNumPlayersInAliveChildren() <= (getNumAliveChildren()-1) * Config::getInstance()->max_users_per_process -threeshold;
	return isAlmostEmpty;
}

int GameserversManager::getNumAliveChildren()
{
	int alive=0;
	for(auto it = children.cbegin(); it != children.cend(); ++it)
		if(it->second.isAlive)
			alive++;
	return alive;
}
int GameserversManager::getNumPlayersInAliveChildren()
{
	int numTotal=0;
	for(auto it = children.cbegin(); it!=children.cend(); ++it)
		if(it->second.isAlive)
			numTotal += it->second.players;
	return numTotal;
}

void GameserversManager::ShowStats()
{
	static time_t last_showstats = 0;

	if(time(NULL) - last_showstats > 5)
	{
		last_showstats = time(NULL);
		for(auto it = children.cbegin(); it != children.cend(); ++it)
		{
			ChildInfo gss = it->second;
			printf("pid: %5d, rooms: %3d, users %3d",gss.pid,gss.rooms,gss.players);
			if(!it->second.isAlive)
				printf("  *dying*");
			printf("\n");

		}
		printf("children: %2d, alive %2d, rooms: %3d, players alive:%3d, players: %3d\n",
		       (int)children.size(),getNumAliveChildren(),getNumRooms(),getNumPlayersInAliveChildren(),getNumPlayers());
		for(auto it = children.cbegin(); it != children.cend(); ++it)
		{
			ChildInfo gss = it->second;
			if(gss.pid == 0)
			  continue;
			protocol::GameServerStats serverStats;
			serverStats.type = protocol::STATS;
			serverStats.pid = gss.pid;
			serverStats.rooms = gss.rooms;
			serverStats.players = gss.players;
			serverStats.max_users = Config::getInstance()->max_users_per_process;
			serverStats.isAlive = gss.isAlive;
			databaseFacade.SendStatisticsRow(serverStats);

		}

	}
}

GameserversManager::GameserversManager():maxchildren(4)
{
	//signal(SIGINT, SIG_IGN);
	prepara_segnali();
}


int GameserversManager::getNumRooms()
{
	int rooms = 0;
	for(auto it = children.cbegin(); it != children.cend(); ++it)
		rooms += it->second.rooms;
	return rooms;
}
int GameserversManager::getNumPlayers()
{
	int pl = 0;
	for(auto it = children.cbegin(); it != children.cend(); ++it)
		pl += it->second.players;
	return pl;
}

void GameserversManager::clientConnected(int child_fd,void* handle)
{
	printf("client connected %d\n",child_fd);
	auto that = (GameserversManager*)handle;
	that->children[child_fd].players = 0;
	that->children[child_fd].rooms= 0;
	that->children[child_fd].isAlive= true;
	that->children[child_fd].last_update = time(NULL);
	that->children[child_fd].pid = 0;
}
void GameserversManager::clientDisconnected(int child_fd,void *handle)
{
	printf("client disconnected %d\n",child_fd);
	auto that = (GameserversManager*)handle;
	that->children.erase(child_fd);
}
void GameserversManager::clientMessaged(int child_fd, void * buffer, unsigned int len, void* handle)
{
	auto that = (GameserversManager*)handle;

	MessageType type = *((MessageType *)buffer);
	if(type == STATS)
	{
		GameServerStats* gss = (GameServerStats*)buffer;

		log(VERBOSE,"il figlio ha spedito un messaggio\n");
		that->children[child_fd].players = gss->players;
		that->children[child_fd].rooms= gss->rooms;
		that->children[child_fd].isAlive= gss->isAlive;
		that->children[child_fd].last_update = time(NULL);
		that->children[child_fd].pid = gss->pid;
		Statistics::getInstance()->setNumPlayers(that->getNumPlayers());
		Statistics::getInstance()->setNumRooms(that->getNumRooms());
	}
	else if(type == CHAT)
	{
		GameServerChat* gss = (GameServerChat*)buffer;
		that->clients_connector->broadcastMessage(buffer,sizeof(GameServerChat),child_fd);
		that->databaseFacade.broadcastMessage(gss);
		


	}
	else if(type == LOGINREQUEST)
	{
		auto tmp = (protocol::LoginRequest *)buffer;
		protocol::LoginRequest request = *tmp;
		printf("login request: user %s\n",request.loginstring);
		auto response = that->databaseFacade.processLoginRequest(request);
		that->clients_connector->sendMessage(child_fd,&response,sizeof(response));
		


	}
	else if(type == DUELRESULT)
	{
		auto tmp = (protocol::DuelResult * ) buffer;
		protocol::DuelResult result = *tmp;
		that->databaseFacade.processDuelResult(result);


	}
}


void GameserversManager::timeoutCallback(void * handle)
{
	auto that = (GameserversManager*) handle;
	std::list<protocol::GameServerChat> lista =  that->databaseFacade.getPendingMessages();

	for(auto lit = lista.cbegin(); lit!= lista.cend(); ++lit)
		that->clients_connector->broadcastMessage(&(*lit),sizeof(protocol::GameServerChat));

	that->ShowStats();
	
}

void GameserversManager::StartServer(protocol::Connector * connector)
{
	clients_connector = connector;
	connector->clearCallbacks();
	connector->addClientConnectedCallback(&GameserversManager::clientConnected,this);
	connector->addClientDisconnectedCallback(&GameserversManager::clientDisconnected,this);
	connector->addClientMessagedCallback(&GameserversManager::clientMessaged,this);
	while(1)
	{
		struct timeval t = {2,0};
		connector->processEvents(&t);
		timeoutCallback(this);

	}
	

}
}
