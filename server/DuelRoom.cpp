#include "DuelRoom.h"
#include "single_duel.h"
#include "tag_duel.h"
#include "handicap_duel.h"
#include "GameServer.h"
#include "RoomManager.h"
#include "debug.h"
#include "Users.h"
#include <algorithm>
#include <signal.h>
#include <algorithm> 

static const int TIMEOUT_INTERVAL=2;
namespace ygo
{
DuelRoom::DuelRoom(RoomManager*roomManager,GameServer*gameServer,unsigned char mode)
    :RoomInterface(roomManager,gameServer),mode(mode),duel_mode(0),last_winner(-1),user_timeout(nullptr),lflist(3)
{
    createGame();
}


void DuelRoom::flushPendingMessages()
{
    if(!chatReady)
        return;
    for(auto it = players.begin(); it!=players.end(); ++it)
    {

        for(auto i=it->second.pendingMessages.begin(); i != it->second.pendingMessages.end(); i++)
        {
            RemoteChatToPlayer(it->first,i->second,i->first);

        }
        it->second.pendingMessages.clear();
    }

}


bool DuelRoom::isAvailableToPlayer(DuelPlayer* refdp, unsigned char refmode)
{
    if(state != DuelRoom::State::WAITING)
        return false;
	if(numPlayers == 0)
		return true;
    int reflflist = refdp->lflist;
    int refScore = refdp->cachedRankScore;
    if(abs(refScore - getFirstPlayer()->cachedRankScore) >= RoomManager::maxScoreDifference(refScore))
        return false;

    if(refmode != MODE_ANY && mode != refmode &&
        !(refmode == MODE_TAG && mode ==MODE_HANDICAP && getDpFromType(NETPLAYER_TYPE_PLAYER1) !=nullptr))
        return false;

    if(!(getLfList() == reflflist || getLfList() == 3 || reflflist == 3))
        return false;

    if(refdp->loginStatus == Users::LoginResult::AUTHENTICATED &&
        getFirstPlayer()->loginStatus == Users::LoginResult::AUTHENTICATED &&
        !wcscmp(refdp->namew_low,getFirstPlayer()->namew_low))
        return false;

    return true;
}



void DuelRoom::SendBufferToPlayer(DuelPlayer* dp, unsigned char proto, void* buffer, size_t len)
{
	if(players.find(dp) == players.end())
		return;
	if(dp->netServer != this)
	{
		printf("MEGABUG, ho un giocatore che non dovrebbe esistere\n");
		
		LeaveGame(dp );
		setState(ZOMBIE);
		updateServerState();
		return;
	}
	
	logger.LogServerMessage((uintptr_t) dp,proto,(char*)buffer,len);
	
	if(proto == STOC_TYPE_CHANGE)
	{
		STOC_TypeChange *sctc = (STOC_TypeChange *) buffer;
		sctc->type &= ~ 0x10;
		
		
	}
	if(proto == STOC_DUEL_END)
	{
		char* p = net_server_write;
		BufferIO::WriteInt16(p, 1);
		BufferIO::WriteInt8(p, proto);
		return;
	}
    if(proto == STOC_DUEL_START)
    {
        STOC_JoinGame scjg;
        scjg.info = duel_mode->host_info;
        scjg.info.time_limit=std::max(Config::getInstance()->maxTimer,Config::getInstance()->startTimer);
		scjg.info.enable_priority= false;
        RoomInterface::SendBufferToPlayer(dp, STOC_JOIN_GAME, &scjg,sizeof(STOC_JoinGame));

        for(auto it:players)
        {
            STOC_HS_PlayerEnter scpe;
            BufferIO::CopyWStr(it.first->name, scpe.name, 20);
            scpe.pos = it.first->type;
            RoomInterface::SendBufferToPlayer(dp, STOC_HS_PLAYER_ENTER, &scpe,sizeof(STOC_HS_PlayerEnter));

        }


    }
    else if(proto == STOC_JOIN_GAME)
    {
        STOC_JoinGame *scjg = (STOC_JoinGame *)buffer;
        scjg->info.time_limit=0;
		scjg->info.enable_priority= false;

    }


    RoomInterface::SendBufferToPlayer(dp,proto,buffer,len);
    if(proto == STOC_GAME_MSG)
    {
        unsigned char* wbuf = (unsigned char*)buffer;
        ultimo_game_message = wbuf[0];
        if(wbuf[0] == MSG_WIN && state == PLAYING)
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
			
			/* PESCE D'APRILE
			{
				char buf[16];
				int *val = (int*) &buf[4];
				buf[3] = dp->type;
				buf[2]=MSG_LPUPDATE;
				*val = 108000;
				RoomInterface::SendBufferToPlayer(dp, STOC_GAME_MSG, &buf[2],6);
			}*/
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
	if (proto == STOC_HS_PLAYER_CHANGE)
	{
		STOC_HS_PlayerChange *scpc = (STOC_HS_PlayerChange *) buffer;
		if(dp->type == (scpc->status >>4) && ((scpc->status&0x0f) == PLAYERCHANGE_READY) != players[dp].isReady)
		{
		   playerReadinessChange(dp,!players[dp].isReady);
		   log(BUG,"bug in readiness\n");
		}
		
	}
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

DuelPlayer * DuelRoom::getDpFromType(unsigned char type)
{
    for(auto it = players.cbegin(); it!=players.cend(); ++it)
    {
        if(it->first->type == type)
            return it->first;
    }
    return nullptr;

}

void DuelRoom::auto_idle_cb(evutil_socket_t fd, short events, void* arg)
{
    DuelRoom* that = (DuelRoom*)arg;
    log(VERBOSE,"auto idle_cb\n");
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

void DuelRoom::ShowPlayerScores()
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

        swprintf(buffer,64,L"%d",it->first->cachedGameScore);
        wcscat(message,buffer);
    }
    BroadcastSystemChat(message,true);
}

void DuelRoom::ShowPlayerOdds()
{
    char message[256];



    if(mode != MODE_SINGLE && mode != MODE_MATCH)
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

    float odds;
    char* chosen_one;
    if(_players[0]->cachedRankScore > _players[1]->cachedRankScore)
    {
        odds = 100.0*Users::getInstance()->win_exp(_players[0]->cachedRankScore - _players[1]->cachedRankScore);
        chosen_one = name0;
    }
    else
    {
        odds = 100.0*Users::getInstance()->win_exp(_players[1]->cachedRankScore - _players[0]->cachedRankScore);
        chosen_one = name1;
    }

    sprintf(message,"There is a %d%% chance that %s is going to win this duel",(int) odds,chosen_one);

    std::string temp(message);
    BroadcastSystemChat(std::wstring(temp.begin(),temp.end()),true);
    BroadcastSystemChat(L"View the full statistics at http://www.ygopro.it",true);
}

int DuelRoom::getLfList()
{
   return lflist;
}

void DuelRoom::clientStarted()
{
    if(state==FULL)
    {
        setState(PLAYING);
        timeval timeout = {TIMEOUT_INTERVAL, 0};
        event_add(user_timeout, &timeout);
        chatReady=false;
        ShowPlayerOdds();
        ShowPlayerScores();
        //duel_mode->host_info.time_limit=60;
    }

}


void DuelRoom::setState(State state)
{
    if(this->state != state)
        roomManager->notifyStateChange(this,this->state,state);
    this->state=state;
}

void DuelRoom::destroyGame()
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

int DuelRoom::getMaxDuelPlayers()
{
    int maxplayers = 2;
    if(mode == MODE_TAG)
        maxplayers=4;
    else if(mode == MODE_HANDICAP)
        maxplayers=3;
    return maxplayers;
}



void DuelRoom::playerConnected(DuelPlayer *dp)
{
    if(players.find(dp)==players.end())
        players[dp] = DuelPlayerInfo();
    numPlayers=players.size();

    log(VERBOSE,"netserver: giocatori connessi:%d\n",numPlayers);
    updateServerState();
}

int DuelRoom::getNumDuelPlayers()
{
    int n=0;

    for(auto it = players.cbegin(); it!=players.cend(); ++it)
    {
        if(it->first->type != NETPLAYER_TYPE_OBSERVER && !it->second.zombiePlayer)
            n++;
    }
    return n;
}

void DuelRoom::updateServerState()
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
        log(VERBOSE,"server vuoto. addio, morto\n");

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

void DuelRoom::playerDisconnected(DuelPlayer* dp )
{
    if(players.find(dp)!=players.end())
        players.erase(dp);
    numPlayers=players.size();

    log(VERBOSE,"netserver: giocatori connessi:%d\n",numPlayers);
    updateServerState();
}
void DuelRoom::DuelTimer(evutil_socket_t fd, short events, void* arg)
{
    DuelRoom* that = (DuelRoom* )arg;


    if(that->state != PLAYING)
        return;

    if(that->mode == MODE_SINGLE || that->mode == MODE_MATCH)
        SingleDuel::SingleTimer(fd,events,that->duel_mode);
    else if(that->mode == MODE_TAG)
        TagDuel::TagTimer(fd,events,that->duel_mode);
    else if(that->mode == MODE_HANDICAP)
        HandicapDuel::TagTimer(fd,events,that->duel_mode);


/** inizio il codice per auto terminare al momento del bisogno **/
    auto last_response = that->duel_mode->last_response;
    int limite =  that->duel_mode->time_limit[last_response];
    int trascorso = that->duel_mode->time_elapsed;
    int rimanente = limite - trascorso ;
    if(rimanente != 4 && rimanente != 3)
        return;

	/* ORA VEDO SE POSSO FARE QUALCOSA*/
	char buffer[5];
	buffer[0] = CTOS_RESPONSE;
    int *risposta =(int*) &buffer[1];
    if(that->ultimo_game_message == MSG_SELECT_BATTLECMD) {
		*risposta = 3;
	}
	else if(that->ultimo_game_message == 16)
	{
		*risposta = -1;
	}
	else if(that->ultimo_game_message == MSG_SELECT_IDLECMD) {
		*risposta = 7;
	}
	else if(that->ultimo_game_message == 15) {
		*risposta = 1;
	}
	else
        return;
	
	/* CERCO L'UTENTE */
    DuelPlayer *dpattivo = nullptr;
    for(auto it = that->players.cbegin(); it!=that->players.cend(); ++it)
    {
        if(it->first->state == CTOS_RESPONSE)
            dpattivo = it->first;
	}
    if(dpattivo == nullptr)
        return;
    
    that->HandleCTOSPacket(dpattivo, buffer, 5);
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


void DuelRoom::createGame()
{
    event_base* net_evbase=roomManager->net_evbase;
    auto_idle = event_new(net_evbase, 0, EV_TIMEOUT , DuelRoom::auto_idle_cb, this);
    user_timeout = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, DuelRoom::user_timeout_cb, this);
    if(mode == MODE_SINGLE)
        duel_mode = new SingleDuel(false);
    else if(mode == MODE_MATCH)
        duel_mode = new SingleDuel(true);
    else if(mode == MODE_TAG)
        duel_mode = new TagDuel();
    else if(mode == MODE_HANDICAP)
        duel_mode = new HandicapDuel();

    duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, DuelTimer, this);

    BufferIO::CopyWStr("", duel_mode->name, 20);
    BufferIO::CopyWStr("", duel_mode->pass, 20);

    HostInfo info;
    info.rule=2;
    info.mode=mode==MODE_HANDICAP?MODE_TAG:mode;
    info.draw_count=1;
    info.no_check_deck=false;
    info.start_hand=5;
    info.lflist=0;
    info.time_limit=Config::getInstance()->startTimer;
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

void DuelRoom::DisconnectPlayer(DuelPlayer* dp)
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

std::map<DuelPlayer*, DuelPlayerInfo> DuelRoom::ExtractAllPlayers()
{
	std::map<DuelPlayer*, DuelPlayerInfo> p = players;
	
	for(auto it = p.cbegin();it!= p.cend();++it)
		ExtractPlayer(it->first);
	return p;
}

void DuelRoom::ExtractPlayer(DuelPlayer* dp)
{
	
    //it removes the player from the duel without disconnecting its tcp connection
    log(VERBOSE,"ExtractPlayer called\n");
    playerDisconnected(dp);
    LeaveGame(dp);
	dp->netServer = nullptr;
	dp->game = nullptr;
}
void DuelRoom::InsertPlayer(DuelPlayer* dp)
{
	if(state >= PLAYING)
		return;
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
    reCheckLfList();

    if(mode == MODE_HANDICAP)
    {
        STOC_HS_PlayerEnter scpe;
        BufferIO::CopyWStr(L"[HANDICAP MATCH]", scpe.name, 20);
        scpe.pos = 0;
        SendPacketToPlayer(dp, STOC_HS_PLAYER_ENTER, scpe);
    }

}


void DuelRoom::LeaveGame(DuelPlayer* dp)
{
    unsigned char oldstate = dp->state;
    unsigned char oldtype = dp->type;

    log(VERBOSE,"leavegame chiamato\n");
	if(players.find(dp)!=players.end())
		players[dp].zombiePlayer = true;

		
    /*bug in match duel,
     * if the player leaves during side decking
     * the player MUST become a loser
     */
    if(state == PLAYING && mode == MODE_MATCH && last_winner == dp->type)
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

    if(state == PLAYING && dp->game == duel_mode && mode == MODE_TAG && oldtype != NETPLAYER_TYPE_OBSERVER)
    {
         log(BUG,"player disconnected in tag, inform about duel result\n");
         
         wchar_t nome[40];
         BufferIO::CopyWStr(dp->name,nome,40);
         std::wstring messaggio = L"Lost connection with: " + std::wstring(nome);
         BroadcastRemoteChat(messaggio,-1);
		 
		 //informa tutti del risultato
         char buf[3];
         buf[0] = MSG_WIN;
         buf[1] = 1-(oldtype/2);
         buf[2] = 0x04;
         for(auto it=players.cbegin(); it!= players.cend(); ++it)
			SendBufferToPlayer(it->first,STOC_GAME_MSG,buf,3);
         
        
    }

    if(state != ZOMBIE && dp->game == duel_mode && duel_mode != 0)
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

void DuelRoom::StopBroadcast()
{
}

void DuelRoom::StopListen()
{
}

void DuelRoom::Victory(char winner)
{
    if(last_winner < 0)
        return;
    DuelPlayer* _players[4];
    for(int i = 0; i<4; i++)
        _players[i]=0;

    for(auto it = players.cbegin(); it!= players.cend(); ++it)
    {
        if(it->first->type <= NETPLAYER_TYPE_PLAYER4 && it->first->type >= NETPLAYER_TYPE_PLAYER1)
            _players[it->first->type] = it->first;
    }
    if(!_players[0] && mode != MODE_HANDICAP)
        return;
    if(!_players[1])
        return;
    if((mode == MODE_TAG || mode == MODE_HANDICAP) && !_players[2])
        return;
    if((mode == MODE_TAG || mode == MODE_HANDICAP) && !_players[3])
        return;
    if((mode == MODE_SINGLE || mode == MODE_MATCH) && winner > 2)
        return;
    if(mode == MODE_TAG && winner > 2)
        return;

    //anti cheat
    if((mode== MODE_SINGLE || mode == MODE_MATCH) && !strcmp(_players[NETPLAYER_TYPE_PLAYER1]->ip,_players[NETPLAYER_TYPE_PLAYER2]->ip))
        winner = -1;

    if(winner < 0 || winner == 2)
    {
        if(mode == MODE_SINGLE || mode == MODE_MATCH)
        {
            char win[20], lose[20];

            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER1]->name,win,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER2]->name,lose,20);
            log(INFO,"SingleDuel, winner: %s, loser: %s\n",win,lose);
            std::string wins(win), loses(lose);
            Users::getInstance()->Draw(wins,loses);
        }
        else if (mode == MODE_TAG)
        {
            char win1[20], win2[20], lose1[20],lose2[20];

            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER1]->name,win1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER2]->name,win2,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER3]->name,lose1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER4]->name,lose2,20);

            Users::getInstance()->Draw(std::string(win1),std::string(win2),std::string(lose1),std::string(lose2));
            log(INFO,"Tagduel finished: winners %s and %s, losers: %s and %s\n",win1,win2,lose1,lose2);
        }
        else if (mode == MODE_HANDICAP)
        {
            char win2[20], lose1[20],lose2[20];

            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER2]->name,win2,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER3]->name,lose1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER4]->name,lose2,20);

            Users::getInstance()->Draw(std::string(win2),std::string(win2),std::string(lose1),std::string(lose2));
            log(INFO,"Handicapduel finshed, draw: %s VS %s and %s\n",win2,win2,lose1,lose2);



        }
        return;
    }

    if(mode == MODE_SINGLE || mode == MODE_MATCH)
    {
        char win[20], lose[20];

        BufferIO::CopyWStr(_players[winner]->name,win,20);
        BufferIO::CopyWStr(_players[1-winner]->name,lose,20);
        
		/*log(INFO,"SingleDuel, winner: %s, loser: %s\n",win,lose);
        std::string wins(win), loses(lose);
        Users::getInstance()->Victory(wins,loses);
		*/
		
		if(strcmp(win,logger.getPlayerInfo((uintptr_t) _players[winner])->name))
			log(BUG,"nome sbagliato %s %s\n",win,logger.getPlayerInfo((uintptr_t) _players[winner])->name);
		std::vector<LoggerPlayerInfo*> nomi;
		nomi.push_back(logger.getPlayerInfo((uintptr_t) _players[winner]));
		nomi.push_back(logger.getPlayerInfo((uintptr_t) _players[1-winner]));
	
		Users::getInstance()->UpdateStats(nomi,0);
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
    else if (mode == MODE_HANDICAP)
    {
        char win1[20], win2[20], lose1[20],lose2[20];

        if(winner != 1)
        {
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER2]->name,win1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER2]->name,win2,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER3]->name,lose1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER4]->name,lose2,20);
        }
        else
        {
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER3]->name,win1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER4]->name,win2,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER2]->name,lose1,20);
            BufferIO::CopyWStr(_players[NETPLAYER_TYPE_PLAYER2]->name,lose2,20);

        }
        Users::getInstance()->Victory(std::string(win1),std::string(win2),std::string(lose1),std::string(lose2));
        log(INFO,"Handicap duel finished: winners %s and %s, losers: %s and %s\n",win1,win2,lose1,lose2);


    }
}


void DuelRoom::StopServer()
{
    //the duel asked me to stop
    log(VERBOSE,"netserver server diventato zombie\n");
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

void DuelRoom::RemoteChatToPlayer(DuelPlayer* dp, std::wstring msg,int color)
{
	if(chatReady)
    {
        RoomInterface::RemoteChatToPlayer(dp,msg,color);
    }
    else
    {
        players[dp].pendingMessages.push_back(std::pair<int,std::wstring>(color,msg));
        if(players[dp].pendingMessages.size() > 5)
            players[dp].pendingMessages.pop_front();
    }
	
	
}
void DuelRoom::SystemChatToPlayer(DuelPlayer*dp, const std::wstring msg,bool isAdmin,int color)
{
    RemoteChatToPlayer(dp,msg,isAdmin?-2:color);
}

void DuelRoom::toObserver(DuelPlayer* dp)
{
    bool wasReady = players[dp].isReady;
    log(VERBOSE,"to observer\n");
    duel_mode->ToObserver(dp);

    playerReadinessChange(dp,false);
    updateServerState();
    if(wasReady)
        reCheckLfList();
}

void DuelRoom::updateUserTimeout(DuelPlayer* dp)
{
    log(VERBOSE,"user timeout update\n");
    players[dp].secondsWaiting=0;

}
void DuelRoom::user_timeout_cb(evutil_socket_t fd, short events, void* arg)
{
    DuelRoom* that = (DuelRoom*)arg;
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

        it->second.secondsWaiting += TIMEOUT_INTERVAL;
        if(it->first->type != NETPLAYER_TYPE_OBSERVER && it->second.secondsWaiting >= maxTimeout)
        {
            deadUsers.push_back(it->first);
        }
        else if(it->second.secondsWaiting >= 120 && that->mode == MODE_MATCH &&
                it->first->type != NETPLAYER_TYPE_OBSERVER && it->first->state == CTOS_UPDATE_DECK)
        {
            deadUsers.push_back(it->first);
        }
        else if(it->first->state ==CTOS_TIME_CONFIRM && it->second.secondsWaiting >= 2)
            that->duel_mode->TimeConfirm(it->first);
        else if(it->first->state ==CTOS_HAND_RESULT && it->second.secondsWaiting >= 60)
            deadUsers.push_back(it->first);
        //deadUsers.push_back(it->first);
        
    }
    for(auto it = deadUsers.begin(); it!= deadUsers.end(); ++it)
    {
        that->LeaveGame(*it);
    }
}

bool DuelRoom::reCheckLfList()
{
 /*           int lflist_intersection=3;
            for(auto it = players.cbegin(); it!=players.cend(); ++it)
            {
                if(it->first->type <= NETPLAYER_TYPE_PLAYER4 && it->second.isReady)
                    lflist_intersection &= it->first->lflist;
            }
            printf("intersezione %d\n",lflist_intersection);


            bool prosegui = lflist==3 && lflist_intersection <3 && lflist_intersection > 0;
             prosegui = prosegui || (lflist <3 && lflist_intersection == 3);
*/
            int lflist_intersection=3;
            for(auto it = players.cbegin(); it!=players.cend(); ++it)
            {
                    lflist_intersection &= it->first->lflist;
            }
            bool prosegui = (lflist ==3) && lflist_intersection > 0;

            if(!duel_mode)
                prosegui = false;
            if(!prosegui)
                return false;

            //non resettare la stanza allo stato iniziale
            if(lflist_intersection ==3)
                return false;

            //ora iniziamo a salvare lo stato della stanza
            lflist = lflist_intersection;

            unsigned int list_hash=0;

            list_hash=deckManager._lfList[lflist-1].hash;


            STOC_JoinGame scjg;
            HostInfo info;

            info.rule=(Config::getInstance()->strictAllowedList)?(lflist-1):2;
            info.mode=mode==MODE_HANDICAP?MODE_TAG:mode;
            info.draw_count=1;
            info.no_check_deck=false;
            info.start_hand=5;
            info.lflist=list_hash;
            info.time_limit=Config::getInstance()->startTimer;
            info.start_lp=8000;
            info.enable_priority=false;
            info.no_shuffle_deck=false;

            duel_mode->host_info = info;
            scjg.info = info;

            /*for(auto it = players.cbegin(); it!=players.cend(); ++it)
            {
                SendPacketToPlayer(it->first, STOC_JOIN_GAME, scjg);
            }*/

            for(auto it = players.cbegin(); it!=players.cend(); ++it)
            {
                SendPacketToPlayer(it->first, STOC_JOIN_GAME, scjg);

                    STOC_TypeChange sctc;
                    sctc.type = 0;
                    sctc.type |= it->first->type;
                    SendPacketToPlayer(it->first, STOC_TYPE_CHANGE, sctc);


                for(auto iit = players.cbegin(); iit!=players.cend(); ++iit)
                {
                    if(iit->first->type <= NETPLAYER_TYPE_PLAYER4)
                    {
                        STOC_HS_PlayerEnter scpe;
                        scpe.pos = iit->first->type;
                        BufferIO::CopyWStr(iit->first->name, scpe.name, 20);
                        SendPacketToPlayer(it->first, STOC_HS_PLAYER_ENTER, scpe);

                        STOC_HS_PlayerChange scpc;
                        scpc.status = iit->second.isReady? PLAYERCHANGE_READY:PLAYERCHANGE_NOTREADY;
                        scpc.status |= (iit->first->type << 4);
                        SendPacketToPlayer(it->first, STOC_HS_PLAYER_CHANGE, scpc);
                    }




                }



            }






return true;

}


void DuelRoom::RoomChat(DuelPlayer* dp, std::wstring messaggio)
{
	if(!dp->game)
            return;
	STOC_Chat scc;
	scc.player = dp->type;
	
	int msglen = BufferIO::CopyWStr(messaggio.c_str(), scc.msg, 256);
	for(auto it=players.begin(); it!=players.end(); ++it)
    {
        if(it->first== dp)
            continue;
        SendBufferToPlayer(it->first, STOC_CHAT, &scc, 4 + msglen * 2);
    }
	
}

void DuelRoom::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{
    char* pdata = data;



    unsigned char pktType = BufferIO::ReadUInt8(pdata);

    if( players.end() == players.find(dp))
    {
        log(INFO,"BUG: handlectospacket ha ricevuto un pacchetto per un utente inesistente \n");
        return;
    }

    
    if(state==ZOMBIE)
    {
        log(INFO,"pacchetto ricevuto per uno zombie, ignorato\n");
        return;
    }
	updateUserTimeout(dp);

    if((pktType != CTOS_SURRENDER) && (pktType != CTOS_CHAT) && (dp->state == 0xff || (dp->state && dp->state != pktType)))
        return;

	logger.LogClientMessage((uintptr_t)dp,pktType,data,len-1);
	
    switch(pktType)
    {
    case CTOS_RESPONSE:
    {
        if(!dp->game || !duel_mode->pduel)
            return;

        attiva_segnali();
        try
        {
			duel_mode->GetResponse(dp, pdata, len > 64 ? 64 : len - 1);
            
			int resp_type = dp->type;
            if(mode == MODE_TAG)
                resp_type= dp->type < 2 ? 0 : 1;
            if(logger.getPlayerInfo((uintptr_t)dp)->hisTurn and duel_mode->time_limit[resp_type]>0 and duel_mode->time_limit[resp_type]<Config::getInstance()->maxTimer)
                duel_mode->time_limit[resp_type] +=1;
        }
        catch (std::string &errore)
        {
            printf("aiuto!\n");
            last_winner = -1;
            setState(ZOMBIE);
            updateServerState();

            //kill(SIGINT,getpid());

        }
        disattiva_segnali();

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
        wchar_t messaggio[256];

        int msglen = BufferIO::CopyWStr((unsigned short*) pdata,messaggio, 256);
        unsigned short* msgbuf = (unsigned short*)pdata;
        if(!dp->game)
            return;

        

        duel_mode->Chat(dp, msgbuf, len - 1);

        break;
    }
    case CTOS_UPDATE_DECK:
    {
        dp->lflist = detectDeckCompatibleLflist(pdata);

        if(!dp->game)
            return;
		memcpy(players[dp].deck, data, len);
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
        if(!duel_mode || duel_mode->pduel || state ==PLAYING)
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
                log(VERBOSE,"sent player change to p\n");
            }
        }



        break;
    }
    case CTOS_HS_TOOBSERVER:
    {
        if(!duel_mode || duel_mode->pduel || state ==PLAYING)
            break;
        toObserver(dp);
        break;
    }
    case CTOS_HS_READY:
        if(dp->lflist <=0)
        {
            STOC_HS_PlayerChange scpc;
			scpc.status = (dp->type << 4) | PLAYERCHANGE_NOTREADY;
			SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
			STOC_ErrorMsg scem;
			scem.msg = ERRMSG_DECKERROR;
			scem.code = -dp->lflist;
			SendPacketToPlayer(dp, STOC_ERROR_MSG, scem);
            dp->lflist = 0;
            break;
        }
        if((dp->lflist == 2 && lflist ==1) || (dp->lflist == 1 && lflist ==2))
        {
            STOC_ErrorMsg scem;
            scem.msg = ERRMSG_DECKERROR;
			scem.code = 1;
			SendPacketToPlayer(dp, STOC_ERROR_MSG, scem);
            ExtractPlayer(dp);
            roomManager->InsertPlayer(dp,mode);
            break;
        }

    case CTOS_HS_NOTREADY:
    {
        if(!duel_mode || duel_mode->pduel || state ==PLAYING)
            break;
        playerReadinessChange(dp,CTOS_HS_NOTREADY - pktType);

        duel_mode->PlayerReady(dp, CTOS_HS_NOTREADY - pktType);

           /*     if(reCheckLfList())
        {
                    STOC_HS_PlayerChange scpc;
                        scpc.status =  PLAYERCHANGE_READY;
                        scpc.status |= (dp->type<< 4);
                        SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
        }*/


        if(pktType == CTOS_HS_READY)
        {
            duel_mode->host_player = dp;
            duel_mode->StartDuel(dp);
            duel_mode->host_player=NULL;
        }
        if(state != PLAYING)
            reCheckLfList();
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




}



}


