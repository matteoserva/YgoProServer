#ifndef EXTERNALCHAT_CPP
#define EXTERNALCHAT_CPP

//#include "GameserversManager.h"
#include "common/server_protocol.h"
#include <list>
#include <mysql_connection.h>


namespace ygo
{
	
	class MySqlWrapper;
class ExternalChat
{
	MySqlWrapper *mysqlWrapper;
    public:
	ExternalChat(MySqlWrapper *);
	
	
    void broadcastMessage(protocol::GameServerChat*);
    std::list<protocol::GameServerChat> getPendingMessages();

    private:
    int last_id;
    int pid;
    
    void gen_random(char *s, const int len);

};

}
#endif
