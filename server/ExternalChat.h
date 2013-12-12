#ifndef EXTERNALCHAT_CPP
#define EXTERNALCHAT_CPP

#include "GameserversManager.h"
#include <mysql_connection.h>

namespace ygo
{
class ExternalChat
{
    public:

    void broadcastMessage(GameServerChat*);
    std::list<GameServerChat> getPendingMessages();
    static ExternalChat* getInstance();
    void connect();
    void disconnect();



    private:
    int last_id;
    int pid;
    ExternalChat();
    void gen_random(char *s, const int len);

};

}
#endif
