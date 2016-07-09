#include "ExternalChat.h"
#include <dirent.h>
#include <stdio.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/driver.h>
#include "mysql_driver.h"
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <netinet/in.h>
//{"id":"1370101637.4086.51aa178563c099.72780478","sender":"f79556c7709d11e00e774b912b2244ac659987ac","recipient":"channel|xxx","type":"msg","body":"fsd\u2192\u2193","timestamp":1370101637}

#include "MySqlWrapper.h"



namespace ygo
{

ExternalChat* ExternalChat::getInstance()
{
    static ExternalChat ec;
    return &ec;
}

void ExternalChat::broadcastMessage(GameServerChat* msg)
{
    if(Config::getInstance()->noExternalChat)
        return;
    char buffer[1024];
    char nome[25];
    const char* localIP = "127.0.0.1";
    std::string binaryIP = "";
    binaryIP.resize(4);
    if(inet_pton(AF_INET,localIP,&binaryIP[0]) != 1)
        return ;

    std::wstring messaggio = msg->messaggio;


    auto ago = messaggio.find(L"]: ",1);
    if(ago != std::wstring::npos and ago < 28)
    {
        BufferIO::EncodeUTF8(msg->messaggio + ago + 3,buffer);
        std::wstring temp = messaggio.substr(1,ago-1);
        BufferIO::CopyWStr(temp.c_str(),nome,25);


    }
    else
    {
        strcpy(nome,"[---]");
        BufferIO::EncodeUTF8(msg->messaggio,buffer);
        std::cout << buffer <<std::endl;
    }


    try
    {
        sql::Connection* con = MySqlWrapper::getInstance()->getConnection();
        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("INSERT INTO ajax_chat_messages(userName,userID,userRole,channel,dateTime,ip,text,pid,chatColor) VALUES (?,3, 1,0,now(),?,?,?,?)"));
        //sql::PreparedStatement *stmt = con->prepareStatement("INSERT INTO users VALUES (?, ?, ?)");



        stmt->setString(1, std::string(nome));
        stmt->setString(2, binaryIP);
        stmt->setString(3, buffer);
        stmt->setInt(4,pid);
		stmt->setInt(5,msg->chatColor);
        stmt->execute();

    }
    catch (sql::SQLException &e)
    {
        MySqlWrapper::getInstance()->notifyException(e);
        return;
    }
}
std::list<GameServerChat> ExternalChat::getPendingMessages()
{

    std::list<GameServerChat> lista;
    if(Config::getInstance()->noExternalChat)
        return lista;
		
	static time_t lastCheck = 0;
	time_t now = time(NULL);
	if(now - lastCheck <2)	
		return lista;
	lastCheck = now;
	
    try
    {
        sql::Connection* con = MySqlWrapper::getInstance()->getConnection();

        //std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("SELECT rank from (SELECT username,score, @rownum := @rownum + 1 AS rank FROM stats, (SELECT @rownum := 0) r ORDER BY score DESC) z where username = ?"));
        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("select userName,LEFT(text,256),id,chatColor from ajax_chat_messages where text not like '/%' and id > ? and pid != ? order by id desc limit 5"));

        stmt->setInt(1, last_id);
        stmt->setInt(2, pid);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        while(res->next())
        {
            std::string username = res->getString(1);
            std::string message = res->getString(2);
            int id = res->getInt(3);
			int chatColor = res->getInt(4);
            if(id>last_id)
                last_id = id;
            wchar_t buffer[1024];
            GameServerChat gsc;
            gsc.chatColor = chatColor;
            gsc.type=MessageType::CHAT;
            BufferIO::DecodeUTF8(message.c_str(),buffer);
			if(username.find(">",1) != std::string::npos || chatColor < 0)
				swprintf(gsc.messaggio,260,L"[%hs]: %ls",username.c_str(),buffer);
			else
				swprintf(gsc.messaggio,260,L"[%hs<^_^>]: %ls",username.c_str(),buffer);
            lista.push_front(gsc);
        }

    }
    catch (sql::SQLException &e)
    {
        MySqlWrapper::getInstance()->notifyException(e);

    }

    return lista;
}

ExternalChat::ExternalChat()
{
    last_id=0;
    pid = (int)getpid();
}

void ExternalChat::connect()
{

}

void ExternalChat::disconnect()
{

}

void ExternalChat::gen_random(char *s, const int len) {
    static const char alphanum[] = "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    s[len] = 0;
}

}
