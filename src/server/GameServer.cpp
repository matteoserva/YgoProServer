#include "common/common_structures.h"
#include "GameServer.h"

#include "RoomManager.h"
#include "common/debug.h"
#include <time.h>
#include <netinet/tcp.h>
#include <thread>
#include <signal.h>

#include "common/connector.h"

using ygo::Config;
using namespace protocol;
namespace ygo
{

volatile bool needsReboot = false;

void sigterm_handler(int signum)
{
	static int timesPressed = 0;
	
	needsReboot=true;
	++timesPressed;

	printf("SIGINT received: now %d\n",timesPressed);
	if(timesPressed ==10)
		kill(getpid(),SIGKILL);
	else if(timesPressed ==6)
		kill(getpid(),SIGQUIT);
	else if(timesPressed == 5)
		kill(getpid(),SIGTERM);
}


GameServer::GameServer()
{
	signal(SIGINT, sigterm_handler);
	net_evbase = 0;
	listener = nullptr;
	last_sent = 0;
	
	MAXPLAYERS = Config::getInstance()->max_users_per_process;
}

bool GameServer::StartServer(int server_fd,protocol::Connector * m)
{
	if(net_evbase)
		return false;
	net_evbase = event_base_new();
	if(!net_evbase)
		return false;
	
	manager_connector = m;
	manager_connector->clearCallbacks();
	manager_connector->addClientMessagedCallback(&GameServer::handleManagerMessage,this);
	
	int manager_fd = *(manager_connector->getClients().begin());
	
	listener =evconnlistener_new(net_evbase,ServerAccept, this, LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE ,-1,server_fd);

	evutil_make_socket_nonblocking(server_fd);
	if(!listener)
	{
		event_base_free(net_evbase);
		net_evbase = 0;
		return false;
	}
	isListening=true;
	evconnlistener_set_error_cb(listener, ServerAcceptError);
	roomManager.setGameServer(const_cast<ygo::GameServer *>(this));


	createManagerBufferEvent(manager_fd);

	sendStats(0,0,this);
	return true;
}

void GameServer::createManagerBufferEvent(int manager_fd )
{
	manager_buf = bufferevent_socket_new(net_evbase, manager_fd, 0);
	bufferevent_setcb(manager_buf, ManagerRead, NULL, ManagerEvent, this);
	bufferevent_enable(manager_buf, EV_READ|EV_WRITE);
	
}

void GameServer::handleManagerMessage(int client, void * buffer, unsigned int , void* handle)
{
	auto that = (GameServer*) handle;
	
	
	auto type = (MessageType*) buffer;
	switch(*type)
	{
		case MessageType::CHAT:
		{
			auto gsc = (protocol::GameServerChat*) buffer;			
			that->roomManager.BroadcastMessage(gsc->messaggio,gsc->chatColor);
			
		}
		break;
		case MessageType::LOGINRESULT:
			that->handleLoginResult((protocol::LoginResult*)buffer);
		break;
		default:
		break;
	}
	
}

void GameServer::ManagerRead(bufferevent *bev, void *ctx)
{

	GameServer* that = (GameServer*)ctx;
	evbuffer* input = bufferevent_get_input(bev);
	size_t len = evbuffer_get_length(input);
	
	int manager_fd = *(that->manager_connector->getClients().begin());
	
	char buffer[256];
	while(len > 0)
	{
		size_t spediti = std::min(len,(size_t)256);
		evbuffer_remove(input, buffer,spediti);
		that->manager_connector->processClientData(manager_fd,buffer,spediti);
		len -= spediti;
	}

}
void GameServer::ManagerEvent(bufferevent* bev, short events, void* ctx)
{
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
	{
		((GameServer *) ctx)->StopListen();
		int fd = bufferevent_getfd(bev);
		bufferevent_disable(bev, EV_READ);
		
		bufferevent_free(bev);
		((GameServer *) ctx)->manager_connector->disconnectClient(fd);
		((GameServer *) ctx)->manager_buf = nullptr;
		printf("manager disconnesso, smetto di accettare utenti\n");
	}
}


int GameServer::getNumPlayers()
{
	return users.size();


}
GameServer::~GameServer()
{
	if(listener)
	{
		StopServer();
	}

	while(users.size() > 0)
	{
		log(WARN,"waiting for reboot, users connected: %lu\n",users.size());
		sleep(5);
	}

	if(net_evbase)
		event_base_loopbreak(net_evbase);
	while(net_evbase)
	{
		log(WARN,"waiting for server thread\n");
		sleep(1);
	}
}
void GameServer::StopServer()
{
	if(!net_evbase)
		return;

	if(listener == nullptr)
		return;
	if(needsReboot)
		return;
	StopListen();

	needsReboot = true;


}

void GameServer::StopListen()
{
	evconnlistener_disable(listener);
	isListening = false;
}

void GameServer::ServerAccept(evconnlistener* listener, evutil_socket_t fd, sockaddr* address, int socklen, void* ctx)
{
	GameServer* that = (GameServer*)ctx;

	int optval=1;
	int optlen = sizeof(optval);
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
	optval = 120;
	setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &optval, optlen);
	optval = 4;
	setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &optval, optlen);
	optval = 30;
	setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &optval, optlen);
	optval = 1;
	setsockopt(fd, SOL_TCP, TCP_NODELAY, &optval, optlen);

	bufferevent* bev = bufferevent_socket_new(that->net_evbase, fd, BEV_OPT_CLOSE_ON_FREE);
	DuelPlayer dp;
	dp.name[0] = 0;
	dp.type = 0xff;
	dp.bev = bev;
	dp.netServer=0;
	dp.loginStatus = ygo::LoginResult::NOTENTERED;
	sockaddr_in* sa = (sockaddr_in*)address;

	inet_ntop(AF_INET, &(sa->sin_addr), dp.ip, INET_ADDRSTRLEN);
	DuelPlayer *dpp = new DuelPlayer(dp);
	that->users[bev] = dpp;
	bufferevent_setcb(bev, ServerEchoRead, NULL, ServerEchoEvent, ctx);
	bufferevent_enable(bev, EV_READ);
	if(that->users.size()>= that->MAXPLAYERS)
	{
		that->StopListen();

	}

	
}
void GameServer::ServerAcceptError(evconnlistener* listener, void* ctx)
{
	GameServer* that = (GameServer*)ctx;
	that->StopListen();
}



void GameServer::callChatCallback(std::wstring message,int color)
{
	GameServerChat gsc;
	gsc.type = CHAT;
	wcscpy(gsc.messaggio,message.c_str());
	gsc.chatColor = color;
	manager_connector->broadcastMessage(&gsc,sizeof(GameServerChat));
	
}


DuelPlayer* GameServer::findPlayer(std::wstring nome)
{
	for(auto it = loggedUsers.begin(); it!=loggedUsers.end(); ++it)
	{
		if(it->first ==nome)
		{
			return (it->second);
		}
	}

	return nullptr;
}
void GameServer::ServerEchoRead(bufferevent *bev, void *ctx)
{
	GameServer* that = (GameServer*)ctx;
	if(that->users.find(bev) == that->users.end())
	{
		printf("LETTO UN BUFFEREVENT INVALIDO\n");
		print_trace();
		bufferevent_disable(bev, EV_READ);
		bufferevent_free(bev);
		return;
	}



	evbuffer* input = bufferevent_get_input(bev);
	size_t len = evbuffer_get_length(input);
	unsigned short packet_len = 0;
	while(that->users.find(bev) != that->users.end())
	{
		if(len < 2)
			return;
		evbuffer_copyout(input, &packet_len, 2);
		if(len < packet_len + 2)
			return;
		evbuffer_remove(input, that->net_server_read, packet_len + 2);
		if(packet_len)
			that->HandleCTOSPacket(that->users[bev], &(that->net_server_read[2]), packet_len);
		len -= packet_len + 2;
	}
}
void GameServer::ServerEchoEvent(bufferevent* bev, short events, void* ctx)
{
	GameServer* that = (GameServer*)ctx;
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
	{
		DuelPlayer* dp = that->users[bev];
		if(dp->netServer)
		{
			dp->netServer->LeaveGame(dp);
			if(that->users.find(bev)!= that->users.end())
			{
				log(BUG,"BUG: tcp terminated but disconnectplayer not called\n");
				that->DisconnectPlayer(dp);
			}

		}
		else that->DisconnectPlayer(dp);
	}
}

void GameServer::RestartListen()
{
	if(!isListening && !needsReboot)
	{
		evconnlistener_enable(listener);
		isListening = true;
	}
}


void GameServer::keepAlive(evutil_socket_t fd, short events, void* arg)
{
	GameServer*that = (GameServer*) arg;

	std::string message = Config::getInstance()->spam_string;
	that->roomManager.BroadcastMessage(std::wstring(message.begin(),message.end()),-1);
}

void GameServer::sendStats(evutil_socket_t fd, short events, void* arg)
{
	GameServer *that = (GameServer*) arg;
	GameServerStats gss;
	gss.rooms = that->roomManager.getNumRooms();
	gss.players = that->getNumPlayers();
	gss.isAlive = that->listener != nullptr && !needsReboot;
	gss.max_users = that->MAXPLAYERS;
	gss.pid = getpid();
	gss.type = STATS;
	
	printf("GameServer: Players: %u\n", that->roomManager.getNumPlayers());
	that->manager_connector->broadcastMessage(&gss,sizeof(GameServerStats));
	if(that->manager_connector->numClients() == 0 && !needsReboot && that->manager_buf == nullptr)
	{
		printf("tento la riconnessione al manager\n");
		int fd = that->manager_connector->reconnect();
		if(fd >= 0)
		{
			
			that->createManagerBufferEvent(fd);
			printf("riconnesso con successo al manager\n");
			that->RestartListen();
		}
		else
		{
			printf("connessione al manager persa, riavvio\n");
			needsReboot = true;
		}
	}

	if(!gss.isAlive && !that->getNumPlayers())
		event_base_loopbreak(that->net_evbase);
	if(that->listener != nullptr &&  needsReboot)
	{
		//lo eliminiamo proprio, così libero il socket per un nuovo processo
		evconnlistener_free(that->listener);
		that->listener = nullptr;

	}
}



int GameServer::ServerThread(void* parama)
{
	GameServer*that = (GameServer*)parama;
	
	event* keepAliveEvent = event_new(that->net_evbase, 0, EV_TIMEOUT | EV_PERSIST, keepAlive, that);
	timeval timeout = {600, 0};
	event_add(keepAliveEvent, &timeout);

	event* statsEvent = event_new(that->net_evbase, 0, EV_TIMEOUT | EV_PERSIST, sendStats, that);
	timeval statstimeout = {5, 0};
	event_add(statsEvent, &statstimeout);

	event_base_dispatch(that->net_evbase);
	if(that->listener != nullptr)
	{
		evconnlistener_free(that->listener);
		that->listener = nullptr;

	}
	event_free(keepAliveEvent);
	event_free(statsEvent);
	
	event_base_free(that->net_evbase);
	that->net_evbase = 0;
	
	return 0;
}

bool GameServer::dispatchPM(std::wstring recipient,std::wstring message)
{
	std::transform(recipient.begin(), recipient.end(), recipient.begin(), ::tolower);
	printf("dispatch\n");
	DuelPlayer* dp = findPlayer(recipient);
	if(dp==nullptr)
		return false;

	dp->netServer->SystemChatToPlayer(dp,message,false,5);
	printf("completed\n");
	return true;
}
bool GameServer::sendPM(std::wstring recipient,std::wstring message)
{
	return dispatchPM(recipient,message);


}


void GameServer::DisconnectPlayer(DuelPlayer* dp)
{
	auto bit = users.find(dp->bev);
	if(bit != users.end())
	{
		if(dp->loginStatus == ygo::LoginResult::NOPASSWORD || dp->loginStatus == ygo::LoginResult::AUTHENTICATED)
		{

			wchar_t nome[25];
			BufferIO::CopyWStr(dp->name,nome,20);
			std::wstring nomes(nome);
			std::transform(nomes.begin(), nomes.end(), nomes.begin(), ::tolower);
			log(VERBOSE,"rimuovo %Ls, %d\n",nomes.c_str(),(int)loggedUsers.size());
			if(loggedUsers.find(nomes)!=loggedUsers.end())
				loggedUsers.erase(nomes);
		}

		dp->netServer=NULL;
		bufferevent_flush(dp->bev, EV_WRITE, BEV_FLUSH);
		bufferevent_disable(dp->bev, EV_READ);
		bufferevent_free(dp->bev);
		users.erase(bit);
		delete dp;


	}

	if(listener != nullptr && users.size()< MAXPLAYERS)
		RestartListen();
	
}

void GameServer::safe_bufferevent_write(DuelPlayer* dp, void* buffer, size_t len)
{
	bufferevent* bev = dp->bev;

	if(users.find(dp->bev) != users.end() && users[bev] == dp)
		bufferevent_write(dp->bev, buffer, len);
	else
	{
		printf("MEGABUG, bufferevent per un utente inesistente\n");
		print_trace();
	}

}

void GameServer::handleLoginResult(protocol::LoginResult *l)
{
	auto bit = users.find((bufferevent*)l->handle);
	if( bit == users.end())
		return;
	DuelPlayer* dp = bit->second;
	if(dp->loginStatus != ygo::LoginResult::WAITINGJOIN2)
		return;
		
	char cc[3];
	cc[2] = 0;
	cc[0] = l->countryCode[0];
	cc[1] = l->countryCode[1];


	dp->countryCode = std::string(cc);
	BufferIO::CopyWStr(l->name, dp->name, 20);
	dp->loginStatus = l->loginStatus;
	dp->color = l->color;
	dp->cachedScore = l->cachedScore;
	dp->cachedRank = l->cachedRank;
	if(roomManager.InsertPlayerInWaitingRoom(dp))
	{
		wchar_t nome[25];
		BufferIO::CopyWStr(dp->name,nome,20);
		std::wstring nomes(nome);
		std::transform(nomes.begin(), nomes.end(), nomes.begin(), ::tolower);
		BufferIO::CopyWStr(nomes.c_str(),dp->namew_low,20);

		if(dp->loginStatus == ygo::LoginResult::NOPASSWORD || dp->loginStatus == ygo::LoginResult::AUTHENTICATED)
		{
			loggedUsers[nomes] = dp;
		}

	}


}
void GameServer::saveDuelResult(std::vector<LoggerPlayerData> nomi, int risultato,std::string replay)
{
	protocol::DuelResult duelResult;
	duelResult.num_players = nomi.size();
	duelResult.type = protocol::DUELRESULT;
	strncpy(duelResult.replayCode,replay.c_str(),sizeof(duelResult.replayCode));
	duelResult.winner = risultato;
	for(unsigned int i = 0; i < duelResult.num_players;i++)
		duelResult.players[i] = nomi[i];
	manager_connector->broadcastMessage(&duelResult,sizeof(protocol::DuelResult));
	
	
}


void GameServer::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{
	char* pdata = data;
	unsigned char pktType = BufferIO::ReadUInt8(pdata);

	if(dp->loginStatus == ygo::LoginResult::NOTENTERED || dp->loginStatus == ygo::LoginResult::WAITINGJOIN)
	{
		if(pktType==CTOS_PLAYER_INFO && dp->loginStatus == ygo::LoginResult::NOTENTERED)
		{
			CTOS_PlayerInfo* pkt = (CTOS_PlayerInfo*)pdata;


			BufferIO::CopyWStr(pkt->name,dp->name,20);

			dp->loginStatus = ygo::LoginResult::WAITINGJOIN;
		}
		else if(pktType == CTOS_JOIN_GAME && dp->name[0] != 0 && dp->loginStatus == ygo::LoginResult::WAITINGJOIN)
		{
			CTOS_JoinGame * ctjg =(CTOS_JoinGame*) pdata;
			char loginstring[45];
			int c1 = BufferIO::CopyWStr(dp->name,loginstring,20);

			int passc = BufferIO::CopyWStr(ctjg->pass,&loginstring[c1+1],20);
			memset(dp->name,0,sizeof(dp->name));

			
			protocol::LoginRequest loginRequest;
			loginRequest.type = protocol::LOGINREQUEST;
			loginRequest.handle = dp->bev;
			memcpy(loginRequest.loginstring,loginstring,sizeof(loginRequest.loginstring));
			strncpy(loginRequest.ip,dp->ip,sizeof(loginRequest.ip));
			manager_connector->broadcastMessage(&loginRequest,sizeof(protocol::LoginRequest));
			
			dp->loginStatus = ygo::LoginResult::WAITINGJOIN2;
			dp->loginRequestTimestamp = time(NULL);

		}
		else if(!strcmp(data,"ping"))
		{

			printf("pong\n");
			bufferevent_write(dp->bev, "pong", 5);
			bufferevent_flush(dp->bev, EV_WRITE, BEV_FLUSH);

		}
		else if(!strncmp(data,"ipchange",8))
		{
			inet_ntop(AF_INET, &data[8], dp->ip, INET_ADDRSTRLEN);
		}
		else
		{
			log(WARN,"player info is not the first packet\n");
		}
		return;
	}

	roomManager.HandleCTOSPacket(dp,data,len);
	return;
}

}
