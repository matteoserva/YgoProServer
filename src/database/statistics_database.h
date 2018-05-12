#include "common/server_protocol.h"
namespace ygo
{
	
	class MySqlWrapper;
}
class StatisticsDatabase
{
	ygo::MySqlWrapper *mysqlWrapper;
public:
	StatisticsDatabase(ygo::MySqlWrapper *);
	void SendStatisticsRow(protocol::GameServerStats serverStats);
	
	
	
	
	
};