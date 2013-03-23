#include "WaitingRoom.h"
#include "RoomManager.h"
#include "netserver.h"
namespace ygo
{

void WaitingRoom::ReadyFlagPressed(DuelPlayer* dp,bool readyFlag)
{
    if(player_status[dp].status!=DuelPlayerStatus::STATS)
    {
        STOC_HS_PlayerChange scpc;
        scpc.status = (NETPLAYER_TYPE_PLAYER1 << 4) | PLAYERCHANGE_READY;
        SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
    }
    else
        playerReadinessChange(dp,readyFlag);

}

void WaitingRoom::ShowStats(DuelPlayer* dp)
{
    char name[20],message[256];
    BufferIO::CopyWStr(dp->name,name,20);
    SendNameToPlayer(dp,0,name);
    std::string username(name);
    int rank = Users::getInstance()->getRank(username);
    int score = dp->cachedRankScore;

    switch (dp->loginStatus)
    {
    case Users::LoginResult::AUTHENTICATED:
        sprintf(message, "Rank:  %d",rank);
        SendNameToPlayer(dp,2,message);
        sprintf(message, "Score: %d",score);
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


    player_status[dp].status = DuelPlayerStatus::STATS;
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
        scpc.status = (i << 4) | PLAYERCHANGE_READY;
        SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
    }
}

void WaitingRoom::ToObserverPressed(DuelPlayer* dp)
{
    std::vector<CMNetServer *> lista = roomManager->getCompatibleRoomsList(dp->cachedRankScore);
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
        else
            sprintf(tipo,"%s","[SINGLE]");

        sprintf(testo,"%s %d ",tipo,lista[i]->getFirstPlayer()->cachedRankScore);

        char nome[20];
        BufferIO::CopyWStr(lista[i]->getFirstPlayer()->name,nome,20);
        strncat(testo,nome,20-strlen(testo));

        SendNameToPlayer(dp,i+1,testo);
        log(VERBOSE,"waitingroom, trovato server\n");
    }
    player_status[dp].status=DuelPlayerStatus::CHOOSESERVER;
    player_status[dp].listaStanzeCompatibili = lista;

    if(lista.size() == 0)
        SendNameToPlayer(dp,2,"NO ROOMS AVAILABLE");
}

void WaitingRoom::ShowCustomMode(DuelPlayer* dp)
{
    SendNameToPlayer(dp,0,"-Custom duel mode-");
    SendNameToPlayer(dp,1,"");
    SendNameToPlayer(dp,2,"not working yet");
    SendNameToPlayer(dp,3,"");
    player_status[dp].status=DuelPlayerStatus::CUSTOMMODE;
}
void WaitingRoom::ToDuelistPressed(DuelPlayer* dp)
{
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
    player_status[dp].status=DuelPlayerStatus::CHOOSEGAMETYPE;
}

void WaitingRoom::ButtonKickPressed(DuelPlayer* dp,int pos)
{
    if(pos == 0)
    {
        ShowStats(dp);
        return;
    }

    switch(player_status[dp].status)
    {
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
    }

}

bool WaitingRoom::handleChatCommand(DuelPlayer* dp,char* msg)
{
    char messaggio[256];
    int msglen = BufferIO::CopyWStr(msg, messaggio, 256);
    log(INFO,"ricevuto messaggio %s\n",messaggio);

    if(!strcmp(messaggio,"!tag") || !strcmp(messaggio,"!t"))
    {
        ExtractPlayer(dp);
        roomManager->InsertPlayer(dp,MODE_TAG);

    }
    else if(!strcmp(messaggio,"!single") || !strcmp(messaggio,"!s"))
    {
        ExtractPlayer(dp);
        roomManager->InsertPlayer(dp,MODE_SINGLE);
    }
    else if(!strcmp(messaggio,"!match") || !strcmp(messaggio,"!m"))
    {
        ExtractPlayer(dp);
        roomManager->InsertPlayer(dp,MODE_MATCH);
    }
    else if(!strncmp(messaggio,"!shout ",7) )
    {
        char*msg2 = &messaggio[7];
        std::string tmp(msg2);
        std::wstring tmp2(tmp.begin(),tmp.end());
        roomManager->BroadcastMessage(tmp2,true);
    }
    else
    {
        return false;
    }
    return true;

}

}
