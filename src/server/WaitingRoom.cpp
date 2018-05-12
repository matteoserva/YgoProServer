#include "WaitingRoom.h"
#include "GameServer.h"
#include "common/debug.h"
//#include "Users.h"
#include "common/Config.h"
//#include "Statistics.h"

extern const unsigned int BUILD_NUMBER;
namespace ygo
{

int WaitingRoom::minSecondsWaiting;
int WaitingRoom::maxSecondsWaiting;
const std::string WaitingRoom::banner = "[Checkmate Server!]";

WaitingRoom::WaitingRoom(RoomManager*roomManager,GameServer*gameServer):
    RoomInterface(roomManager,gameServer),cicle_users(0)
{
    WaitingRoom::minSecondsWaiting=Config::getInstance()->waitingroom_min_waiting;
    WaitingRoom::maxSecondsWaiting=Config::getInstance()->waitingroom_max_waiting;
	
	queueManager.setMaxAge(WaitingRoom::maxSecondsWaiting);
	queueManager.createQueue(QUEUE_SINGLE_UNRANKED_TCG,2);
	queueManager.createQueue(QUEUE_SINGLE_UNRANKED_OCG,2);
	queueManager.createQueue(QUEUE_SINGLE_RANKED_OCG,2);
	queueManager.createQueue(QUEUE_SINGLE_RANKED_TCG,2);
	queueManager.createQueue(QUEUE_MATCH_RANKED_TCG,2);
	queueManager.createQueue(QUEUE_MATCH_RANKED_OCG,2);
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
        if(it->second.isReady)
            it->second.secondsWaiting+= 0.1;

    }




}

void WaitingRoom::cicle_users_cb(evutil_socket_t fd, short events, void* arg)
{
    WaitingRoom*that = (WaitingRoom*)arg;

    if(!that->players.size())
        return;

	auto bestRows = that->queueManager.extractBest();
	for(int i = 0; i < bestRows.size(); i++)
	{
		QueueElement & elem = bestRows[i];
		DuelRoom * room = nullptr;
		switch(elem.queue)
		{
			case QUEUE_SINGLE_UNRANKED_TCG:
			case QUEUE_SINGLE_UNRANKED_OCG:
			case QUEUE_SINGLE_RANKED_OCG:
			case QUEUE_SINGLE_RANKED_TCG:
				room = that->roomManager->createServer(MODE_SINGLE);
				break;
			case QUEUE_MATCH_RANKED_TCG:
			case QUEUE_MATCH_RANKED_OCG:
				room = that->roomManager->createServer(MODE_MATCH);
				break;
		}
		
		printf("estraggo: ");
		for(auto it=elem.elements.begin(); it!= elem.elements.end(); ++it)
		{
			DuelPlayer * dp = (DuelPlayer * ) *it;
			std::vector<char> deck = that->player_status[dp].lastDeck;
			if(that->players.find(dp) == that->players.end())
				printf("stavo estraendo un player inesistente\n");
			
			char opname[20];
            BufferIO::CopyWStr(dp->name,opname,20);
			printf("(%s %u) ",opname,dp->cachedScore);
			that->ExtractPlayer(dp);
			room->InsertPlayer(dp);
			if(deck.size() > 0)
			{
			room->HandleCTOSPacket(dp,&deck[0],deck.size());
			char ready = CTOS_HS_READY;
			room->HandleCTOSPacket(dp,&ready,1);
			}
			
		}
		printf("\n");
		
	}


    std::list<DuelPlayer*> players_bored;
    
    for(auto it=that->players.begin(); it!=that->players.end(); ++it)
    {
        char nome[30];
        BufferIO::CopyWStr(it->first->name,nome,30);
        if( !that->ReadyToDuel(it->first))
            continue;
        if(that->player_status[it->first].modeScelto == MODE_SINGLE || that->player_status[it->first].modeScelto == MODE_MATCH)
			continue;
        if(it->second.secondsWaiting>= minSecondsWaiting)
            players_bored.push_back(it->first);
    }

   
        for (auto it = players_bored.cbegin(); it != players_bored.cend(); ++it)
        {
            if(that->players.find(*it) != that->players.end())
            {
                unsigned char mode = that->player_status[*it].modeScelto;
				std::vector<char> deck = that->player_status[*it].lastDeck;
                that->ExtractPlayer(*it);
                DuelRoom *result = that->roomManager->InsertPlayer(*it,mode);
				
				if(result && result->state == DuelRoom::WAITING)
				{
					STOC_HS_PlayerChange scpc;
					scpc.status = PLAYERCHANGE_READY | ((((*it)->type) & 0xf) <<4);
					result->SendPacketToPlayer(*it, STOC_HS_PLAYER_CHANGE, scpc);
					
					printf("result->state %d\n",result->state);
					if(deck.size() > 0)
					{
						result->HandleCTOSPacket(*it,&deck[0],deck.size());
						char ready = CTOS_HS_READY;
						result->HandleCTOSPacket(*it,&ready,1);
					
					}
				}
				
				
            }
        }
}

void WaitingRoom::ExtractPlayer(DuelPlayer* dp)
{
	queueManager.removeElement(dp);
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
    info.lflist=0;
    info.time_limit=0;
    info.start_lp=0;
    info.enable_priority=false;
    info.no_shuffle_deck=false;

    dp->type = NETPLAYER_TYPE_PLAYER1;

    STOC_JoinGame scjg;
    scjg.info=info;
    SendPacketToPlayer(dp, STOC_JOIN_GAME, scjg);

    STOC_TypeChange sctc;
    sctc.type = dp->type ;
    SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);


    updateObserversNum();

    ShowDuelSettings(dp);

    char buffer[200];

    sprintf(buffer,"PID: %d, BUILD: %d, PLAYERS: %u",(int)getpid(),(int)BUILD_NUMBER,roomManager->getNumPlayers());
    RemoteChatToPlayer(dp,std::wstring(buffer, buffer + strlen(buffer)),-1);
	//ChatWithPlayer(dp, "CheckMate",buffer);

     ChatWithPlayer(dp, "CheckMate","Welcome to the CheckMate server!");
    //ChatWithPlayer(dp, "CheckMate","Type !tag to enter a tag duel, !single for a single duel or !match");

    if(dp->loginStatus != ygo::LoginResult::AUTHENTICATED && dp->loginStatus != ygo::LoginResult::UNRANKED)
    {
        ChatWithPlayer(dp, "CheckMate","to register and login, go back and change the username to yourusername$yourpassword");
    }

    if(dp->loginStatus != ygo::LoginResult::AUTHENTICATED && dp->loginStatus != ygo::LoginResult::NOPASSWORD)
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

bool WaitingRoom::ReadyToDuel(DuelPlayer* dp)
{

    return players[dp].isReady;// && (player_status[dp].status==DuelPlayerStatus::STATS || player_status[dp].status==DuelPlayerStatus::CHOOSEBANLIST);
}

DuelPlayer* WaitingRoom::ExtractBestMatchPlayer(DuelPlayer* referencePlayer,int lflist,unsigned char mode)
{
    int referenceScore = referencePlayer->cachedScore;
       if(!players.size())
        return nullptr;

    int qdifference = 0;
    DuelPlayer *chosenOne = nullptr;
    for(auto it=players.cbegin(); it!=players.cend(); ++it)
    {
        if(it->second.secondsWaiting >= minSecondsWaiting)
        {

            DuelPlayer* dp = it->first;
            if(!ReadyToDuel(dp))
                continue;

            if(!(lflist ==3 || dp->lflist == 3 || dp->lflist == lflist))
             {
                 continue;
             }


            if(player_status[dp].modeScelto != mode)
            {

                continue;
            }


            char opname[20];
            BufferIO::CopyWStr(dp->name,opname,20);
            int opscore = dp->cachedScore;

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
        log(VERBOSE,"qdifference = %d\n",qdifference);
    }
    return chosenOne;
}
void WaitingRoom::RoomChat(DuelPlayer* dp, std::wstring messaggio)
{
    char sender[20];
	
    BufferIO::CopyWStr(dp->name,sender,20);
	for(auto it=players.begin(); it!=players.end(); ++it)
    {
        if(it->first== dp)
            continue;
        ChatWithPlayer(it->first, std::string(sender) + "<"+dp->countryCode+">",messaggio);
    }

}


void WaitingRoom::HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)
{
    char*pdata=data;
    unsigned char pktType = BufferIO::ReadUInt8(pdata);

    switch(pktType)
    {
    case CTOS_UPDATE_DECK:
    {
			
			
			
			std::vector<char> & lastDeck = player_status[dp].lastDeck;
			lastDeck.clear();
			lastDeck.insert(lastDeck.end(),data,data+len);
			
			
			
            break;
    }
    case CTOS_PLAYER_INFO:
    {

        //log(INFO,"WaitingRoom:Player joined %s \n",name);
        break;
    }
    case CTOS_CHAT:
    {
        //ChatMessageReceived(dp,(unsigned short*) pdata);
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
	
	{
		    if(player_status[dp].lastDeck.size() <= 0)
				break;
			
		    char *deckData = &player_status[dp].lastDeck[0];
			BufferIO::ReadUInt8(deckData);
            
			
			int comp = detectDeckCompatibleLflist(deckData);
			
			
		    player_status[dp].banlistCompatibili = 0;
			if(comp >= 0)
			{
				player_status[dp].banlistCompatibili = comp;
			}
			else
			{
				STOC_ErrorMsg scem;
				scem.msg = ERRMSG_DECKERROR;
				scem.code = -comp;
				SendPacketToPlayer(dp, STOC_ERROR_MSG, scem);
				STOC_HS_PlayerChange scpc;
				scpc.status = PLAYERCHANGE_NOTREADY;
				scpc.status |= (NETPLAYER_TYPE_PLAYER1 << 4);
				SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
				break;
			}
            
			
			
			if(player_status[dp].banlistCompatibili == 3 && dp->lflist == 3)
                dp->lflist = 1;
            else if (player_status[dp].banlistCompatibili != 3)
                dp->lflist =player_status[dp].banlistCompatibili;

			switch(player_status[dp].banlistCompatibili)
			{
				case 1:
				SystemChatToPlayer(dp,L"Your deck is compatible with the OCG banlist only.",true); break;
				case 2:
				SystemChatToPlayer(dp,L"Your deck is compatible with the TCG banlist only.",true); break;
				case 3:
				SystemChatToPlayer(dp,L"Your deck is compatible with both banlists.",true); break;
				default:
				SystemChatToPlayer(dp,L"Your deck is not compatible with any banlist.",true); break;
			}

            bool ranked = (dp->loginStatus == ygo::LoginResult::NOPASSWORD || dp->loginStatus == ygo::LoginResult::AUTHENTICATED);
		
		
			if(!ranked && player_status[dp].modeScelto == MODE_MATCH)
			{
				STOC_HS_PlayerChange scpc;
				scpc.status = PLAYERCHANGE_NOTREADY;
				scpc.status |= (NETPLAYER_TYPE_PLAYER1 << 4);
				SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
				SystemChatToPlayer(dp,L"Match duel is available only to ranked users.",true);
				break;
			}
		
		    ReadyFlagPressed(dp,true);
			ShowDuelSettings(dp);
			if(player_status[dp].modeScelto == MODE_SINGLE)
			{
				char opname[20];
				BufferIO::CopyWStr(dp->name,opname,20);
				if( (opname[0] == '-') == ranked )
					printf("\n\nbug ranked, %s %u\n\n\n",opname,dp->loginStatus);
				if((dp->lflist & 1 ) && !ranked)
					queueManager.insertElement(dp,0,QUEUE_SINGLE_UNRANKED_OCG);
				if((dp->lflist & 2 ) && !ranked)
					queueManager.insertElement(dp,0,QUEUE_SINGLE_UNRANKED_TCG);
				if((dp->lflist & 1 ) && ranked)
					queueManager.insertElement(dp,dp->cachedScore,QUEUE_SINGLE_RANKED_OCG);
				if((dp->lflist & 2 ) && ranked)
					queueManager.insertElement(dp,dp->cachedScore,QUEUE_SINGLE_RANKED_TCG);
			}
			if(player_status[dp].modeScelto == MODE_MATCH)
			{
				if(dp->lflist & 1 )
					queueManager.insertElement(dp,0,QUEUE_MATCH_RANKED_OCG);
				if(dp->lflist & 2 )
					queueManager.insertElement(dp,0,QUEUE_MATCH_RANKED_TCG);
			}
		
		break;
    }
	case CTOS_HS_NOTREADY:
	{
		 ReadyFlagPressed(dp,false);
			queueManager.removeElement(dp);
			
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
		
			//ButtonStartPressed(dp);
        break;
    }

    }
}

}
