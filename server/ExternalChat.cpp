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

    char buffer[1024];
    const char* localIP = "127.0.0.1";
    std::string binaryIP = "";
    binaryIP.resize(4);
    if(inet_pton(AF_INET,localIP,&binaryIP[0]) != 1)
        return ;

    BufferIO::EncodeUTF8(msg->messaggio,buffer);

    try
    {
        sql::Connection* con = MySqlWrapper::getInstance()->getConnection();
        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("INSERT INTO ajax_chat_messages(userName,userID,userRole,channel,dateTime,ip,text) VALUES (?,3, 1,0,now(),?,?)"));
        //sql::PreparedStatement *stmt = con->prepareStatement("INSERT INTO users VALUES (?, ?, ?)");
        stmt->setString(1, "[---]");
        stmt->setString(2, binaryIP);
        stmt->setString(3, buffer);
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

    try
    {
        sql::Connection* con = MySqlWrapper::getInstance()->getConnection();

        //std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("SELECT rank from (SELECT username,score, @rownum := @rownum + 1 AS rank FROM stats, (SELECT @rownum := 0) r ORDER BY score DESC) z where username = ?"));
        std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("select userName,text,id from ajax_chat_messages where text not like '/%' and id > ? and userID != 3 order by id desc limit 5"));

        stmt->setInt(1, last_id);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        while(res->next())
        {
            std::string username = res->getString(1);
            std::string message = res->getString(2);
            int id = res->getInt(3);
            if(id>last_id)
                last_id = id;
            wchar_t buffer[1024];
            GameServerChat gsc;
            gsc.isAdmin = false;
            gsc.type=MessageType::CHAT;
            BufferIO::DecodeUTF8(message.c_str(),buffer);
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
