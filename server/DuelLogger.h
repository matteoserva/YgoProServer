#include "network.h"

namespace ygo
{

    class PlayerInfo
    {
        public:
        int position;
        bool isFirst;
        bool isTag;
        bool isStarted;


    };

    class DuelLogger
    {
      private:
            DuelLogger();
            std::map<DuelPlayer*,PlayerInfo> players;
            bool playerExists(DuelPlayer* dp);

      public:
             static DuelLogger* getInstance();
             void addPlayer(DuelPlayer* dp);
             void removePlayer(DuelPlayer*dp);



      void logServerPacket(DuelPlayer* dp,char * data, int len);

    };
}
