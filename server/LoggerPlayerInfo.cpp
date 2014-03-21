
#include <stdio.h>
#include "DuelLogger.h"

LoggerPlayerInfo::LoggerPlayerInfo():type(7),turns(0),maxSpSummonTurn(0)
{
	
}

void LoggerPlayerInfo::NewTurn()
{
	hisTurn = false;
	turns++;
	SpSummonTurn = 0;
}
	void LoggerPlayerInfo::MainPhase()
	{
		hisTurn = true;
	}
void LoggerPlayerInfo::SpecialSummon()
{
	if(!hisTurn)
		return;
	SpSummonTurn++;
	if(SpSummonTurn > maxSpSummonTurn)
		maxSpSummonTurn = SpSummonTurn;
	debugp("%s special summons\n",name);
}

void LoggerPlayerInfo::TypeChange(unsigned char t)
{
	type = t;
}