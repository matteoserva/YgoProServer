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



        duel_mode = new TagDuel();
			duel_mode->etimer = event_new(net_evbase, 0, EV_TIMEOUT | EV_PERSIST, SingleDuel::SingleTimer, duel_mode);
        BufferIO::CopyWStr(L"", duel_mode->name, 20);
		BufferIO::CopyWStr(L"", duel_mode->pass, 20);

		HostInfo info;
		info.rule=0;
		info.mode=0;
		info.lflist=1;
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

    }

void NetServer::DisconnectPlayer(DuelPlayer* dp) {
    gameServer->DisconnectPlayer(dp);
}


void NetServer::StopListen()
{

    //TODO this.state = stopped
}



void NetServer::StopServer() {

	if(duel_mode){
		//duel_mode->EndDuel();
		//event_free(duel_mode->etimer);
		//delete duel_mode;
		//duel_mode=0;
	}


}


void NetServer::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len) {
	char* pdata = data;
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
		duel_mode->JoinGame(dp, pdata, false);
		break;
	}
	case CTOS_LEAVE_GAME: {
		if(!duel_mode)
			break;
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
