
#include <stdio.h>
#include "DuelLogger.h"

LoggerPlayerInfo::LoggerPlayerInfo():type(7)

{
	lpd.turns=0;
	lpd.maxSpSummonTurn = 0;
	lpd.maxAttacksTurn = 0;
	lpd.maxDamage1shot = 0;
	lpd.recoveredDuel = 0;
	lpd.setMonstersDuel = 0;
	lpd.effectsDuel = 0;
	lpd.setSTDuel = 0;
}

void LoggerPlayerInfo::NewTurn()
{
	hisTurn = false;
	lpd.turns++;
	SpSummonTurn = 0;
	attacksTurn = 0;
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
	if(SpSummonTurn > lpd.maxSpSummonTurn)
		lpd.maxSpSummonTurn = SpSummonTurn;
	debugp("%s special summons\n",lpd.name);
}

void LoggerPlayerInfo::SetMonster()
{
	if(!hisTurn)
		return;
	debugp("set monster\n");
	lpd.setMonstersDuel++;
}
	void LoggerPlayerInfo::Effect()
	{
		if(!hisTurn)
		return;
		debugp("Effect\n");
		lpd.effectsDuel++;
		
	}
	void LoggerPlayerInfo::SetST()
	{
		if(!hisTurn)
		return;
		debugp("set st\n");
		lpd.setSTDuel++;
		
	}


void LoggerPlayerInfo::attack()
{
	if(!hisTurn)
		return;
	attacksTurn++;
	if(attacksTurn > lpd.maxAttacksTurn)
		lpd.maxAttacksTurn = attacksTurn;
	debugp("%s attacks summons\n",lpd.name);
}

void LoggerPlayerInfo::Damage(signed char ID, int damage)
{
	if(!hisTurn)
		return;
	if(ID == (1-playerID))
	{
	if(lpd.maxDamage1shot < damage)
		lpd.maxDamage1shot = damage;
	debugp("%s inflicts %d damage\n",lpd.name,damage);
	
	}
	
	
}
void LoggerPlayerInfo::Recover(signed char ID, int damage)
{
	if(!hisTurn)
		return;
	if(ID == playerID)
	{
		lpd.recoveredDuel += damage;
		debugp("%s recovers %d damage\n",lpd.name,damage);
	
	}

	
}
void LoggerPlayerInfo::TypeChange(unsigned char t)
{
	type = t;
}