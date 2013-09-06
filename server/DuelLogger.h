#include "network.h"

namespace ygo
{
    class DuelLogger
    {
      private:
            DuelLogger();

      public:
             static DuelLogger* getInstance();

      void logServerPacket(DuelPlayer* dp,char * data, int len);

    };
}
