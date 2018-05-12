

#include "common/server_protocol.h"
#include <vector>
#include <list>

class StatisticsDatabase;
namespace ygo
{
		class ExternalChat;
		class Users;
		class MySqlWrapper;
}
class DatabaseFacade
{
	ygo::MySqlWrapper * mysqlWrapper;
	ygo::ExternalChat * externalChat;
	ygo::Users* users;
	StatisticsDatabase *statisticsDatabase;
	
public:
	DatabaseFacade();
	~DatabaseFacade();


	void connect();
	void disconnect();
	protocol::LoginResult processLoginRequest(protocol::LoginRequest request);
	void processDuelResult(protocol::DuelResult);
	void broadcastMessage(protocol::GameServerChat* );
    std::list<protocol::GameServerChat> getPendingMessages();
	void SendStatisticsRow(protocol::GameServerStats serverStats);
	
	
	
};