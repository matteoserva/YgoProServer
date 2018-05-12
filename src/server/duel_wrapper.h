
#include <event2/util.h>

namespace ygo{
	class DuelMode;
}

class DuelWrapper
{
	ygo::DuelMode *duel_data;
	unsigned char duel_mode;
	
	public:
	void StepTimer(evutil_socket_t fd, short events);
	DuelWrapper();
	bool createDuel(unsigned char mode);
	bool destroyDuel();
	~DuelWrapper();
	ygo::DuelMode* operator->();
	operator bool();
	bool operator ==(ygo::DuelMode* dm);
	
	unsigned short getTimeLimit(unsigned char user);
	void setTimeLimit(unsigned char user, unsigned short);
	unsigned short getLastResponse();
	unsigned short getTimeElapsed();
};

#define MODE_HANDICAP   0x10
#define MODE_ANY 0xFF