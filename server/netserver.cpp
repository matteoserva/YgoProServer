#include "netserver.h"
#include "single_duel.h"
#include "tag_duel.h"
#include "GameServer.h"
#include "RoomManager.h"
#include "debug.h"
#include "Users.h"
#include <algorithm>
namespace ygo
{
CMNetServer::CMNetServer(RoomManager*roomManager,GameServer*gameServer,unsigned char mode)
    :CMNetServerInterface(roomManager,gameServer),mode(mode),duel_mode(0),last_winner(-1),user_timeout(nullptr)
{
    createGame();
}


void CMNetServer::flushPendingMessages()
{
    if(!chatReady)
        return;
    for(auto it = players.begin(); it!=players.end(); ++it)
    {

        for(auto i=it->second.pendingMessages.begin(); i != it->second.pendingMessages.end(); i++)
        {
            SystemChatToPlayer(it->first,i->second,i->first);

        }
        it->second.pendingMessages.clear();
    }

}
void CMNetServer::SendPacketToPlayer(DuelPlayer* dp, unsigned char proto)
{
    CMNetServerInterface::SendPacketToPlayer(dp,proto);
    if(proto == STOC_DUEL_START)
    {
        clientStarted();
    }
    else if(proto == STOC_CHANGE_SIDE)
    {
        chatReady=false;
        ReadyMessagesSent=0;
    }
}

void CMNetServer::SendPacketToPlayer(DuelPlayer* dp, unsigned char proto,STOC_TypeChange sctc)
{
    sctc.type &= ~ 0x10;
    CMNetServerInterface::SendPacketToPlayer(dp,proto,sctc);
}

/*
void CMNetServer::EverybodyIsPlaying()
{


    if(mode != MODE_TAG)
        for(auto it = players.cbegin(); it!=players.cend(); ++it)
        {
            if(it->first->type == NETPLAYER_TYPE_OBSERVER)
                continue;
            char buffer[256],name[20];
            BufferIO::CopyWStr(it->first->name, name,20);
            int score = Users::getInstance()->getScore(std::string(name));
            sprintf(buffer, "%s has %d points",name,score);
            /* people keep complaining about the match maker
            //SendMessageToPlayer(dp,buffer);
        }

}
*/
void CMNetServer::SendBufferToPlayer(DuelPlayer* dp, unsigned char proto, void* buffer, size_t len)
{
    CMNetServerInterface::SendBufferToPlayer(dp,proto,buffer,len);
    if(proto == STOC_GAME_MSG)
    {
        unsigned char* wbuf = (unsigned char*)buffer;
        if(wbuf[0] == MSG_WIN)
        {
            log(VERBOSE,"---------vittoria per il giocatore\n");
            last_winner =wbuf[1];
        }

        if(wbuf[0] == MSG_START)
        {
            if(++ReadyMessagesSent == players.size())
            {
                chatReady=true;
                flushPendingMessages();
            }

        }
    }
    else if(proto==STOC_REPLAY)
    {
        if(chatReady)
        {
            chatReady=false;
        }

        ReadyMessagesSent=0;
    }
}

void CMNetServer::auto_idle_cb(evutil_socket_t fd, short events, void* arg)
{
    CMNetServer* that = (CMNetServer*)arg;
    log(INFO,"auto idle_cb\n");
    if(that->state != FULL)
        return;
    for(auto it = that->players.cbegin(); it!=that->players.cend(); ++it)
    {
        if(it->first->type != NETPLAYER_TYPE_OBSERVER && !(it->second.isReady))
        {
            that->SendMessageToPlayer(it->first,"You are moved to spectators for not being ready");
            that->toObserver(it->first);

        }
    }
}

void CMNetServer::ShowPlayerScores()
{
    wchar_t message[256];
    message[0]=0;
    wchar_t buffer[64];
    wcscat(message,L"SCORES:");

    for(auto it = players.cbegin(); it!=players.cend(); ++it)
    {
        if(it->first->type == NETPLAYER_TYPE_OBSERVER)
            continue;
        if(it->first->cachedRankScore == 0)
        {
            BroadcastSystemChat(L"this duel is unranked",true);
            return;
        }


        BufferIO::CopyWStr(it->first->name,buffer,20);
        wcscat(message,L"  [");
        wcscat(message,buffer);
        wcscat(message,L"]<");

        std::wstring tmp(it->first->countryCode.begin(),it->first->countryCode.end());
        wcscat(message,tmp.c_str());
        wcscat(message,L">: ");

        swprintf(buffer,64,L"%d(%+d)",it->first->cachedRankScore, it->first->cachedGameScore-it->first->cachedRankScore);
        wcscat(message,buffer);
    }
    BroadcastSystemChat(message,true);
}

void CMNetServer::ShowPlayerOdds()
{
    char message[256];



    if(mode == MODE_TAG)
        return;
    DuelPlayer* _players[2];
    for(int i = 0; i<2; i++)
        _players[i]=0;

    for(auto it = players.cbegin(); it!= players.cend(); ++it)
    {
        if(it->first->type <= NETPLAYER_TYPE_PLAYER2 && it->first->type >= NETPLAYER_TYPE_PLAYER1)
            _players[it->first->type] = it->first;
    }
    if(!_players[0] || !_players[0]->cachedRankScore)
        return;
    if(!_players[1] || !_players[1]->cachedRankScore)
        return;

    char name0[20];
    char name1[20];
    BufferIO::CopyWStr(_players[0]->name,name0,20);
    BufferIO::CopyWStr(_players[1]->name,name1,20);
    message[0] = 0;


    if(_players[0]->cachedRankScore > _players[1]->cachedRankScore)
    {
        float odds = 100.0*Users::getInstance()->win_exp(_players[0]->cachedRankScore - _players[1]->cachedRankScore);
        sprintf(message,"There is a %d%% chance that %s is going to win this duel",(int) odds,name0);


    }
    else
    {
        float odds = 100.0*Users::getInstance()->win_exp(_players[1]->cachedRankScore - _players[0]->cachedRankScore);
        sprintf(message,"There is a %d%% chance that %s is going to win this duel",(int) odds,name1);
    }
    std::string temp(message);
    BroadcastSystemChat(std::wstring(temp.begin(),temp.end()),true);





}

void CMNetServer::clientStarted()
{
    if(state==FULL)
    {
        setState(PLAYING);
        timeval timeout = {10, 0};
        event_add(user_timeout, &timeout);
        chatReady=false;
        ShowPlayerOdds();
        ShowPlayerScores();
        //duel_mode->host_info.time_limit=60;
    }

}


void CMNetServer::setState(State state)
{
    if(this->state != state)
        roomManager->notifyStateChange(this,this->state,state);
    this->state=state;
}

void CMNetServer::destroyGame()
{
    if(duel_mode)
    {
        if(state != DEAD)
        {
            auto tempPlayers = players;
            for(auto it =tempPlayers.cbegin(); it!=tempPlayers.cend(); ++it)
            {
                LeaveGame(it->first);
            }
        }
        duel_mode->EndDuel();
        event_del(duel_mode->etimer);
        event_free(duel_mode->etimer);
        delete duel_mode;
        duel_mode=0;
    }
    if(user_timeout)
    {
        event_free(user_timeout);
        user_timeout=0;
    }
    if(auto_idle)
    {
        event_free(auto_idle);
        auto_idle=0;
    }

}

int CMNetServer::getMaxDuelPlayers()
{
    int maxplayers = 2;
    if(mode == MODE_TAG)
        maxplayers=4;
    return maxplayers;
}



void CMNetServer::playerConnected(DuelPlayer *dp)
{
    if(players.find(dp)==players.end())
        players[dp] = DuelPlayerInfo();
    numPlayers=players.size();

    log(VERBOSE,"netserver: giocatori connessi:%d\n",numPlayers);
    updateServerState();
}

int CMNetServer::getNumDuelPlayers()
{
    int n=0;

    for(auto it = players.cbegin(); it!=players.cend(); ++it)
    {
        if(it->first->type != NETPLAYER_TYPE_OBSERVER)
            n++;
    }
    return n;
}

void CMNetServer::updateServerState()
{
    if(auto_idle)
        event_del(auto_idle);

    if(getNumDuelPlayers() < getMaxDuelPlayers() &&state==FULL)
    {
        setState(WAITING);
        log(VERBOSE,"server not full\n");
    }
    if(numPlayers==0 &&state != DEAD)//&& (state==ZOMBIE || state == WAITING))
    {
        log(INFO,"server vuoto. addio, morto\n");

        destroyGame();
        setState(DEAD);
    }
    if(getNumDuelPlayers()>=getMaxDuelPlayers() && state==WAITING)//
    {
        log(VERBOSE,"server full\n");
        setState(FULL);
        timeval timeout = {10, 0};
        event_add(auto_idle, &timeout);
    }

}

void CMNetServer::playerDisconnected(DuelPlayer* dp )
{
    if(players.find(dp)!=players.end())
        players.erase(dp);
    numPlayers=players.size();

    log(INFO,"netserver: giocatori connessi:%d\n",numPlayers);
    updateServerState();
}
void CMNetServer::DuelTimer(evutil_socket_t fd, short events, void* arg)
{
    CMNetServer* that = (CMNetServer* )arg;


    if(that->state != PLAYING)
        return;

    if(that->mode == MODE_SINGLE || that->mode == MODE_MATCH)
        SingleDuel::SingleTimer(fd,events,that->duel_mode);
    else if(that->mode == MODE_TAG)
        TagDuel::TagTimer(fd,events,that->duel_mode);

}

static int getNumDayOfWeek()
{
    time_t rawtime;
    tm * timeinfo;
    time(&rawtime);
    timeinfo=localtime(&rawtime);
    int wday=timeinfo->tm_wday;
    return wday;
}


void CMNetServer::createGame()
{
    event_base* net_evbase=roomManager->net_evbase;
    auto_idle = event_new(net_evbase, 0, EV_TIMEOUT , CMNetServer::auto_idle_cb, this);
    user_timeout = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, CMNetServer::user_timeout_cb, this);
    if(mode == MODE_SINGLE)
        duel_mode = new SingleDuel(false);
    else if(mode == MODE_MATCH)
        duel_mode = new SingleDuel(true);
    else if(mode == MODE_TAG)
        duel_mode = new TagDuel();

    duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, DuelTimer, this);

    BufferIO::CopyWStr("", duel_mode->name, 20);
    BufferIO::CopyWStr("", duel_mode->pass, 20);

    HostInfo info;
    info.rule=2;
    info.mode=mode;
    info.draw_count=1;
    info.no_check_deck=false;
    info.start_hand=5;
    info.lflist=1;
    info.time_limit=60;
    info.start_lp=(getNumDayOfWeek() == 6)?8000:8000;
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
    duel_mode->host_info = info;
    duel_mode->setNetServer(this);
    setState(WAITING);
    numPlayers=0;

    ReadyMessagesSent = 0;
    chatReady=true;
}
void CMNetServer::DisconnectPlayer(DuelPlayer* dp)
{
    log(VERBOSE,"DisconnectPlayer called\n");

    auto bit = players.find(dp);
    if(bit != players.end())
    {
        dp->netServer=nullptr;
        gameServer->DisconnectPlayer(dp);
    }
    playerDisconnected(dp);
    //dp->netServer=0;
}

void CMNetServer::ExtractPlayer(DuelPlayer* dp)
{
    //it removes the player from the duel without disconnecting its tcp connection
    log(VERBOSE,"ExtractPlayer called\n");
    playerDisconnected(dp);
    LeaveGame(dp);
}
void CMNetServer::InsertPlayer(DuelPlayer* dp)
{

    //it inserts forcefully the player into the server
    log(VERBOSE,"InsertPlayer called\n");
    playerConnected(dp);
    CTOS_JoinGame csjg;
    csjg.version = PRO_VERSION;
    csjg.gameid = 0;
    BufferIO::CopyWStr("", csjg.pass, 20);
    dp->game=0;
    dp->type=0xff;
    dp->netServer = this;
    duel_mode->JoinGame(dp, &csjg, false);

    duel_mode->host_player=NULL;
    players[dp].last_state_in_timeout = dp->state;
}


void CMNetServer::LeaveGame(DuelPlayer* dp)
{
    unsigned char oldstate = dp->state;
    unsigned char oldtype = dp->type;

    log(INFO,"leavegame chiamato\n");

    /*bug in match duel,
     * if the player leaves during side decking
     * the player MUST become a loser
     */
    if(state == PLAYING && mode == MODE_MATCH && last_winner > 0 && last_winner == dp->type)
    {
        last_winner = 1-dp->type;
    }

    /*bug in duels, sometimes a leaver doesn't become a loser*/
    if(oldtype != NETPLAYER_TYPE_OBSERVER && state == PLAYING && last_winner < 0)
    {
        if(mode == MODE_SINGLE || mode == MODE_MATCH)
            last_winner = 1-dp->type;
        else
            last_winner = (dp->type > 1)?0:1;
    }

    if(state != ZOMBIE && dp->game == duel_mode)
        duel_mode->LeaveGame(dp);
    else
        DisconnectPlayer(dp);

    if(players.find(dp)!=players.end())
    {
        //this is a bug in tagduel
        log(BUG,"player left but the duel didn't call DisconnectPlayer\n");
        DisconnectPlayer(dp);
    }

    if(oldtype != NETPLAYER_TYPE_OBSERVER && state == PLAYING)
    {
        log(BUG,"player left but the game is still playing\n");
        StopServer();
        for(auto it=players.cbegin(); it!= players.cend(); ++it)
        {
            if(it->first != dp)
                SendPacketToPlayer(it->first, STOC_DUEL_END);
        }
    }

}

void CMNetServer::StopBroadcast()
{
}

void CMNetServer::StopListen()
{
}

void CMNetServer::Victory(char winner)
{
    DuelPlayer* _players[4];
    for(int i = 0; i<4; i++)
        _players[i]=0;

    for(auto it = players.cbegin(); it!= players.cend(); ++it)
    {
        if(it->first->type <= NETPLAYER_TYPE_PLAYER4 && it->first->type >= NETPLAYER_TYPE_PLAYER1)
            _players[it->first->type] = it->first;
    }
    if(!_players[0])
        return;
    if(!_players[1])
        return;
    if(mode == MODE_TAG && !_players[2])
        return;
    if(mode == MODE_TAG && !_players[3])
        return;
    if((mode == MODE_SINGLE || mode == MODE_MATCH) && winner > 2)
        return;
    if(mode == MODE_TAG && winner > 2)
        return;

    //anti cheat
    if(mode!= MODE_TAG && !strcmp(_players[NETPLAYER_TYPE_PLAYER1]->ip,_players[NETPLAYER_TYPE_PLAYER2]->ip))
        winner = -1;

    if(winner < 0 || winner == 2)
    {
        if(mode != MODE_TAG)
        {
            char win[20], lose[20];

            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER1]->name,win,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER2]->name,lose,20);
            log(INFO,"SingleDuel, winner: %s, loser: %s\n",win,lose);
            std::string wins(win), loses(lose);
            Users::getInstance()->Draw(wins,loses);
        }
        else
        {
            char win1[20], win2[20], lose1[20],lose2[20];

            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER1]->name,win1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER2]->name,win2,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER3]->name,lose1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER4]->name,lose2,20);

            Users::getInstance()->Draw(std::string(win1),std::string(win2),std::string(lose1),std::string(lose2));
            log(INFO,"Tagduel finished: winners %s and %s, losers: %s and %s\n",win1,win2,lose1,lose2);
        }
        return;
    }

    if(mode == MODE_SINGLE || mode == MODE_MATCH)
    {
        char win[20], lose[20];

        BufferIO::CopyWStr(_players[winner]->name,win,20);
        BufferIO::CopyWStr(_players[1-winner]->name,lose,20);
        log(INFO,"SingleDuel, winner: %s, loser: %s\n",win,lose);
        std::string wins(win), loses(lose);
        Users::getInstance()->Victory(wins,loses);
    }
    else if(mode == MODE_TAG)
    {
        char win1[20], win2[20], lose1[20],lose2[20];

        if(winner != 1)
        {
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER1]->name,win1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER2]->name,win2,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER3]->name,lose1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER4]->name,lose2,20);
        }
        else
        {
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER3]->name,win1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER4]->name,win2,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER1]->name,lose1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER2]->name,lose2,20);

        }
        Users::getInstance()->Victory(std::string(win1),std::string(win2),std::string(lose1),std::string(lose2));
        log(INFO,"Tagduel finished: winners %s and %s, losers: %s and %s\n",win1,win2,lose1,lose2);
    }
}


void CMNetServer::StopServer()
{
    //the duel asked me to stop
    log(INFO,"netserver server diventato zombie\n");
    if(state==PLAYING)
    {
        Victory(last_winner);
        if(mode == MODE_TAG)
        {
            //Bug in tagduel.cpp
            //only the first two players were notified at the end of the duel
            /*for(auto it=players.cbegin(); it!= players.cend(); ++it)
            {
                if(it->first->type == NETPLAYER_TYPE_PLAYER3 || it->first->type == NETPLAYER_TYPE_PLAYER4)
                    SendPacketToPlayer(it->first, STOC_DUEL_END);
            }*/
        }



    }
    setState(ZOMBIE);
    updateServerState();
}

void CMNetServer::SystemChatToPlayer(DuelPlayer*dp, const std::wstring msg,bool isAdmin)
{
    if(chatReady)
    {
        CMNetServerInterface::SystemChatToPlayer(dp,msg,isAdmin);
    }
    else
    {
        players[dp].pendingMessages.push_back(std::pair<bool,std::wstring>(isAdmin,msg));
        if(players[dp].pendingMessages.size() > 5)
            players[dp].pendingMessages.pop_front();
    }

}

void CMNetServer::toObserver(DuelPlayer* dp)
{

    log(INFO,"to observer\n");
    duel_mode->ToObserver(dp);
    playerReadinessChange(dp,false);
    updateServerState();
}

void CMNetServer::updateUserTimeout(DuelPlayer* dp)
{
    log(VERBOSE,"user timeout update\n");
    players[dp].secondsWaiting=0;

}
void CMNetServer::user_timeout_cb(evutil_socket_t fd, short events, void* arg)
{
    CMNetServer* that = (CMNetServer*)arg;
    std::list<DuelPlayer *> deadUsers;
    log(VERBOSE,"timeout cb\n");
    const int maxTimeout = 300;

    for(auto it = that->players.begin(); it!=that->players.end(); ++it)
    {
        if(it->second.last_state_in_timeout != it->first->state)
        {
            it->second.last_state_in_timeout = it->first->state;
            it->second.secondsWaiting = 0;
            continue;
        }

        it->second.secondsWaiting += 10;
        if(it->first->type != NETPLAYER_TYPE_OBSERVER && it->second.secondsWaiting >= maxTimeout)
        {
            deadUsers.push_back(it->first);
        }
        else if(it->second.secondsWaiting >= 120 && that->mode == MODE_MATCH &&
                it->first->type != NETPLAYER_TYPE_OBSERVER && it->first->state == CTOS_UPDATE_DECK)
        {
            deadUsers.push_back(it->first);
        }
        else if(it->first->state ==CTOS_TIME_CONFIRM && it->second.secondsWaiting >= 20)
            that->duel_mode->TimeConfirm(it->first);
        else if(it->first->state ==CTOS_HAND_RESULT && it->second.secondsWaiting >= 60)
            deadUsers.push_back(it->first);
        //deadUsers.push_back(it->first);
        else if(that->state == ZOMBIE && it->second.zombiePlayer == false)
            it->second.zombiePlayer = true;
        else if(that->state == ZOMBIE && it->second.zombiePlayer)
            deadUsers.push_back(it->first);
    }
    for(auto it = deadUsers.begin(); it!= deadUsers.end(); ++it)
    {
        that->LeaveGame(*it);
    }
}



void CMNetServer::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{
    char* pdata = data;



    unsigned char pktType = BufferIO::ReadUInt8(pdata);

    if( players.end() == players.find(dp))
    {
        log(INFO,"BUG: handlectospacket ha ricevuto un pacchetto per un utente inesistente \n");
        return;
    }

    updateUserTimeout(dp);
    if(state==ZOMBIE)
    {
        log(INFO,"pacchetto ricevuto per uno zombie, ignorato\n");
        return;
    }



    if((pktType != CTOS_SURRENDER) && (pktType != CTOS_CHAT) && (dp->state == 0xff || (dp->state && dp->state != pktType)))
        return;

    switch(pktType)
    {
    case CTOS_RESPONSE:
    {
        if(!dp->game || !duel_mode->pduel)
            return;


        duel_mode->GetResponse(dp, pdata, len > 64 ? 64 : len - 1);
        int resp_type = dp->type;
        if(mode == MODE_TAG)
            resp_type= dp->type < 2 ? 0 : 1;
        if(duel_mode->time_limit[resp_type]>0 and duel_mode->time_limit[resp_type]<60)
            duel_mode->time_limit[resp_type] +=1;
        //printf("response inviato da %d\n",dp->type);
        ;
        break;
    }
    case CTOS_TIME_CONFIRM:
    {
        if(!dp->game || !duel_mode->pduel)
            return;
        duel_mode->TimeConfirm(dp);
        break;
    }
    case CTOS_CHAT:
    {
        if(!dp->game)
            return;
        wchar_t messaggio[256];
        int msglen = BufferIO::CopyWStr((unsigned short*) pdata,messaggio, 256);
        if(msglen != 0 && handleChatCommand(dp,messaggio))
            break;

        duel_mode->Chat(dp, pdata, len - 1);

        shout((unsigned short*)pdata,dp);

        break;
    }
    case CTOS_UPDATE_DECK:
    {
        if(!dp->game)
            return;
        duel_mode->UpdateDeck(dp, pdata);
        break;
    }
    case CTOS_HAND_RESULT:
    {
        if(!dp->game)
            return;
        CTOS_HandResult* pkt = (CTOS_HandResult*)pdata;
        dp->game->HandResult(dp, pkt->res);
        break;
    }
    case CTOS_TP_RESULT:
    {
        if(!dp->game)
            return;
        CTOS_TPResult* pkt = (CTOS_TPResult*)pdata;
        dp->game->TPResult(dp, pkt->res);
        break;
    }

    case CTOS_LEAVE_GAME:
    {
        if(!duel_mode)
            break;
        LeaveGame(dp);
        break;
    }
    case CTOS_SURRENDER:
    {
        if(!duel_mode)
            break;
        duel_mode->Surrender(dp);
        break;
    }
    case CTOS_HS_TODUELIST:
    {
        if(!duel_mode || duel_mode->pduel)
            break;
        playerReadinessChange(dp,false);
        duel_mode->ToDuelist(dp);
        updateServerState();


        /*  to avoid a bug in the XXXduel.cpp
            when a user clicks on "toduelist" his information
            about other user's readiness is not updated
        */
        for(auto it = players.cbegin(); it!=players.cend(); ++it)
        {
            if(it->first != dp && it->first->type != NETPLAYER_TYPE_OBSERVER && it->second.isReady)
            {
                STOC_HS_PlayerChange scpc;
                scpc.status = (it->first->type << 4) |PLAYERCHANGE_READY;

                SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
                log(INFO,"sent player change to p\n");
            }
        }



        break;
    }
    case CTOS_HS_TOOBSERVER:
    {
        if(!duel_mode || duel_mode->pduel)
            break;
        toObserver(dp);
        break;
    }
    case CTOS_HS_READY:
    case CTOS_HS_NOTREADY:
    {
        if(!duel_mode || duel_mode->pduel)
            break;
        playerReadinessChange(dp,CTOS_HS_NOTREADY - pktType);

        duel_mode->PlayerReady(dp, CTOS_HS_NOTREADY - pktType);
        if(pktType == CTOS_HS_READY)
        {
            duel_mode->host_player = dp;
            duel_mode->StartDuel(dp);
            duel_mode->host_player=NULL;
        }

        break;
    }
    case CTOS_HS_KICK:
    {
        if(!duel_mode || duel_mode->pduel)
            break;
        CTOS_Kick* pkt = (CTOS_Kick*)pdata;
        duel_mode->PlayerKick(dp, pkt->pos);
        break;
    }
    case CTOS_HS_START:
    {
        if(!duel_mode || duel_mode->pduel)
            break;

        break;
    }
    }

    if(isCrashed)
    {
        setState(ZOMBIE);
        updateServerState();
        isCrashed=false;
    }
}

bool CMNetServer::isCrashed = false;
void CMNetServer::crash_detected()
{
    isCrashed = true;

    FILE* fp = fopen("cm-error.log", "at");
    if(!fp)
        return;
    fprintf(fp, "server crashato pid: %d\n", (int) getpid());
    fclose(fp);
}

}


