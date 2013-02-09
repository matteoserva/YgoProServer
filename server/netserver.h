#ifndef NETSERVER_H
#define NETSERVER_H

#include "config.h"
#include "network.h"
#include "data_manager.h"
#include "deck_manager.h"
#include <set>
#include <unordered_map>
#include <map>
#include "NetServerInterface.h"
#include "field.h"
namespace ygo
{
class GameServer;
class RoomManager;






class CMNetServer:public CMNetServerInterface
{
    public:
    unsigned char mode;
    enum State {WAITING,FULL,PLAYING,ZOMBIE,DEAD};
    State state;
private:
    void Victory(unsigned char winner);
    unsigned char last_winner;
    int getNumDuelPlayers();
    void updateServerState();
    void destroyGame();
    event* auto_idle;
    static void auto_idle_cb(evutil_socket_t fd, short events, void* arg);

    void playerReadinessChange(DuelPlayer *dp, bool isReady);
    DuelMode* duel_mode;


    int numPlayers;

    void playerConnected(DuelPlayer* dp);
    void playerDisconnected(DuelPlayer* dp);
    int getMaxDuelPlayers();
    void clientStarted();
    void setState(State state);
    void toObserver(DuelPlayer*dp);

public:

    CMNetServer(RoomManager*roomManager,GameServer*,unsigned char mode);
    void LeaveGame(DuelPlayer* dp);
    bool StartServer(unsigned short port);
    bool StartBroadcast();
    void StopServer();
    void StopBroadcast();
    void StopListen();
    void createGame();
    void DisconnectPlayer(DuelPlayer* dp);
    void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len);

    void InsertPlayer(DuelPlayer* dp);
    void ExtractPlayer(DuelPlayer* dp);

    using CMNetServerInterface::SendPacketToPlayer;
    void SendPacketToPlayer(DuelPlayer* dp, unsigned char proto);
    void SendBufferToPlayer(DuelPlayer* dp, unsigned char proto, void* buffer, size_t len);

};

}

#endif //NETSERVER_H
