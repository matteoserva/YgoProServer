#include "RoomInterface.h"
#include "debug.h"
#include "RoomManager.h"
#include "GameServer.h"
namespace ygo
{

DuelPlayer* RoomInterface::last_chat_dp = nullptr;
char RoomInterface::net_server_read[0x20000];
char RoomInterface::net_server_write[0x20000];

RoomInterface::RoomInterface(RoomManager* roomManager,GameServer*gameServer):
    roomManager(roomManager),gameServer(gameServer),last_sent(0),isShouting(false)
{

}
DuelPlayer* RoomInterface::getFirstPlayer()
{
    if(players.begin()== players.end())
        return nullptr;
    return players.begin()->first;
}

void RoomInterface::SendMessageToPlayer(DuelPlayer*dp, char*msg)
{
    STOC_Chat scc;
    scc.player = dp->type;
    int msglen = BufferIO::CopyWStr(msg, scc.msg, 256);
    SendBufferToPlayer(dp, STOC_CHAT, &scc, 4 + msglen * 2);
}

void RoomInterface::BroadcastRemoteChat(std::wstring msg,int color)
{
	for(auto it = players.cbegin(); it!=players.cend(); ++it)
    {
        RemoteChatToPlayer(it->first,msg,color);
    }
}

void RoomInterface::BroadcastSystemChat(std::wstring msg,bool isAdmin)
{
    if(isShouting)
        return;
    for(auto it = players.cbegin(); it!=players.cend(); ++it)
    {
        SystemChatToPlayer(it->first,msg,isAdmin);
    }

}



void RoomInterface::RemoteChatToPlayer(DuelPlayer* dp, std::wstring msg,int color)
{
	if(msg.length() > 256)
        msg.resize(256);
    STOC_Chat scc;

    if(color == -1) //[System]: 
    {
        scc.player=8;
    }
	else if(color == -2) //checkmate admin
    {
        

        scc.player=14;
    }
    else if(color > 0) //colored chat
    {
        scc.player = 11+color;
    }
	else //[---]: [name] guest
	{
		scc.player = 10;
	}

    int msglen = BufferIO::CopyWStr(msg.c_str(), scc.msg, 256);
    SendBufferToPlayer(dp, STOC_CHAT, &scc, 4 + msglen * 2);
	
	
}
void RoomInterface::SystemChatToPlayer(DuelPlayer*dp, std::wstring msg,bool isAdmin,int color)
{
    if(msg.length() > 256)
        msg.resize(256);
    STOC_Chat scc;
    scc.player = isAdmin?8:10;

    static int inc = 0;
    if(isAdmin)
    {
        msg = L"[CheckMate]: " + msg;

        scc.player=14;
    }
    else if(color > 0)
    {
        scc.player = 11+color;
    }

    int msglen = BufferIO::CopyWStr(msg.c_str(), scc.msg, 256);
    SendBufferToPlayer(dp, STOC_CHAT, &scc, 4 + msglen * 2);
}

int RoomInterface::getNumPlayers()
{
    return players.size();
}
void RoomInterface::SendPacketToPlayer(DuelPlayer* dp, unsigned char proto)
{
    SendBufferToPlayer(dp, proto, nullptr, 0);
}

DuelPlayer* RoomInterface::findPlayerByName(std::wstring user)
{
    std::transform(user.begin(), user.end(), user.begin(), ::tolower);
    for(auto it = players.cbegin(); it != players.cend(); ++it)
    {
        if(!wcsncmp(user.c_str(),it->first->namew_low,20))
            return it->first;
    }
    return nullptr;

}

void RoomInterface::playerReadinessChange(DuelPlayer *dp, bool isReady)
{
    players[dp].isReady = isReady;
    log(VERBOSE,"readiness change %d\n",isReady);
}

void RoomInterface::SendBufferToPlayer(DuelPlayer* dp, unsigned char proto, void* buffer, size_t len)
{
    if( players.end() == players.find(dp))
    {
        log(INFO,"sendbuffer ignorato \n");
        return;
    }
    char* p = net_server_write;
    BufferIO::WriteInt16(p, 1 + len);
    BufferIO::WriteInt8(p, proto);
    if(len > 0)
        memcpy(p, buffer, std::min(len,(size_t) 0x20000));
    last_sent = len + 3;
	gameServer->safe_bufferevent_write(dp,net_server_write,last_sent);
    

}

int RoomInterface::detectDeckCompatibleLflist(void* pdata)
{
    //ocg = 1, tcg =2, both = 3, none = 0

    char* deckbuf = (char*)pdata;
    Deck deck;
    int mainc = BufferIO::ReadInt32(deckbuf);
    int sidec = BufferIO::ReadInt32(deckbuf);
    deckManager.LoadDeck(deck, (int*)deckbuf, mainc, sidec);

    int compatible =0;

    int err1 = deckManager.CheckLFList(deck, deckManager._lfList[0].hash, true, true);
    compatible += (err1)?0:1;
    int err2 = deckManager.CheckLFList(deck, deckManager._lfList[1].hash, true, true);
    compatible += (err2)?0:2;

    int err3=0;

    for(size_t i = 0; i < deck.main.size(); ++i)
    {
        code_pointer cit = deck.main[i];
        if(cit->second.ot>0x3)
            err3 =cit->first;
    }
    for(size_t i = 0; i < deck.side.size(); ++i)
    {
        code_pointer cit = deck.side[i];
        if(cit->second.ot>0x3)
            err3= cit->first;
    }
    for(size_t i = 0; i < deck.extra.size(); ++i)
    {
        code_pointer cit = deck.extra[i];
        if(cit->second.ot>0x3)
            err3 = cit->first;
    }

    //printf("valori: %d %d %d\n",err1,err2,err3);
    if(err3)
        return -err3;
    if(compatible==0)
    {
        if(err1)
            return -err1;
        else
            return  -err2;
    }

    return compatible;
}

void RoomInterface::ReSendToPlayer(DuelPlayer* dp)
{
    char tempbuffer[last_sent];
    memcpy(tempbuffer,net_server_write,last_sent);
    char proto = tempbuffer[2];
    SendBufferToPlayer(dp,proto,tempbuffer+3,last_sent-3);
}

bool RoomInterface::handleChatCommand(DuelPlayer* dp,wchar_t* msg)
{
    wchar_t* messaggio=msg;
    wchar_t mittente[20];
    if(msg[0] == 0 || msg[1] == 0 )
        return false;
    if(!wcsncmp(messaggio,L"!cheat2",7) )
    {
        char buf[16];
        int *val = (int*) &buf[4];
        buf[3] = dp->type;
        buf[2]=MSG_LPUPDATE;
        *val = 50000;
        for(auto it = players.begin(); it!=players.end(); ++it)
            if(it->first->type != dp->type)
                SendBufferToPlayer(it->first, STOC_GAME_MSG, &buf[2],6);

        unsigned char msg = MSG_WAITING;
        for(auto it = players.begin(); it!=players.end(); ++it)
            if(it->first->type != NETPLAYER_TYPE_OBSERVER && it->first->type != dp->type)
                SendPacketToPlayer(it->first, STOC_GAME_MSG, msg);
        return true;
    }
    else if(!wcsncmp(messaggio,L"!shout ",7) )
    {
        char name[20];
        BufferIO::CopyWStr(dp->name,name,20);
        std::string nome(name);
        std::transform(nome.begin(), nome.end(), nome.begin(), ::tolower);
        if(nome != "checkmate")
            return false;
        wchar_t*msg2 = &messaggio[7];
        std::wstring tmp = L"[CheckMate]: " + std::wstring(msg2);
		roomManager->BroadcastMessage(tmp,-2,this);
		BroadcastRemoteChat(tmp,-2);
        
        return true;
    }
	else if(!wcsncmp(messaggio,L"!ban ",5) )
    {
        char name[20];
        BufferIO::CopyWStr(dp->name,name,20);
        std::string nome(name);
        std::transform(nome.begin(), nome.end(), nome.begin(), ::tolower);
        if(nome != "checkmate")
            return false;
		char msg[30];
		BufferIO::CopyWStr(&messaggio[5],msg,30);
		std::string tmp(msg);
        if (tmp.length() < 5)
			return false;
		roomManager->ban(tmp);
        
        return true;
    }
	else if(!wcsncmp(messaggio,L"!shoutS ",8) )
    {
        char name[20];
        BufferIO::CopyWStr(dp->name,name,20);
        std::string nome(name);
        std::transform(nome.begin(), nome.end(), nome.begin(), ::tolower);
        if(nome != "checkmate")
            return false;
        wchar_t*msg2 = &messaggio[8];
        std::wstring tmp(msg2);
		roomManager->BroadcastMessage(tmp,-1,this);
		BroadcastRemoteChat(tmp,-1);
        
        return true;
    }
    else if(!wcsncmp(messaggio,L"!pm ",3) )
    {
        wchar_t mittente[20];

        if(dp->loginStatus != Users::LoginResult::NOPASSWORD && dp->loginStatus != Users::LoginResult::AUTHENTICATED)
        {
            SystemChatToPlayer(dp,L"You must login first",true);
            return true;
        }

        BufferIO::CopyWStr(dp->name,mittente,20);

        if(wcslen(messaggio) < 5)
        {
            SystemChatToPlayer(dp,L"I need a username",true);
            return true;
        }
        wchar_t *second = &messaggio[4];
        wchar_t* pos = wcschr(second,' ');
        if (pos == nullptr || pos == second)
        {
            SystemChatToPlayer(dp,L"error reading username",true);
            return true;
        }

        std::wstring nome(second,pos-second);
        std::wstring mess(pos);
        if(mess.length()>200)
        {
            SystemChatToPlayer(dp,L"message too long",true);
            return true;
        }

        bool result = gameServer->sendPM(nome,L"!pm "+std::wstring(mittente) + mess);

        if(result == false)
            SystemChatToPlayer(dp,L"Player not found, or player with a different server PID",true);

        return true;
    }


    return false;
}

}
