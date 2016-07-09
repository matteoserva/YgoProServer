#ifndef _NetServerInterface_H_
#define _NetServerInterface_H_
#include "network.h"
#include <list>

namespace ygo
{
class DuelPlayer;

class GameServer;
struct DuelPlayerInfo
{
    DuelPlayerInfo():zombiePlayer(false),isReady(false),secondsWaiting(0)
    {
    };
bool isReady;
float secondsWaiting;
unsigned char last_state_in_timeout;
bool zombiePlayer;
char deck[1024];


std::list< std::pair<int,std::wstring> > pendingMessages;

};

class RoomManager;

class RoomInterface
{
private:
    unsigned short last_sent;
protected:
    RoomManager* roomManager;
    static char net_server_read[0x20000];
    static char net_server_write[0x20000];
    std::map<DuelPlayer*, DuelPlayerInfo> players;
    GameServer* gameServer;
    void playerReadinessChange(DuelPlayer *dp, bool isReady);

    static DuelPlayer* last_chat_dp;


public:
    RoomInterface(RoomManager* roomManager,GameServer*gameServer);
    virtual ~RoomInterface() {};
    virtual void ExtractPlayer(DuelPlayer* dp)=0;
    virtual void InsertPlayer(DuelPlayer* dp)=0;
    virtual void LeaveGame(DuelPlayer* dp)=0;
    virtual void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)=0;
    virtual bool handleChatCommand(DuelPlayer* dp,wchar_t* msg);
    DuelPlayer* getFirstPlayer();

    DuelPlayer* findPlayerByName(std::wstring);

    virtual void SendMessageToPlayer(DuelPlayer*dp, char*msg);
    virtual void SystemChatToPlayer(DuelPlayer*dp, std::wstring,bool isAdmin = false,int color = 0);
	virtual void RemoteChatToPlayer(DuelPlayer*dp, std::wstring,int color = 0);
    void SendPacketToPlayer(DuelPlayer* dp, unsigned char proto);
    template<typename ST>
    void SendPacketToPlayer(DuelPlayer* dp, unsigned char proto, ST& st)
    {
        SendBufferToPlayer(dp,proto,&st,sizeof(ST));


    }
    bool isShouting;
    void BroadcastSystemChat(std::wstring,bool isAdmin = false);
	void BroadcastRemoteChat(std::wstring,int color=0);
    virtual void SendBufferToPlayer(DuelPlayer* dp, unsigned char proto, void* buffer, size_t len);
    void ReSendToPlayer(DuelPlayer* dp);
    int getNumPlayers();
    int detectDeckCompatibleLflist(void* pdata);
	virtual void RoomChat(DuelPlayer* dp, std::wstring messaggio)=0;
};
}
#endif
