#ifndef _STATISTICS_H_
#define _STATISTICS_H_

namespace ygo {

class Statistics
{
        public:
        static Statistics* getInstance();
        void setNumPlayers(int);
        void setNumRooms(int);

};

}

#endif
