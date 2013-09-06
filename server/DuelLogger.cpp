#include "DuelLogger.h"

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

    void DuelLogger::logServerPacket(DuelPlayer* dp,char * data, int len)
    {

        char* pdata = data;
        BufferIO::ReadInt16(pdata);
        unsigned char pktType = BufferIO::ReadUInt8(pdata);
        printf("logpacket %d\n\n",(int)pktType);

        switch(pktType) {
            case STOC_GAME_MSG: {
            //ClientAnalyze(pdata, len - 1);
            break;
            }
            case STOC_DUEL_START: {
                printf("start ricevuto\n\n\n\n\n");
            //ClientAnalyze(pdata, len - 1);
            break;
            }


        }
    }
}
