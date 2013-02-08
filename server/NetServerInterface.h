#ifndef _NetServerInterface_H_
#define _NetServerInterface_H_
#include "network.h"

namespace ygo {
class DuelPlayer;

class RoomManager;
class CMNetServerInterface
{
        private:
            unsigned short last_sent;
        protected:
            RoomManager* roomManager;
            char net_server_read[0x2000];
            char net_server_write[0x2000];
        public:
        CMNetServerInterface(RoomManager* roomManager);
        virtual ~CMNetServerInterface(){};
        virtual void ExtractPlayer(DuelPlayer* dp)=0;
        virtual void InsertPlayer(DuelPlayer* dp)=0;
        virtual void LeaveGame(DuelPlayer* dp)=0;
        virtual void HandleCTOSPacket(DuelPlayer* dp, char* data, unsigned int len)=0;


        void SendMessageToPlayer(DuelPlayer*dp, char*msg);
        void SendPacketToPlayer(DuelPlayer* dp, unsigned char proto);
        template<typename ST>
        void SendPacketToPlayer(DuelPlayer* dp, unsigned char proto, ST& st)
        {
            char* p = net_server_write;
            BufferIO::WriteInt16(p, 1 + sizeof(ST));
            BufferIO::WriteInt8(p, proto);
            memcpy(p, &st, sizeof(ST));
            last_sent = sizeof(ST) + 3;
            if(dp)
                bufferevent_write(dp->bev, net_server_write, last_sent);

        }
        void SendBufferToPlayer(DuelPlayer* dp, unsigned char proto, void* buffer, size_t len);
        void ReSendToPlayer(DuelPlayer* dp);

};
}
#endif
