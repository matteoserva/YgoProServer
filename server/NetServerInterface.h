#ifndef _NetServerInterface_H_
#define _NetServerInterface_H_


namespace ygo {
class DuelPlayer;


class CMNetServerInterface
{
        public:
        CMNetServerInterface();
        virtual ~CMNetServerInterface(){};
        virtual void ExtractPlayer(DuelPlayer* dp)=0;
        virtual void InsertPlayer(DuelPlayer* dp)=0;
        virtual void LeaveGame(DuelPlayer* dp)=0;
        virtual void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)=0;

};
}
#endif
