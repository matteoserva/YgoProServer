#ifndef EXTERNALCHAT_CPP
#define EXTERNALCHAT_CPP

#include "GameserversManager.h"

namespace ygo
{
class ExternalChat
{
    public:

    void broadcastMessage(GameServerChat*);
    std::list<GameServerChat> getPendingMessages();
    static ExternalChat* getInstance();

    private:
    ExternalChat();
    void gen_random(char *s, const int len);

};

}
#endif
