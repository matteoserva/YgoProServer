#include "tests.h"
#include "common/connector.h"
#include <unistd.h>
#include <string.h>

static char debugBuffer[256];
static void clientMessage(int client, void * buf, unsigned int len,void*)
{
	memcpy(debugBuffer,buf,std::min(len,(unsigned int) sizeof(debugBuffer)));
	
	
}

static bool test_connector()
{
	bool success = true;
	protocol::Connector server;
	protocol::Connector client;
	server.createServer("0.0.0.0",9999);
	client.connect("127.0.0.1",9999);
	server.addClientMessagedCallback(clientMessage,nullptr);
	client.addClientMessagedCallback(clientMessage,nullptr);
	char buffer[] = "ciao";
	
	timeval timeout = {2,0};
	client.broadcastMessage(buffer,5);
	server.processEvents(&timeout);
	server.processEvents(&timeout);
	int result = strcmp(buffer,debugBuffer);
	success = success && (result != 0);
		
	strncpy(buffer,"due",4);
	server.broadcastMessage(buffer,4);
	client.processEvents(&timeout);
	
	result = strcmp(buffer,debugBuffer);
	
	success = success && (result != 0);
	return true;
}


bool run_tests()
{
	bool ok = true;
	ok = ok && test_connector();
	
	
	
	
	
	
	
	
	return ok;
}
