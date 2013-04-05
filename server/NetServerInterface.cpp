#include "NetServerInterface.h"
#include "debug.h"
#include "RoomManager.h"
#include "GameServer.h"
namespace ygo
{

DuelPlayer* CMNetServerInterface::last_chat_dp = nullptr;
char CMNetServerInterface::net_server_read[0x20000];
char CMNetServerInterface::net_server_write[0x20000];

CMNetServerInterface::CMNetServerInterface(RoomManager* roomManager,GameServer*gameServer):
    roomManager(roomManager),gameServer(gameServer),last_sent(0),isShouting(false)
{

}
DuelPlayer* CMNetServerInterface::getFirstPlayer()
{
    return players.begin()->first;
}

void CMNetServerInterface::SendMessageToPlayer(DuelPlayer*dp, char*msg)
{
    STOC_Chat scc;
    scc.player = dp->type;
    int msglen = BufferIO::CopyWStr(msg, scc.msg, 256);
    SendBufferToPlayer(dp, STOC_CHAT, &scc, 4 + msglen * 2);
}

void CMNetServerInterface::BroadcastSystemChat(std::wstring msg,bool isAdmin)
{
    if(isShouting)
        return;
    for(auto it = players.cbegin(); it!=players.cend(); ++it)
    {
        SystemChatToPlayer(it->first,msg,isAdmin);
    }

}
void CMNetServerInterface::shout(unsigned short* msg,DuelPlayer* dp)
{
    if(last_chat_dp == dp && time(NULL) - dp->chatTimestamp.back() <5)
        return;
    last_chat_dp = dp;

    dp->chatTimestamp.push_back(time(NULL));
    if(dp->chatTimestamp.size() > 5)
    {
        if(dp->chatTimestamp.back() - dp->chatTimestamp.front() <5)
        {
            roomManager->ban(std::string(dp->ip));
            std::cout<<"banned: "<<std::string(dp->ip)<<std::endl;
            LeaveGame(dp);
            return;
        }

        dp->chatTimestamp.pop_front();
    }


    wchar_t name[25];
    wchar_t messaggio[200];
    BufferIO::CopyWStr(msg, messaggio, 200);
    BufferIO::CopyWStr(dp->name, name, 20);
    std::wstring tmp(dp->countryCode.begin(),dp->countryCode.end());
    tmp = L"<"+tmp+L">";
    wcscat(name,tmp.c_str());
    if(dp->loginStatus == Users::LoginResult::AUTHENTICATED || dp->loginStatus == Users::LoginResult::NOPASSWORD)
        shout_internal(std::wstring(messaggio),false,std::wstring(name));
}

void CMNetServerInterface::shout_internal(std::wstring message,bool isAdmin,std::wstring sender)
{
    isShouting=true;
    if(sender!=L"")
        message = L"["+sender+L"]: "+message;
    roomManager->BroadcastMessage(message,isAdmin);
    isShouting=false;
}

void CMNetServerInterface::SystemChatToPlayer(DuelPlayer*dp, std::wstring msg,bool isAdmin)
{
    if(msg.length() > 256)
        msg.resize(256);
    STOC_Chat scc;
    scc.player = isAdmin?8:10;
    int msglen = BufferIO::CopyWStr(msg.c_str(), scc.msg, 256);
    SendBufferToPlayer(dp, STOC_CHAT, &scc, 4 + msglen * 2);
}

int CMNetServerInterface::getNumPlayers()
{
    return players.size();
}
void CMNetServerInterface::SendPacketToPlayer(DuelPlayer* dp, unsigned char proto)
{
    SendBufferToPlayer(dp, proto, nullptr, 0);
}

void CMNetServerInterface::playerReadinessChange(DuelPlayer *dp, bool isReady)
{
    players[dp].isReady = isReady;
    log(VERBOSE,"readiness change %d\n",isReady);
}

void CMNetServerInterface::SendBufferToPlayer(DuelPlayer* dp, unsigned char proto, void* buffer, size_t len)
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
    if(dp)
        bufferevent_write(dp->bev, net_server_write, last_sent);
}
void CMNetServerInterface::ReSendToPlayer(DuelPlayer* dp)
{
    if(players.find(dp) == players.end())
    {
        log(INFO,"resend ignorato\n");
        return;
    }
    if(dp)
        bufferevent_write(dp->bev, net_server_write, last_sent);
}

bool CMNetServerInterface::handleChatCommand(DuelPlayer* dp,wchar_t* msg)
{
    wchar_t* messaggio=msg;
    wchar_t mittente[20];
    if(msg[0] == 0 || msg[0] != '!')
        return false;
    if(!wcsncmp(messaggio,L"!pm ",3) )
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
