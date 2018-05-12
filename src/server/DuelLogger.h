#ifndef _DUEL_LOGGER_
#define _DUEL_LOGGER_
#include <stdint.h>
#include <cstddef>
#include <map>

void debugp(const char *format, ...);

#include "common/common_structures.h"


class LoggerPlayerInfo
{
	
public:
	LoggerPlayerData lpd;

	LoggerPlayerInfo();
	unsigned char type;
	unsigned char ultimo_game_msg;
	bool hisTurn;
	
	
	unsigned int SpSummonTurn;
	
	unsigned int attacksTurn;
	signed char playerID;
	
	
	
	
	
	void NewTurn();
	void MainPhase();
	void SpecialSummon();
	void TypeChange(unsigned char);
	void attack();
	void Damage(signed char, int);
	void Recover(signed char, int);
	void SetMonster();
	void Effect();
	void SetST();
};

class DuelLogger
{
	std::map<uintptr_t, LoggerPlayerInfo> players;
	public:
	DuelLogger();
	~DuelLogger();
	void LogClientMessage(uintptr_t dp,unsigned char, char*buffer,size_t size);
	void LogServerMessage(uintptr_t dp,unsigned char, char*buffer,size_t size);
	LoggerPlayerInfo* getPlayerInfo(uintptr_t dp);
	
};
#endif