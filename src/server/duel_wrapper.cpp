#include "common/common_structures.h"
#include "duel_wrapper.h"
#include "network.h"
#include "single_duel.h"
#include "tag_duel.h"
#include "handicap_duel.h"

class SingleDuelWrapper: public ygo::SingleDuel
{
public:
	~SingleDuelWrapper() {}
	SingleDuelWrapper(bool isMatch): ygo::SingleDuel(isMatch) {}
	unsigned short getTimeLimit(unsigned char user)
	{
		return ygo::SingleDuel::time_limit[user];
	}
	void setTimeLimit(unsigned char user, unsigned short tl)
	{
		ygo::SingleDuel::time_limit[user] = tl;
	}
	unsigned short getLastResponse()
	{
		return ygo::SingleDuel::last_response;
	}
	unsigned short getTimeElapsed()
	{
		return ygo::SingleDuel::time_elapsed;
	}
};

class TagDuelWrapper: public ygo::TagDuel
{
public:
	~TagDuelWrapper() {}
	unsigned short getTimeLimit(unsigned char user)
	{
		return ygo::TagDuel::time_limit[user];
	}
	void setTimeLimit(unsigned char user, unsigned short tl)
	{
		ygo::TagDuel::time_limit[user] = tl;
	}
	unsigned short getLastResponse()
	{
		return ygo::TagDuel::last_response;
	}
	unsigned short getTimeElapsed()
	{
		return ygo::TagDuel::time_elapsed;
	}
};

class HandicapDuelWrapper: public ygo::HandicapDuel
{
public:
	~HandicapDuelWrapper() {}
	unsigned short getTimeLimit(unsigned char user)
	{
		return ygo::HandicapDuel::time_limit[user];
	}
	void setTimeLimit(unsigned char user, unsigned short tl)
	{
		ygo::HandicapDuel::time_limit[user] = tl;
	}
	unsigned short getLastResponse()
	{
		return ygo::HandicapDuel::last_response;
	}
	unsigned short getTimeElapsed()
	{
		return ygo::HandicapDuel::time_elapsed;
	}
};




void DuelWrapper::StepTimer(evutil_socket_t fd, short events)
{
	if(duel_mode == MODE_SINGLE || duel_mode == MODE_MATCH)
		SingleDuelWrapper::SingleTimer(fd,events,duel_data);
	else if(duel_mode == MODE_TAG)
		TagDuelWrapper::TagTimer(fd,events,duel_data);
	else if(duel_mode == MODE_HANDICAP)
		HandicapDuelWrapper::TagTimer(fd,events,duel_data);
}

DuelWrapper::DuelWrapper()
{
	duel_data = nullptr;

}
bool DuelWrapper::createDuel(unsigned char mode)
{
	if(duel_data != nullptr)
		return false;

	duel_mode = mode;
	if(mode == MODE_SINGLE)
		duel_data = new SingleDuelWrapper(false);
	else if(mode == MODE_MATCH)
		duel_data = new SingleDuelWrapper(true);
	else if(mode == MODE_TAG)
		duel_data = new TagDuelWrapper();
	else if(mode == MODE_HANDICAP)
		duel_data = new HandicapDuelWrapper();
	return true;
}
bool DuelWrapper::destroyDuel()
{
	if(duel_data == nullptr)
		return false;
	delete duel_data;
	duel_data = nullptr;
	return true;
}
DuelWrapper::~DuelWrapper()
{
	destroyDuel();

}
DuelWrapper::operator bool()
{
	return (duel_data != nullptr);

}

bool DuelWrapper::operator ==(ygo::DuelMode* dm)
{
	return dm == duel_data;

}

ygo::DuelMode* DuelWrapper::operator->()
{
	return duel_data;

}

unsigned short DuelWrapper::getTimeLimit(unsigned char user)
{
	switch (duel_mode)
	{
	case MODE_SINGLE:
	case MODE_MATCH:
		return dynamic_cast<SingleDuelWrapper*>(duel_data)->getTimeLimit(user);
	case MODE_TAG:
		return dynamic_cast<TagDuelWrapper*>(duel_data)->getTimeLimit(user);
	case MODE_HANDICAP:
		return dynamic_cast<HandicapDuelWrapper*>(duel_data)->getTimeLimit(user);
	}

	return 0;
}
void DuelWrapper::setTimeLimit(unsigned char user, unsigned short tl)
{
	switch (duel_mode)
	{
	case MODE_SINGLE:
	case MODE_MATCH:
		return dynamic_cast<SingleDuelWrapper*>(duel_data)->setTimeLimit(user, tl);
	case MODE_TAG:
		return dynamic_cast<TagDuelWrapper*>(duel_data)->setTimeLimit(user, tl);
	case MODE_HANDICAP:
		return dynamic_cast<HandicapDuelWrapper*>(duel_data)->setTimeLimit(user,tl);
	}
}
unsigned short DuelWrapper::getLastResponse()
{
	switch (duel_mode)
	{
	case MODE_SINGLE:
	case MODE_MATCH:
		return dynamic_cast<SingleDuelWrapper*>(duel_data)->getLastResponse();
	case MODE_TAG:
		return dynamic_cast<TagDuelWrapper*>(duel_data)->getLastResponse();
	case MODE_HANDICAP:
		return dynamic_cast<HandicapDuelWrapper*>(duel_data)->getLastResponse();
	}
	return 0;
}
unsigned short DuelWrapper::getTimeElapsed()
{
	switch (duel_mode)
	{
	case MODE_SINGLE:
	case MODE_MATCH:
		return dynamic_cast<SingleDuelWrapper*>(duel_data)->getTimeElapsed();
	case MODE_TAG:
		return dynamic_cast<TagDuelWrapper*>(duel_data)->getTimeElapsed();
	case MODE_HANDICAP:
		return dynamic_cast<HandicapDuelWrapper*>(duel_data)->getTimeElapsed();
	}
	return 0;
}
