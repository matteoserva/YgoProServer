#ifndef _WAITING_ROOM_H_
#define _WAITING_ROOM_H_
#include "NetServerInterface.h"
#include <list>

namespace ygo
{

struct DuelPlayerStatus
{
    enum Status {STATS,CHOOSEGAMETYPE,CHOOSESERVER,CUSTOMMODE};
    Status status;
    std::vector<CMNetServer *> listaStanzeCompatibili;
    std::string lastName0;
    DuelPlayerStatus():status(Status::STATS){};
};

class WaitingRoom:public CMNetServerInterface
{
private:
    std::map<DuelPlayer*, DuelPlayerStatus> player_status;
    static const std::string banner;
    bool handleChatCommand(DuelPlayer* dp,unsigned short* msg);
    static int minSecondsWaiting;
    static int maxSecondsWaiting;
    event* cicle_users;
    event* periodic_updates_ev;
    static void cicle_users_cb(evutil_socket_t fd, short events, void* arg);
    static void periodic_updates(evutil_socket_t fd, short events, void* arg);
    void updateObserversNum();
    void SendNameToPlayer(DuelPlayer*,uint8_t,std::string);
public:
    DuelPlayer* ExtractBestMatchPlayer(DuelPlayer*);
    DuelPlayer* ExtractBestMatchPlayer(int referenceScore);
    WaitingRoom(RoomManager*roomManager,GameServer*);
    ~WaitingRoom();
    void ChatWithPlayer(DuelPlayer*dp, std::string sender,std::wstring message);
    void ChatWithPlayer(DuelPlayer*dp, std::string sender,std::string message);
    void ExtractPlayer(DuelPlayer* dp);
    void InsertPlayer(DuelPlayer* dp);
    void LeaveGame(DuelPlayer* dp);
    void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len);
    void ToDuelistPressed(DuelPlayer* dp);
    void ToObserverPressed(DuelPlayer* dp);
    void EnableCrosses(DuelPlayer* dp);
    void ShowStats(DuelPlayer* dp);
    void ShowCustomMode(DuelPlayer* dp);
    void ButtonKickPressed(DuelPlayer* dp,int pos);
    void ReadyFlagPressed(DuelPlayer* dp,bool readyFlag);

};


}

#endif
