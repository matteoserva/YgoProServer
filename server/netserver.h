#ifndef NETSERVER_H
#define NETSERVER_H

#include "config.h"
#include "network.h"
#include "data_manager.h"
#include "deck_manager.h"
#include <unordered_map>
#include "NetServerInterface.h"
#include "field.h"
#include <mutex>
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
    static void crash_detected();
    static bool isCrashed;
private:
    static void DuelTimer(evutil_socket_t fd, short events, void* arg);
    void updateUserTimeout(DuelPlayer* dp);
    void Victory(char winner);
    char last_winner;
    int getNumDuelPlayers();
    void updateServerState();
    void destroyGame();
    event* auto_idle;
    event* user_timeout;
    static void auto_idle_cb(evutil_socket_t fd, short events, void* arg);
    static void user_timeout_cb(evutil_socket_t fd, short events, void* arg);

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
    void SendPacketToPlayer(DuelPlayer* dp, unsigned char proto,STOC_TypeChange);
    void SendBufferToPlayer(DuelPlayer* dp, unsigned char proto, void* buffer, size_t len);

};

}

#endif //NETSERVER_H
