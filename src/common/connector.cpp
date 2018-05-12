#include "connector.h"
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
namespace protocol
{

Connector::Connector()
{
	serverSocket = -1;
}

Connector::~Connector()
{
	stopThread();
	if(serverSocket >= 0)
		close(serverSocket);
	for(auto &it : clients)
		close(it.first);
}

std::vector<int> Connector::getClients()
{
	std::vector<int>  result;
	for(auto it : clients)
		result.push_back(it.first);
	return result;
}

void Connector::serverCreated(int s)
{
	serverSocket = s;
}

void Connector::processEvents(timeval *timeout)
{
	
	std::vector<int> descriptors;
	bool serverAvailable = false;
	if(serverSocket >= 0)
		serverAvailable = true;
	if(serverAvailable)
		descriptors.push_back(serverSocket);
	for(auto &it : clients)
		descriptors.push_back(it.first);
	if(descriptors.size() == 0)
		return;

	int max_fd = 0;
	fd_set rfds;
	FD_ZERO(&rfds);
	for(int it : descriptors)
	{
		max_fd = std::max(max_fd,it);
		FD_SET(it,&rfds);
	}
	auto retval = select(max_fd + 1, &rfds, NULL, NULL, timeout);
	if(retval == 0)
		return;

	for(int it : descriptors)
	{
		if(! (FD_ISSET(it,&rfds)))
			continue;
		if(serverAvailable && it == serverSocket)
		{
			struct sockaddr_in client_addr;
			socklen_t clilen = sizeof(struct sockaddr_in);
			int new_client = accept(serverSocket, (struct sockaddr *) &client_addr,&clilen);
			if(new_client < 0)
				;//serverDisconnected(serverSocket);
			else
				clientConnected(new_client);

		}
		else
		{
			char tempBuffer[256];
			int bytesread = read(it,tempBuffer,sizeof(tempBuffer));
			if(bytesread <= 0)
				disconnectClient(it);
			else
				processClientData(it,tempBuffer,bytesread);
		}
	}
}

void Connector::addClientMessagedCallback(ClientMessagedCallback c,void* h)
{
	clientMessagedCallbacks.push_back( {c,h});
}

void Connector::addClientDisconnectedCallback(ClientDisconnectedCallback c,void* h)
{
	clientDisconnectedCallbacks.push_back( {c,h});
}

void Connector::addClientConnectedCallback(ClientConnectedCallback c,void* h)
{
	clientConnectedCallbacks.push_back( {c,h});
}

int Connector::connect(int fd)
{
	clientConnected(fd);
	return fd;
}

int Connector::reconnect()
{
	return connect(last_connection_ip,last_connection_port);
}

int Connector::connect(std::string host, unsigned short port)
{
	struct hostent *server = gethostbyname(host.c_str());
	if(!server)
		return -1;

	struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
	      (char *)&serv_addr.sin_addr.s_addr,
	      server->h_length);
	serv_addr.sin_port = htons(port);

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		return -1;
	if (::connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
	{
		close(sockfd);
		return -1;
	}
	last_connection_ip = host;
	last_connection_port = port;
	clientConnected(sockfd);
	return sockfd;
}

unsigned int Connector::broadcastMessage(const void* buf, unsigned int len,int excluded)
{
	unsigned int sent = 0;
	for(auto& it : clients)
	{
		if(excluded != it.first)
		{
			sendMessage(it.first,buf,len);
			sent++;
		}

	}
	return sent;
}

unsigned int Connector::broadcastMessage(const void* buf, unsigned int len)
{
	for(auto& it : clients)
		sendMessage(it.first,buf,len);
	return clients.size();
}

unsigned int Connector::sendMessage(int client, const void* buf,unsigned int len)
{
	PacketLen x = htonl(len);

	int w1 = write(client, &x,sizeof(PacketLen));
	int w2 = write(client,buf,len);

	if(w1 <= 0 || w2 <= 0)
		return 0;
	return w1 + w2;
}


void Connector::processClientData(int client , void* data, unsigned int len)
{
	if(len <= 0)
		return;
	auto it = clients.find(client);
	if(it == clients.end())
		return;
	ClientData & cd = it->second;
	auto chardata = (char*) data;
	std::vector<char> buf;
	if(len > 0)
		cd.buffer.insert(cd.buffer.end(),chardata,chardata+len);

	while(1)
	{
		unsigned int stringLength =cd.buffer.length();
		if( stringLength< sizeof(PacketLen))
			break;
		PacketLen * ds = (PacketLen * ) &cd.buffer[0];
		PacketLen dataSize = ntohl(*ds);


		if(cd.buffer.size() < dataSize + sizeof(PacketLen))
			break;
		void * buffer = &ds[1];
		clientMessaged(client,buffer,dataSize);
		cd.buffer.erase(0,dataSize + sizeof(PacketLen));

	}
}
void Connector::serverDisconnected(int)
{
	close(serverSocket);
	serverSocket = -1;
}

void Connector::clientConnected(int client)
{
	clients[client] = ClientData();
	for(auto it: clientConnectedCallbacks)
		it.first(client,it.second);

}
bool Connector::disconnectClient(int client)
{
	if(clients.find(client) == clients.end())
		return false;
	close(client);
	clients.erase(client);
	clientDisconnected(client);
	return true;
	
}
void Connector::clientMessaged(int client, void* buffer, unsigned int len)
{
	for(auto it: clientMessagedCallbacks)
		it.first(client,buffer,len,it.second);
}

void Connector::clientDisconnected(int client)
{
	
	for(auto it: clientDisconnectedCallbacks)
		it.first(client,it.second);
}

int Connector::createServer(std::string hostIP, unsigned short port)
{

	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	int s = inet_pton(AF_INET, hostIP.c_str(), &sin.sin_addr);
	if(s <= 0)
		return -1;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	int optval = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	optval = 1;
	//setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
	if (::bind(server_fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) == -1)
	{
		printf("errore bind\n");
		return -1;
	}

	int flags = fcntl(server_fd, F_GETFL, 0);
	fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

	listen(server_fd,5);
	serverCreated(server_fd);
	return server_fd;
}

unsigned int Connector::numClients()
{
	return clients.size();
}

void Connector::startThread(timeval * timeout, TimeoutCallback callback,void *handle)
{
	if(looperThread.joinable())
		return;
	looperContinue = true;
	std::thread t(&Connector::threadLoop,this,timeout,callback,handle);
	looperThread = std::move(t);

}

void Connector::stopThread()
{
	if(looperThread.joinable())
	{
		looperContinue = false;
		looperThread.join();
	}
}

void Connector::threadLoop(timeval * t, TimeoutCallback c,void* h)
{

	timeval orig = {5,0};
	if(t)
		orig = *t;

	while(looperContinue)
	{
		timeval timeout = orig;
		processEvents(&timeout);
		if(c)
			c(h);
	}
}

void Connector::clearCallbacks()
{
	clientConnectedCallbacks.clear();
	clientDisconnectedCallbacks.clear();
	clientMessagedCallbacks.clear();

}


}
