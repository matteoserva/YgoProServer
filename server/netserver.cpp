#include "netserver.h"
#include "single_duel.h"
#include "tag_duel.h"
#include "GameServer.h"
namespace ygo {

    NetServer::NetServer()
    {
        server_port = 0;
        net_evbase = 0;
        broadcast_ev = 0;
        listener = 0;
        duel_mode = 0;
        last_sent = 0;

        createGame();


        //
    }
void NetServer::clientStarted()
{
      state = PLAYING;


}
void NetServer::createGame()
{
        net_evbase = event_base_new();
        duel_mode = new SingleDuel(false);
            duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, SingleDuel::SingleTimer, duel_mode);

        Thread::NewThread(ServerThread, this);

        BufferIO::CopyWStr(L"", duel_mode->name, 20);
		BufferIO::CopyWStr(L"", duel_mode->pass, 20);

		HostInfo info;
		info.rule=0;
		info.mode=0;
		info.draw_count=1;
		info.no_check_deck=false;
		info.start_hand=5;
		info.lflist=1;
		info.time_limit=120;
		info.start_lp=8000;
		info.enable_priority=false;
		info.no_shuffle_deck=false;
		unsigned int hash = 1;
		for(auto lfit = deckManager._lfList.begin(); lfit != deckManager._lfList.end(); ++lfit) {
			if(info.lflist == lfit->hash) {
				hash = info.lflist;
				break;
			}
		}
		if(hash == 1)
			info.lflist = deckManager._lfList[0].hash;
		duel_mode->host_info = info;
		duel_mode->setNetServer(this);
}
void NetServer::DisconnectPlayer(DuelPlayer* dp) {
    /*
        This is called from DuelMode only.(singleduel and tagduel)


    */

    gameServer->DisconnectPlayer(dp);
            players--;
			printf("giocatori connessi:%d\n",players);


	if(!players)
	{
	    printf("server vuoto. addio\n");

	    event_base_loopexit(net_evbase, 0);


	}
}


void NetServer::keepAlive(evutil_socket_t fd, short events, void* arg) {
    printf("keepAlive\n");
    event* ev1 = (event*)arg;
        timeval timeout = {5, 0};


}

int NetServer::ServerThread(void* parama) {
    NetServer* that = (NetServer*)parama;

    sleep(1);
    printf("sono serverthread netserver\n");
    timeval timeout = {5, 0};

   event* ev1 = event_new(that->net_evbase, 0, EV_TIMEOUT | EV_PERSIST, keepAlive, ev1);
    event_add(ev1, &timeout);
	event_base_dispatch(that->net_evbase);
    event_del(ev1);
    printf("netserver thread terminato\n");
    	if(that->duel_mode){
		that->duel_mode->EndDuel();
		event_del(that->duel_mode->etimer);
		event_free(that->duel_mode->etimer);
		delete that->duel_mode;

	}
	event_free(ev1);

	event_base_free(that->net_evbase);
	that->createGame();


	return 0;
}

void NetServer::LeaveGame(DuelPlayer* dp)
{
    if(state != ZOMBIE && dp->game == duel_mode)
    {

        duel_mode->LeaveGame(dp);
    }


}

void NetServer::StopBroadcast()
{

    //event_base_loopexit(net_evbase, 0);
    //TODO this.state = stopped


}

void NetServer::StopListen()
{

    //event_base_loopexit(net_evbase, 0);
    //TODO this.state = stopped
}



void NetServer::StopServer() {
    if(net_evbase)
		{
                event_base_loopexit(net_evbase, 0);


		}
		printf("server stoppato\n");

	printf("netserver server stoppato");
    state = ZOMBIE;
       // createGame();
}


void NetServer::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len) {
	char* pdata = data;

	if(state==ZOMBIE)
        return;

	unsigned char pktType = BufferIO::ReadUInt8(pdata);
	if((pktType != CTOS_SURRENDER) && (pktType != CTOS_CHAT) && (dp->state == 0xff || (dp->state && dp->state != pktType)))
		return;
	switch(pktType) {
	case CTOS_RESPONSE: {
		if(!dp->game || !duel_mode->pduel)
			return;
		duel_mode->GetResponse(dp, pdata, len > 64 ? 64 : len - 1);
		break;
	}
	case CTOS_TIME_CONFIRM: {
		if(!dp->game || !duel_mode->pduel)
			return;
		duel_mode->TimeConfirm(dp);
		break;
	}
	case CTOS_CHAT: {
		if(!dp->game)
			return;
		duel_mode->Chat(dp, pdata, len - 1);
		break;
	}
	case CTOS_UPDATE_DECK: {
		if(!dp->game)
			return;
		duel_mode->UpdateDeck(dp, pdata);
		break;
	}
	case CTOS_HAND_RESULT: {
		if(!dp->game)
			return;
		CTOS_HandResult* pkt = (CTOS_HandResult*)pdata;
		dp->game->HandResult(dp, pkt->res);
		break;
	}
	case CTOS_TP_RESULT: {
		if(!dp->game)
			return;
		CTOS_TPResult* pkt = (CTOS_TPResult*)pdata;
		dp->game->TPResult(dp, pkt->res);
		break;
	}
	case CTOS_PLAYER_INFO: {
		CTOS_PlayerInfo* pkt = (CTOS_PlayerInfo*)pdata;
		BufferIO::CopyWStr(pkt->name, dp->name, 20);
		printf("playerinfo ricevuto da NetServer\n");
		break;
	}
	case CTOS_CREATE_GAME: {
		if(dp->game || duel_mode)
			return;
		CTOS_CreateGame* pkt = (CTOS_CreateGame*)pdata;
		if(pkt->info.mode == MODE_SINGLE) {
			duel_mode = new SingleDuel(false);
			duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, SingleDuel::SingleTimer, duel_mode);
		} else if(pkt->info.mode == MODE_MATCH) {
			duel_mode = new SingleDuel(true);
			duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, SingleDuel::SingleTimer, duel_mode);
		} else if(pkt->info.mode == MODE_TAG) {
			duel_mode = new TagDuel();
			duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, TagDuel::TagTimer, duel_mode);
		}
		duel_mode->setNetServer(this);
		if(pkt->info.rule > 3)
			pkt->info.rule = 0;
		if(pkt->info.mode > 2)
			pkt->info.mode = 0;
		unsigned int hash = 1;
		for(auto lfit = deckManager._lfList.begin(); lfit != deckManager._lfList.end(); ++lfit) {
			if(pkt->info.lflist == lfit->hash) {
				hash = pkt->info.lflist;
				break;
			}
		}
		if(hash == 1)
			pkt->info.lflist = deckManager._lfList[0].hash;
		duel_mode->host_info = pkt->info;
		BufferIO::CopyWStr(pkt->name, duel_mode->name, 20);
		BufferIO::CopyWStr(pkt->pass, duel_mode->pass, 20);
		duel_mode->JoinGame(dp, 0, true);
		break;
	}
	case CTOS_JOIN_GAME: {
		if(!duel_mode)
			break;
			 players++;
			printf("giocatori connessi:%d\n",players);
		duel_mode->JoinGame(dp, pdata, false);
		break;
	}
	case CTOS_LEAVE_GAME: {
		if(!duel_mode)
			break;
			 players--;
			printf("giocatori connessi:%d\n",players);
		duel_mode->LeaveGame(dp);
		break;
	}
	case CTOS_SURRENDER: {
		if(!duel_mode)
			break;
		duel_mode->Surrender(dp);
		break;
	}
	case CTOS_HS_TODUELIST: {
		if(!duel_mode || duel_mode->pduel)
			break;
		duel_mode->ToDuelist(dp);
		break;
	}
	case CTOS_HS_TOOBSERVER: {
		if(!duel_mode || duel_mode->pduel)
			break;
		duel_mode->ToObserver(dp);
		break;
	}
	case CTOS_HS_READY:
	case CTOS_HS_NOTREADY: {
		if(!duel_mode || duel_mode->pduel)
			break;
		duel_mode->PlayerReady(dp, CTOS_HS_NOTREADY - pktType);
		break;
	}
	case CTOS_HS_KICK: {
		if(!duel_mode || duel_mode->pduel)
			break;
		CTOS_Kick* pkt = (CTOS_Kick*)pdata;
		duel_mode->PlayerKick(dp, pkt->pos);
		break;
	}
	case CTOS_HS_START: {
		if(!duel_mode || duel_mode->pduel)
			break;
		duel_mode->StartDuel(dp);
		break;
	}
	}
}

}
