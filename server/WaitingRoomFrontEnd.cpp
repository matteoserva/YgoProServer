#include "WaitingRoom.h"
#include "RoomManager.h"
#include "DuelRoom.h"
#include <time.h>
#include "GameServer.h"
namespace ygo
{

void WaitingRoom::ReadyFlagPressed(DuelPlayer* dp,bool readyFlag)
{
        playerReadinessChange(dp,readyFlag);
}

void WaitingRoom::ShowDuelSettings(DuelPlayer* dp)
{
    bool isReady =players[dp].isReady;

    SendNameToPlayer(dp,0,"-Duel Settings-");

    if(player_status[dp].spamScelto == 0)
        SendNameToPlayer(dp,1,"[CheckMate Server!]");
    else
        SendNameToPlayer(dp,1,"www.ygopro.it");

    if(player_status[dp].banlistCompatibili == 3)
    {
        if(dp->lflist ==2 )
             SendNameToPlayer(dp,3,"Banlist: [TCG] [  ]");
        else
            SendNameToPlayer(dp,3,"Banlist: [  ] [OCG]");
    }
    else
    {
        if(dp->lflist ==2 )
            SendNameToPlayer(dp,3,"Banlist: [TCG]");
        else if(dp->lflist == 1)
            SendNameToPlayer(dp,3,"Banlist: [OCG]");
        else
            SendNameToPlayer(dp,3,"Banlist: [   ]");
    }

    if(player_status[dp].modeScelto == MODE_SINGLE )
        SendNameToPlayer(dp,2,"mode: [SINGLE]");
    else if(player_status[dp].modeScelto == MODE_TAG )
        SendNameToPlayer(dp,2,"mode: [TAG]");
    else if(player_status[dp].modeScelto == MODE_MATCH )
        SendNameToPlayer(dp,2,"mode: [MATCH]");
    else if(player_status[dp].modeScelto == MODE_HANDICAP)
        SendNameToPlayer(dp,2,"mode: [HANDICAP]");


    STOC_TypeChange sctc;
    sctc.type =  0x10 | NETPLAYER_TYPE_PLAYER1;
    SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);
    //playerReadinessChange(dp,false);
    STOC_HS_PlayerChange scpc;
    for(int i=1; i<4; i++)
    {
        scpc.status = (i << 4) | PLAYERCHANGE_READY;
        SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
    }



    changePlayerStatus(dp,DuelPlayerStatus::DUELSETTINGS);


}


void WaitingRoom::ShowStats(DuelPlayer* dp)
{
    char name[20],message[256];
    wchar_t wmessage[21];
    BufferIO::CopyWStr(dp->name,name,20);
    SendNameToPlayer(dp,0,name);
    std::string username(name);
    int rank = Users::getInstance()->getRank(username);
    int score = dp->cachedGameScore;

    switch (dp->loginStatus)
    {
    case Users::LoginResult::AUTHENTICATED:
        swprintf(wmessage,20, L"Score: %d, rank:?",score);
        SendNameToPlayer(dp,2,wmessage);
        sprintf(message, "www.ygopro.it");
        //sprintf(message, "Score: %d(%+d)",score,dp->cachedGameScore-score);
        SendNameToPlayer(dp,3,message);
        break;

    case Users::LoginResult::INVALIDPASSWORD:
        SendNameToPlayer(dp,2,"Unregistered user");
        SendNameToPlayer(dp,3,"invalid password");
        break;

    case Users::LoginResult::INVALIDUSERNAME:
        SendNameToPlayer(dp,2,"Unregistered user");
        SendNameToPlayer(dp,3,"invalid username");
        break;

    case Users::LoginResult::NOPASSWORD:
        sprintf(message, "Rank:  %d",rank);
        SendNameToPlayer(dp,2,message);
        SendNameToPlayer(dp,3,"You need a password");
        break;

    case Users::LoginResult::UNRANKED:
        SendNameToPlayer(dp,2,"Unranked player!");
        SendNameToPlayer(dp,3,":-)");
        break;
    }

    SendNameToPlayer(dp,1,banner.c_str());

    STOC_TypeChange sctc;
    sctc.type =  NETPLAYER_TYPE_PLAYER1;
    SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);

    STOC_HS_PlayerChange scpc;
    scpc.status = (dp->type << 4) | (players[dp].isReady?PLAYERCHANGE_READY:PLAYERCHANGE_NOTREADY);
    SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
    for(int i=1; i<4; i++)
    {
        scpc.status = (i << 4) | PLAYERCHANGE_NOTREADY;
        SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
    }


    changePlayerStatus(dp,DuelPlayerStatus::STATS);
    //ChatWithPlayer(dp, "CheckMate",L"我正在工作为了在ygopro上你们也可以用中文");
}


void WaitingRoom::EnableCrosses(DuelPlayer* dp)
{
    STOC_TypeChange sctc;
    sctc.type =  0x10 | NETPLAYER_TYPE_PLAYER1;
    SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);
    playerReadinessChange(dp,false);

    STOC_HS_PlayerChange scpc;
    for(int i=0; i<4; i++)
    {
        scpc.status = (i << 4) | PLAYERCHANGE_NOTREADY;
        SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
    }
}

void WaitingRoom::ToObserverPressed(DuelPlayer* dp)
{
    std::vector<DuelRoom *> lista = roomManager->getCompatibleRoomsList(dp);
    EnableCrosses(dp);
    SendNameToPlayer(dp,0,"-Available rooms-");
    for(int i = 3; i>lista.size(); i--)
        SendNameToPlayer(dp,i,"");
    for(int i=0; i<lista.size() && i<3; i++)
    {
        char testo[20];
        char tipo[10];
        if(lista[i]->mode == MODE_MATCH)
            sprintf(tipo,"%s","[MATCH]");
        else if(lista[i]->mode == MODE_TAG)
            sprintf(tipo,"%s","[TAG]");
		else if(lista[i]->mode == MODE_HANDICAP)
            sprintf(tipo,"%s","[TAG]");
        else
            sprintf(tipo,"%s","[SINGLE]");

        sprintf(testo,"%s %d ",tipo,lista[i]->getFirstPlayer()->cachedRankScore);

        char nome[20];
        BufferIO::CopyWStr(lista[i]->getFirstPlayer()->name,nome,20);
        strncat(testo,nome,20-strlen(testo));

        SendNameToPlayer(dp,i+1,testo);
        log(VERBOSE,"waitingroom, trovato server\n");
    }
    changePlayerStatus(dp,DuelPlayerStatus::CHOOSESERVER);
    player_status[dp].listaStanzeCompatibili = lista;

    if(lista.size() == 0)
        SendNameToPlayer(dp,2,"NO ROOMS AVAILABLE");
}

void WaitingRoom::ShowCustomMode(DuelPlayer* dp)
{
    SendNameToPlayer(dp,0,"-Custom duel mode-");
    SendNameToPlayer(dp,1,"[HANDICAP MATCH]");
    SendNameToPlayer(dp,2,"");
    SendNameToPlayer(dp,3,"");
    changePlayerStatus(dp,DuelPlayerStatus::CUSTOMMODE);
}
void WaitingRoom::ToDuelistPressed(DuelPlayer* dp)
{
            ShowDuelSettings(dp);
return;
    if(player_status[dp].status==DuelPlayerStatus::CHOOSEGAMETYPE)
    {
        ShowCustomMode(dp);
        return;
    }
    if(player_status[dp].status==DuelPlayerStatus::CUSTOMMODE)
    {
        ShowStats(dp);
        return;
    }
    SendNameToPlayer(dp,0,L"-duel mode-");
    SendNameToPlayer(dp,1,"[SINGLE]");
    SendNameToPlayer(dp,2,"[MATCH]");
    SendNameToPlayer(dp,3,"[TAG]");
    EnableCrosses(dp);
    changePlayerStatus(dp,DuelPlayerStatus::CHOOSEGAMETYPE);
}
void WaitingRoom::changePlayerStatus(DuelPlayer* dp,DuelPlayerStatus::Status newstatus)
{
    if(player_status[dp].status==DuelPlayerStatus::CHALLENGERECEIVED)
        refuse_challenge(dp);

    if(newstatus != DuelPlayerStatus::DUELSETTINGS)
        if(player_status[dp].status!=newstatus && players[dp].isReady == true)
        {
            playerReadinessChange(dp,false);

        }


            STOC_HS_PlayerChange scpc;
            scpc.status = (players[dp].isReady)?PLAYERCHANGE_READY:PLAYERCHANGE_NOTREADY;
            SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);

    player_status[dp].status=newstatus;


}
void WaitingRoom::player_erase_cb(DuelPlayer* dp)
{
    refuse_challenge(dp);

}

void WaitingRoom::ButtonStartPressed(DuelPlayer* dp)
    {
        if(player_status[dp].status == DuelPlayerStatus::DUELSETTINGS)
            if(players[dp].isReady)
            {
                unsigned char mode = player_status[dp].modeScelto;
                ExtractPlayer(dp);
                roomManager->InsertPlayer(dp,mode);
            }




    }

void WaitingRoom::ButtonKickPressed(DuelPlayer* dp,int pos)
{
    if(pos == 0)
    {
        ShowStats(dp);
        return;
    }
    /*if(dp->lflist<=0 && DuelPlayerStatus::CHOOSEBANLIST != player_status[dp].status && DuelPlayerStatus::CHALLENGERECEIVED != player_status[dp].status)
    {
        SendNameToPlayer(dp,0,L">=check the box ==>");

        return;
    }*/

    switch(player_status[dp].status)
    {
        case DuelPlayerStatus::CUSTOMMODE:
        ExtractPlayer(dp);
        if(pos == 1)
            //if(wcscmp(dp->namew_low,L"checkmate")==0)
            roomManager->InsertPlayer(dp,MODE_HANDICAP);

        break;

    case DuelPlayerStatus::CHOOSEBANLIST:
        dp->lflist=pos;
        ShowChooseBanlist(dp,dp->lflist,false);
        ReadyFlagPressed(dp,true);
        STOC_HS_PlayerChange scpc;
        scpc.status = 0 | PLAYERCHANGE_READY;
        SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
        break;

    case DuelPlayerStatus::DUELSETTINGS:
        if(pos == 1)
        {
            player_status[dp].spamScelto = (player_status[dp].spamScelto==0)?1:0;
        }
        if(pos == 2)
        {
            if(player_status[dp].modeScelto == MODE_SINGLE)
                player_status[dp].modeScelto = MODE_TAG;
            else if(player_status[dp].modeScelto == MODE_TAG)
                player_status[dp].modeScelto = MODE_MATCH;
            else if(player_status[dp].modeScelto == MODE_MATCH)
                player_status[dp].modeScelto = MODE_HANDICAP;
            else if(player_status[dp].modeScelto == MODE_HANDICAP)
                player_status[dp].modeScelto = MODE_SINGLE;
        }
        if(pos == 3)
        {
            if(dp->lflist == 1 && player_status[dp].banlistCompatibili==3)
                dp->lflist =2;
            else if(dp->lflist == 2 && player_status[dp].banlistCompatibili==3)
                dp->lflist =1;

        }
        ShowDuelSettings(dp);
        break;


    case DuelPlayerStatus::CHOOSEGAMETYPE:
        ExtractPlayer(dp);
        if(pos == 1)
            roomManager->InsertPlayer(dp,MODE_SINGLE);
        else if(pos==2)
            roomManager->InsertPlayer(dp,MODE_MATCH);
        else if(pos==3)
            roomManager->InsertPlayer(dp,MODE_TAG);
        break;


    case DuelPlayerStatus::CHOOSESERVER:
        if(pos > player_status[dp].listaStanzeCompatibili.size() )
            break;
        roomManager->tryToInsertPlayerInServer(dp,player_status[dp].listaStanzeCompatibili[pos-1]);
        break;

     case DuelPlayerStatus::CHALLENGERECEIVED:
        if(pos ==2 && player_status.find(player_status[dp].challenger) != player_status.end() && player_status[player_status[dp].challenger].challenger== dp)
        {
            DuelPlayer* dpnemico = player_status[dp].challenger;
            DuelRoom* netserver = roomManager->createServer(player_status[dp].challenge_mode);
            player_status[dp].challenger = nullptr;
            player_status[dpnemico].challenger = nullptr;

            roomManager->tryToInsertPlayerInServer(dpnemico,netserver);
            roomManager->tryToInsertPlayerInServer(dp,netserver);
        }
        else
        {
            refuse_challenge(dp);
            ShowStats(dp);
        }


        break;
    }

}

bool WaitingRoom::send_challenge_request(DuelPlayer* dp,wchar_t * second,unsigned char mode)
{
        std::wstring nomenemico(second);

        DuelPlayer* dpnemico = findPlayerByName(nomenemico);
        if(dpnemico == nullptr)
        {
            SystemChatToPlayer(dp,L"Username not found.",true);
            return true;

        }

        if(dp == dpnemico)
        {
            SystemChatToPlayer(dp,L"You cannot challenge yourself",true);
            return true;

        }

        if(player_status[dp].challenger)
        {
            SystemChatToPlayer(dp,L"You have already sent a challenge request",true);
            return true;
        }
        if(player_status[dpnemico].challenger)
        {
            SystemChatToPlayer(dp,L"This player is not ready to receive a challenge request",true);
            return true;
        }
        if(wcscmp(dp->namew_low,L"checkmate"))
        if(abs(dpnemico->cachedRankScore - dp->cachedRankScore) > roomManager->maxScoreDifference(dp->cachedRankScore))
            SystemChatToPlayer(dp,L"The difference between your scores is too high,true",true);

        wchar_t dpname[20];
        BufferIO::CopyWStr(dp->name,dpname,20);
        ShowChallengeReceived(dpnemico,dpname);
        player_status[dp].challenger = dpnemico;
                player_status[dpnemico].challenger = dp;
                player_status[dpnemico].challenge_mode = mode;

        SystemChatToPlayer(dp,L"Challenge request sent!",true);
        return true;



}

void WaitingRoom::refuse_challenge(DuelPlayer* dp)
{
    DuelPlayer* opponent = player_status[dp].challenger;
    if(opponent == nullptr)
        return;

    if(player_status[dp].status!=DuelPlayerStatus::CHALLENGERECEIVED)
            SystemChatToPlayer(dp,L"the player refused your challenge request",true);

    player_status[dp].challenger = nullptr;
    if(player_status.find(opponent) == player_status.end())
        return;
    dp = opponent;
        if(player_status[dp].status!=DuelPlayerStatus::CHALLENGERECEIVED)
            SystemChatToPlayer(dp,L"the player refused your challenge request",true);
    player_status[dp].challenger = nullptr;



}

void WaitingRoom::ShowChooseBanlist(DuelPlayer* dp,int selected,bool showCrosses)
{
    if(showCrosses)
        SendNameToPlayer(dp,0,L"Select banlist?");
    else
        SendNameToPlayer(dp,0,L"Banlist chosen:");
    if(selected == 1)
        SendNameToPlayer(dp,1,"[OCG] *");
    else
        SendNameToPlayer(dp,1,"[OCG]");
    if(selected == 2)
        SendNameToPlayer(dp,2,"[TCG] *");
    else
        SendNameToPlayer(dp,2,"[TCG]");
    if(selected == 3)
        SendNameToPlayer(dp,3,"[RANDOM] *");
    else
        SendNameToPlayer(dp,3,"[RANDOM]");
    if(showCrosses)
        EnableCrosses(dp);
    else
    {
        STOC_TypeChange sctc;
        sctc.type =  0x00 | NETPLAYER_TYPE_PLAYER1;
        SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);
    }


    changePlayerStatus(dp,DuelPlayerStatus::CHOOSEBANLIST);
    STOC_HS_PlayerChange scpc;

        scpc.status = (0 << 4) | PLAYERCHANGE_READY;
        SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);


}

void WaitingRoom::ShowChallengeReceived(DuelPlayer* dp,wchar_t * opponent)
{
    SystemChatToPlayer(dp,L"A player challenges you!",true);
    SendNameToPlayer(dp,0,L"challenge received");
    SendNameToPlayer(dp,1,opponent);
    SendNameToPlayer(dp,2,"[ACCEPT]");
    SendNameToPlayer(dp,3,"[DECLINE]");
    EnableCrosses(dp);

    changePlayerStatus(dp,DuelPlayerStatus::CHALLENGERECEIVED);
}

bool WaitingRoom::handleChatCommand(DuelPlayer* dp,wchar_t* messaggio)
{
    if(RoomInterface::handleChatCommand(dp,messaggio))
    {
		//comando riuscito
	}
	else if(!wcscmp(messaggio,L"!tag") || !wcscmp(messaggio,L"!t"))
    {
        SystemChatToPlayer(dp,L"http://ygopro.it/help.php",true);
        playerReadinessChange(dp,false);
        ShowStats(dp);
        //ExtractPlayer(dp);
        //roomManager->InsertPlayer(dp,MODE_TAG);

    }/*
	else if(!wcsncmp(messaggio,L"!join ",6) )
    {
        

        if(wcslen(messaggio) < 7)
        {
            SystemChatToPlayer(dp,L"I need a username",true);
            return true;
        }
        
        std::wstring nome(&messaggio[6]);
        

        DuelPlayer* nemico = gameServer->findPlayer(nome);

        if(nemico==nullptr)
		{
            SystemChatToPlayer(dp,L"Player not found, or player with a different server PID",true);
			return true;
		}
			
			
		DuelRoom * room = dynamic_cast<DuelRoom*> nemico->netServer;
		if(room==nullptr)
		{
			SystemChatToPlayer(dp,L"The Player is not in a duel room",true);
			return true;
			
		}
		ExtractPlayer(dp);
		room->InsertPlayer(dp);
		//roomManager->tryToInsertPlayerInServer(dp,(DuelRoom *)nemico->netServer);
		
        return true;
    }*/
    else if(!wcscmp(messaggio,L"!single") || !wcscmp(messaggio,L"!s") || !wcscmp(messaggio,L"single!"))
    {
        SystemChatToPlayer(dp,L"http://ygopro.it/help.php",true);
        playerReadinessChange(dp,false);
        ShowStats(dp);
        //ExtractPlayer(dp);
        //roomManager->InsertPlayer(dp,MODE_SINGLE);
    }
    else if(!wcscmp(messaggio,L"!match") || !wcscmp(messaggio,L"!m"))
    {
        SystemChatToPlayer(dp,L"http://ygopro.it/help.php",true);
        playerReadinessChange(dp,false);
        ShowStats(dp);
        //ExtractPlayer(dp);
        //roomManager->InsertPlayer(dp,MODE_MATCH);
    }
    else if(!wcscmp(messaggio,L"!help") || !wcscmp(messaggio,L"!h"))
    {
        SystemChatToPlayer(dp,L"http://ygopro.it/help.php",true);
        playerReadinessChange(dp,false);
        ShowStats(dp);
        //ExtractPlayer(dp);
        //roomManager->InsertPlayer(dp,MODE_MATCH);
    }
    else if( !wcsncmp(messaggio,L"!challenge ",11) )
    {
        if(wcslen(messaggio) < 13)
        {
            SystemChatToPlayer(dp,L"I need a username",true);
            return true;
        }

        wchar_t word1[25];
        wchar_t word2[25];
        word2[0] = word1[0] = 0;
        swscanf(&messaggio[11], L"%24ls %24ls",word1,word2);

        printf("mode %ls name %ls\n",word1,word2);
        if(wcslen(word2) == 0)
        {
                wchar_t *second = &messaggio[11];
                send_challenge_request(dp,word1,MODE_SINGLE);

        }
        else if( !wcsncmp(word1,L"SINGLE",6))
        {
                wchar_t *second = &messaggio[11];
                send_challenge_request(dp,word2,MODE_SINGLE);
        }
        else if( !wcsncmp(word1,L"TAG",3))
        {
                wchar_t *second = &messaggio[11];
                send_challenge_request(dp,word2,MODE_TAG);
        }
        else if( !wcsncmp(word1,L"MATCH",5))
        {
                wchar_t *second = &messaggio[11];
                send_challenge_request(dp,word2,MODE_MATCH);
        }
        else if( !wcsncmp(word1,L"HANDICAP",8))
        {
                wchar_t *second = &messaggio[11];
                send_challenge_request(dp,word2,MODE_HANDICAP);
        }
        else
        {
                SystemChatToPlayer(dp,L"Error, invalid duel mode. example:  !challenge SINGLE username",true);
        }
        return true;
    }
    else
    {
        return false;
    }
    return true;

}

}
