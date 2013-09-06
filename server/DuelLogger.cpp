#include "DuelLogger.h"
#include "../ocgcore/field.h"

namespace ygo
{
    DuelLogger::DuelLogger()
    {

    }

    DuelLogger* DuelLogger::getInstance()
    {
        static DuelLogger instance;
        return &instance;
    }
    bool DuelLogger::playerExists(DuelPlayer* dp)
    {
        return (players.find(dp) != players.end());
    }
    void DuelLogger::addPlayer(DuelPlayer* dp)
    {
        //printf("addplayer\n");
        if(!playerExists(dp))
            players[dp] = PlayerInfo();
    }
    void DuelLogger::removePlayer(DuelPlayer*dp)
    {
        //printf("removeplayer\n");
        if(playerExists(dp))
            players.erase(dp);

    }

    void DuelLogger::logServerPacket(DuelPlayer* dp,char * data, int len)
    {
        if(!playerExists(dp))
            return;
        char* pdata = data;
        BufferIO::ReadInt16(pdata);
        unsigned char pktType = BufferIO::ReadUInt8(pdata);
        //printf ("STOCpkt: %d\n",(int)pktType);

        switch(pktType)
        {
            case STOC_TYPE_CHANGE:
            {
                STOC_TypeChange sctc = *((STOC_TypeChange*)pdata);
                int pos = sctc.type & 0x0f;
                players[dp].position = pos;
                //printf("utente in posizione %d\n\n",pos);

                break;
            }


            case STOC_GAME_MSG:
            {
                //ClientAnalyze(pdata, len - 1);
                break;
            }
            case STOC_DUEL_START:
            {
                //printf("start ricevuto\n\n\n\n\n");
                //ClientAnalyze(pdata, len - 1);
                break;
            }
            case MSG_SPSUMMONED:
            {

                break;
            }
        }

        if(pktType != STOC_GAME_MSG)
            return;

        pktType = BufferIO::ReadUInt8(pdata);
        //printf ("gamepkt: %d\n",(int)pktType);
        switch(pktType)
        {

            case MSG_START:
            {
                int is = BufferIO::ReadUInt8(pdata);
                //printf("ricevuto duel start: %x\n",is);
                break;
            }
            case MSG_NEW_TURN:
            {
                int giocatore = BufferIO::ReadInt8(pdata);
                //printf("------------giocatore: %d\n",giocatore);
                break;
            }
        }


    }
}
