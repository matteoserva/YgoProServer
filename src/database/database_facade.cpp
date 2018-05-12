#include "common/common_structures.h"

#include "Users.h"

#include "ExternalChat.h"
#include "database_facade.h"
#include "string.h"
#include "statistics_database.h"
#include "ExternalChat.h"
#include "MySqlWrapper.h"
#include "common/Config.h"
DatabaseFacade::DatabaseFacade()
{
	mysqlWrapper = new ygo::MySqlWrapper();
	externalChat = new ygo::ExternalChat(mysqlWrapper);
	users = new ygo::Users(mysqlWrapper);
	statisticsDatabase = new StatisticsDatabase(mysqlWrapper);
	connect();
}

void DatabaseFacade::connect()
{
	sql::ConnectOptionsMap connection_properties;
	std::string host = "tcp://" + ygo::Config::getInstance()->mysql_host + ":3306";
	connection_properties["hostName"]=host;
	connection_properties["userName"]=ygo::Config::getInstance()->mysql_username;
	connection_properties["password"]=ygo::Config::getInstance()->mysql_password;
	connection_properties["MYSQL_OPT_READ_TIMEOUT"]=1;
	connection_properties["MYSQL_OPT_RECONNECT"]=true;
	connection_properties["MYSQL_OPT_CONNECT_TIMEOUT"]=1;
	connection_properties["schema"]=ygo::Config::getInstance()->mysql_database;
	mysqlWrapper->connect(connection_properties);
}

DatabaseFacade::~DatabaseFacade()
{
	delete externalChat;
	delete users;
	delete statisticsDatabase;
	delete mysqlWrapper;
}


protocol::LoginResult DatabaseFacade::processLoginRequest(protocol::LoginRequest request)
{
	protocol::LoginResult result;
	result.type = protocol::LOGINRESULT;

	std::string cc = users->getCountryCode(std::string(request.ip));
	auto ll = users->login(std::string(request.loginstring),request.ip);
	auto ss = users->getScoreRank(ll.first);

	strncpy(result.countryCode,cc.c_str(),sizeof(result.countryCode));

	strncpy(result.name,ll.first.c_str(),  sizeof(result.name));
	result.loginStatus = ll.second;
	result.color = ll.color;
	result.cachedScore = ss.first;
	result.cachedRank = ss.second;
	result.handle = request.handle;

	return result;
}

void DatabaseFacade::processDuelResult(protocol::DuelResult res)
{
	if(res.num_players <= 0)
		return;


	std::vector<LoggerPlayerData> nomi;
	for(int i = 0; i < res.num_players; i++)
		nomi.push_back(res.players[i]);



	users->saveDuelResult(nomi,res.winner,std::string(res.replayCode));

}
void DatabaseFacade::disconnect()
{

	mysqlWrapper->disconnect();

}

void DatabaseFacade::broadcastMessage(protocol::GameServerChat * mes)
{
	externalChat->broadcastMessage(mes);

}
std::list<protocol::GameServerChat> DatabaseFacade::getPendingMessages()
{
	return externalChat->getPendingMessages();

}


void DatabaseFacade::SendStatisticsRow(protocol::GameServerStats serverStats)
{
	statisticsDatabase->SendStatisticsRow(serverStats);



}
