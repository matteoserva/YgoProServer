#ifndef _CONNECTOR_H
#define _CONNECTOR_H

#include <string>
#include <map>
#include <vector>
#include <thread>
#include <atomic>

namespace protocol
{
class Connector
{
	/* definizioni */
	struct ClientData
	{
		std::string buffer;
		
	};
	typedef unsigned long PacketLen; 
	typedef void (* ClientConnectedCallback)(int,void*);
	typedef void (* ClientDisconnectedCallback)(int,void *);
	typedef void (* ClientMessagedCallback)(int client, void *, unsigned int , void* handle);
	typedef void (* TimeoutCallback)(void *);
	
	/*dati privati */
	
	int serverSocket;
	std::map<int, ClientData> clients;
	std::thread looperThread;
	std::atomic<bool> looperContinue;
	
	std::vector<std::pair<ClientConnectedCallback,void*> > clientConnectedCallbacks;
	std::vector<std::pair<ClientDisconnectedCallback,void*> > clientDisconnectedCallbacks;
	std::vector<std::pair<ClientMessagedCallback,void*> > clientMessagedCallbacks;
	
	std::string last_connection_ip;
	unsigned short last_connection_port;
	
	/* funzioni di callback */
	void serverDisconnected(int);
	void clientConnected(int);
	void clientDisconnected(int);
	void clientMessaged(int client, void* buffer, unsigned int len);
	void serverCreated(int);
	
	/* funzioni thread */
	
	void threadLoop(timeval *, TimeoutCallback,void*);
	
	public:
		Connector();
		~Connector();
		unsigned int numClients();
		int connect(std::string host, unsigned short port);
		int connect(int);
		int reconnect();
		bool disconnectClient(int);
		int createServer(std::string,unsigned short);
		void clearCallbacks();
		void processEvents(timeval *);
		
		unsigned int broadcastMessage(const void*, unsigned int len, int excluded );
		unsigned int broadcastMessage(const void*, unsigned int len);
		unsigned int sendMessage(int client, const void*,unsigned int len);
		std::vector<int> getClients();
		
		void processClientData(int client , void* data, unsigned int len);
		
		
		
		void startThread(timeval * , TimeoutCallback,void*);
		void stopThread();
		
		void addClientMessagedCallback(ClientMessagedCallback,void*);
		void addClientDisconnectedCallback(ClientDisconnectedCallback,void*);
		void addClientConnectedCallback(ClientConnectedCallback,void*);
		
};
}

#endif