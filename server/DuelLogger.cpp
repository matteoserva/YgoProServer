#include "DuelLogger.h"
#include "network.h"
#include "field.h" //MSG_BATTLECMD
#include <stdio.h>

using namespace ygo;

void debugp(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	//vprintf(format, args);
	va_end(args);
}
DuelLogger::DuelLogger()
{
	debugp("hello from the logger\n");
	
}

DuelLogger::~DuelLogger()
{
	
	
}

void DuelLogger::LogClientMessage(uintptr_t dp,unsigned char proto, char*buffer,size_t size)
{
	//debugp("message from the client\n");
	
	if(proto == CTOS_RESPONSE)
		switch(players[dp].ultimo_game_msg)
		{
			case MSG_SELECT_BATTLECMD:
				{
					debugp("battlecmd: %lx risponde %d:\n",dp,buffer[1]);
					if(buffer[1] == 3)
						debugp("finisce turno\n");
					if(buffer[1] == 1)
						players[dp].attack();
					break;
				}
			case MSG_SELECT_IDLECMD:
				{
					debugp("idlecmd: %s %lx risponde %d:",players[dp].name,dp,buffer[1]);

					if(buffer[1] == 7)
						debugp("finisce turno\n");
					if(buffer[1] == 0)
						debugp("normal summon\n");
					//if(buffer[1] == 1)
						//debugp("special summon\n");
					if(buffer[1] == 5)
						players[dp].Effect();
					if(buffer[1] == 4)
						players[dp].SetST();
					if(buffer[1] == 3)
						players[dp].SetMonster();
					break;
				}
			case MSG_SELECT_CHAIN:
				{
					if(buffer[1] == -1)
						debugp("%lx refuses to chain\n",dp);
					else
						debugp("%lx chains with %x\n",dp,buffer[1]);
					break;
				}
			case MSG_SELECT_PLACE:
				{
					//debugp("%lx sets monster %x\n",dp,buffer[1]);
					break;
				}
			case MSG_SELECT_POSITION:
				{
					
					break;
				}
			case MSG_SELECT_CARD:
				{
					debugp("%lx select card %x\n",dp,buffer[1]);
					break;
				}
			default:
				{
					debugp("sconosciuto da %lx. messaggio %d, risponde %d\n",dp,players[dp].ultimo_game_msg,buffer[1]);
				
				}
			
		}
}	

void DuelLogger::LogServerMessage(uintptr_t dp,unsigned char proto, char*buffer,size_t size)
{
	switch (proto)
	{
		case STOC_TYPE_CHANGE:
		{
			STOC_TypeChange * stc = (STOC_TypeChange*) buffer;
			unsigned char type = stc->type & 0xf;
			players[dp].TypeChange(type);
			debugp("giocatore %lx diventa tipo: %d\n",dp,type);
			
			break;
		}
		case STOC_HS_PLAYER_ENTER:
		{
			STOC_HS_PlayerEnter* spe = (STOC_HS_PlayerEnter*) buffer;
			if(players[dp].type == spe->pos)
			{
				BufferIO::CopyWStr(spe->name,players[dp].name,20);
			}
			
			break;
		}
		case STOC_GAME_MSG:
		{
			players[dp].ultimo_game_msg = buffer[0];
			if(buffer[0] == MSG_NEW_TURN)
				players[dp].NewTurn();
			if(buffer[0] ==	MSG_SPSUMMONED)
				players[dp].SpecialSummon();
			if(buffer[0] ==	MSG_DAMAGE)
			{
				int * p = (int*) &buffer[2];
				players[dp].Damage(buffer[1],*p);
				
			}
			if(buffer[0] ==	MSG_RECOVER)
			{
				int * p = (int*) &buffer[2];
				players[dp].Recover(buffer[1],*p);
			}
			if(buffer[0] == MSG_START)
				players[dp].playerID = buffer[1] & 0x0f;
			if(buffer[0] == MSG_SELECT_IDLECMD)
					players[dp].MainPhase();
			break;
		}
		
		
		
	}
	
	//debugp("message from the server\n");
}
LoggerPlayerInfo* DuelLogger::getPlayerInfo(uintptr_t dp)
{
	return &players[dp];
	
}