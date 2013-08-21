#include "WaitingRoom.h"
#include "GameServer.h"
#include "debug.h"
#include "Users.h"
#include "Config.h"
#include "Statistics.h"

extern const unsigned int BUILD_NUMBER;
namespace ygo
{

int WaitingRoom::minSecondsWaiting;
int WaitingRoom::maxSecondsWaiting;
const std::string WaitingRoom::banner = "[Checkmate Server!]";

WaitingRoom::WaitingRoom(RoomManager*roomManager,GameServer*gameServer):
    CMNetServerInterface(roomManager,gameServer),cicle_users(0)
{
    WaitingRoom::minSecondsWaiting=Config::getInstance()->waitingroom_min_waiting;
    WaitingRoom::maxSecondsWaiting=Config::getInstance()->waitingroom_max_waiting;
    event_base* net_evbase=roomManager->net_evbase;
    cicle_users = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, cicle_users_cb, const_cast<WaitingRoom*>(this));
    periodic_updates_ev = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, periodic_updates, const_cast<WaitingRoom*>(this));
    timeval timeout = {1, 0};
    event_add(cicle_users, &timeout);
    timeval timeout2 = {0, 100000};
    event_add(periodic_updates_ev, &timeout2);
}

WaitingRoom::~WaitingRoom()
{
    event_free(cicle_users);
    event_free(periodic_updates_ev);
}

void WaitingRoom::periodic_updates(evutil_socket_t fd, short events, void* arg)
{
    WaitingRoom*that = (WaitingRoom*)arg;
    for(auto it=that->players.begin(); it!=that->players.end(); ++it)
    {
        it->second.secondsWaiting+= 0.1;

        if(it->second.secondsWaiting< 1.5)
            continue;
        int cycle = 10 * (it->second.secondsWaiting-1.5);

        int start = cycle % (banner.length() + 10);
        if(start >= banner.length())
            start = 0;
        std::string newstr = banner.substr(start) + banner.substr(0,start);
        STOC_HS_PlayerEnter scpe;
        BufferIO::CopyWStr(newstr.c_str(), scpe.name, 20);

        scpe.pos = 1;
        //that->SendPacketToPlayer(it->first, STOC_HS_PLAYER_ENTER, scpe);


    }




}

void WaitingRoom::cicle_users_cb(evutil_socket_t fd, short events, void* arg)
{
    WaitingRoom*that = (WaitingRoom*)arg;

    if(!that->players.size())
        return;

    std::list<DuelPlayer*> players_bored;
    int numPlayersReady=0;
    for(auto it=that->players.begin(); it!=that->players.end(); ++it)
    {
        char nome[30];
        BufferIO::CopyWStr(it->first->name,nome,30);
        if(!that->players[it->first].isReady)
            continue;
        //log(INFO,"%s aspetta da %d secondi\n",nome,it->second.secondsWaiting);
        if(it->second.secondsWaiting>= maxSecondsWaiting)
            players_bored.push_back(it->first);
        if(it->second.secondsWaiting>= minSecondsWaiting)
            numPlayersReady++;
    }

    if(players_bored.size() )
        for (auto it = players_bored.cbegin(); it != players_bored.cend(); ++it)
        {
            if(that->players.find(*it) != that->players.end())
            {

                that->ExtractPlayer(*it);
                that->roomManager->InsertPlayer(*it);
            }
        }
}

void WaitingRoom::ExtractPlayer(DuelPlayer* dp)
{
    player_erase_cb(dp);
    players.erase(dp);
    player_status.erase(dp);
    dp->netServer=0;
    updateObserversNum();
}

void WaitingRoom::updateObserversNum()
{
    int num = players.size() -1;
    STOC_HS_WatchChange scwc;
    scwc.watch_count = num;
    for(auto it=players.begin(); it!=players.end(); ++it)
    {
        SendPacketToPlayer(it->first, STOC_HS_WATCH_CHANGE, scwc);
    }
}

void WaitingRoom::ChatWithPlayer(DuelPlayer*dp, std::string sender,std::wstring message)
{
    std::wstring oldName0 = player_status[dp].lastName0;

    SendNameToPlayer(dp,0,sender);
    STOC_Chat scc;
    scc.player = NETPLAYER_TYPE_PLAYER1;
    int msglen = BufferIO::CopyWStr(message.c_str(), scc.msg, 256);
    SendBufferToPlayer(dp, STOC_CHAT, &scc, 4 + msglen * 2);
    SendNameToPlayer(dp,0,oldName0);
}

void WaitingRoom::SendNameToPlayer(DuelPlayer* dp,uint8_t pos,std::wstring message)
{
    if(pos == 0)
        player_status[dp].lastName0 = message;
    STOC_HS_PlayerEnter scpe;
    BufferIO::CopyWStr(message.c_str(), scpe.name, 20);
    scpe.pos = pos;
    SendPacketToPlayer(dp, STOC_HS_PLAYER_ENTER, scpe);

}

void WaitingRoom::ChatWithPlayer(DuelPlayer*dp, std::string sender,std::string message)
{
    ChatWithPlayer(dp,sender,std::wstring(message.cbegin(),message.cend()));
}

void WaitingRoom::InsertPlayer(DuelPlayer* dp)
{
    dp->netServer=this;
    players[dp] = DuelPlayerInfo();
    player_status[dp] = DuelPlayerStatus();
    HostInfo info;
    info.rule=2;
    info.mode=MODE_TAG;
    info.draw_count=1;
    info.no_check_deck=false;
    info.start_hand=5;
    info.lflist=1;
    info.time_limit=0;
    info.start_lp=0;
    info.enable_priority=false;
    info.no_shuffle_deck=false;

    unsigned int hash = 1;
    for(auto lfit = deckManager._lfList.begin(); lfit != deckManager._lfList.end(); ++lfit)
    {
        if(info.lflist == lfit->hash)
        {
            hash = info.lflist;
            break;
        }
    }
    if(hash == 1)
        info.lflist = deckManager._lfList[0].hash;
    dp->type = NETPLAYER_TYPE_PLAYER1;

    STOC_JoinGame scjg;
    scjg.info=info;
    SendPacketToPlayer(dp, STOC_JOIN_GAME, scjg);

    STOC_TypeChange sctc;
    sctc.type = dp->type;
    SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);


    /*STOC_HS_PlayerEnter scpe;
    BufferIO::CopyWStr(dp->name, scpe.name, 20);
    scpe.pos = 0;
    SendPacketToPlayer(dp, STOC_HS_PLAYER_ENTER, scpe);*/






    updateObserversNum();

    playerReadinessChange(dp,true);



    ShowStats(dp);

    char buffer[200];



    sprintf(buffer,"PID: %d, BUILD: %d, PLAYERS: %d",(int)getpid(),(int)BUILD_NUMBER,Statistics::getInstance()->getNumPlayers());
    ChatWithPlayer(dp, "CheckMate",buffer);

     ChatWithPlayer(dp, "CheckMate","Welcome to the CheckMate server!");
    //ChatWithPlayer(dp, "CheckMate","Type !tag to enter a tag duel, !single for a single duel or !match");

    if(dp->loginStatus != Users::LoginResult::AUTHENTICATED && dp->loginStatus != Users::LoginResult::UNRANKED)
    {
        ChatWithPlayer(dp, "CheckMate","to register and login, go back and change the username to yourusername$yourpassword");
    }

    if(dp->loginStatus != Users::LoginResult::AUTHENTICATED && dp->loginStatus != Users::LoginResult::NOPASSWORD)
    {
        SystemChatToPlayer(dp,L"You are playing unranked. Your messages won't appear in the global chat",true);
    }

    //ChatWithPlayer(dp, "CheckMate",L"我正在工作为了在ygopro上你们也可以用中文");
    ChatWithPlayer(dp, "CheckMate","click the 'duel' button to choose the game type or 'spectate' for the available rooms list");

    SystemChatToPlayer(dp,L"profile, score and statistics at http://ygopro.it/",true);
    if(dp->countryCode == "IT")
    {
        SystemChatToPlayer(dp, L"[BIP][BIP] Duellante italiano rilevato [BIP] Benvenuto nel mio server! buon divertimento",true);
    }


}
void WaitingRoom::LeaveGame(DuelPlayer* dp)
{
    ExtractPlayer(dp);
    gameServer->DisconnectPlayer(dp);
}

DuelPlayer* WaitingRoom::ExtractBestMatchPlayer(int referenceScore)
{
    if(!players.size())
        return nullptr;

    int qdifference = 0;
    DuelPlayer *chosenOne = nullptr;

    for(auto it=players.cbegin(); it!=players.cend(); ++it)
    {
        if(it->second.secondsWaiting >= minSecondsWaiting)
        {
            DuelPlayer* dp = it->first;
            if(!players[dp].isReady)
                continue;

            char opname[20];
            BufferIO::CopyWStr(dp->name,opname,20);
            int opscore = dp->cachedRankScore;

            int candidate_qdifference = abs(referenceScore-opscore);
            if(chosenOne == nullptr || candidate_qdifference < qdifference)
            {
                qdifference = candidate_qdifference;
                chosenOne = dp;
                continue;
            }

        }
    }

    int maxqdifference = RoomManager::maxScoreDifference(referenceScore);
    if( qdifference > maxqdifference)
        chosenOne = nullptr;
    if(chosenOne != nullptr)
    {
        ExtractPlayer(chosenOne);
        log(INFO,"qdifference = %d\n",qdifference);
    }
    return chosenOne;
}

DuelPlayer* WaitingRoom::ExtractBestMatchPlayer(DuelPlayer* referencePlayer)
{
    int score = referencePlayer->cachedRankScore;
    return ExtractBestMatchPlayer(score);
}

bool WaitingRoom::ChatMessageReceived(DuelPlayer* dp,unsigned short* msg)
{
    wchar_t messaggio[256];
    int msglen = BufferIO::CopyWStr(msg, messaggio, 256);
    if(msglen == 0)
        return false;
    if(msg[0] == '!' && handleChatCommand(dp,messaggio))
        return true;

    char sender[20];
    BufferIO::CopyWStr(dp->name,sender,20);
    for(auto it=players.begin(); it!=players.end(); ++it)
    {
        if(it->first== dp)
            continue;
        ChatWithPlayer(it->first, std::string(sender) + "<"+dp->countryCode+">",messaggio);
    }

    shout(msg,dp);

    /* wchar_t name[20];
     wchar_t messaggiow[200];
     BufferIO::CopyWStr(msg, messaggiow, 200);
     BufferIO::CopyWStr(dp->name, name, 20);
     if(dp->loginStatus == Users::LoginResult::AUTHENTICATED || dp->loginStatus == Users::LoginResult::NOPASSWORD)
         shout(std::wstring(messaggiow),false,std::wstring(name));*/
    return true;

}


void WaitingRoom::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{
    char*pdata=data;
    unsigned char pktType = BufferIO::ReadUInt8(pdata);

    switch(pktType)
    {
    case CTOS_UPDATE_DECK:
    {
            detectDeckCompatibleLflist(pdata);
            break;
    }
    case CTOS_PLAYER_INFO:
    {

        //log(INFO,"WaitingRoom:Player joined %s \n",name);
        break;
    }
    case CTOS_CHAT:
    {
        ChatMessageReceived(dp,(unsigned short*) pdata);
        break;
    }
    case CTOS_LEAVE_GAME:
    {
        LeaveGame(dp);
        break;
    }
    case CTOS_JOIN_GAME:
    {
        InsertPlayer(dp);
        break;
    }
    case CTOS_HS_READY:
    case CTOS_HS_NOTREADY:
    {
        ReadyFlagPressed(dp,CTOS_HS_NOTREADY - pktType);
        break;
    }
    case CTOS_HS_TODUELIST:
    {
        ToDuelistPressed(dp);
        break;
    }
    case CTOS_HS_TOOBSERVER:
    {
        ToObserverPressed(dp);
        break;
    }
    case CTOS_HS_KICK:
    {
        CTOS_Kick* pkt = (CTOS_Kick*)pdata;
        if(pkt->pos >= 0 && pkt->pos <=3)
            ButtonKickPressed(dp,pkt->pos);
        break;
    }

    case CTOS_HS_START:
    {
        printf("start premuto\n");
        ShowStats(dp);
        break;
    }

    }
}

}
