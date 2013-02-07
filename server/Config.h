#ifndef _CONFIG_H_
#define _CONFIG_H_
#include <string>

namespace ygo
{
    class Config
    {
        public:
        static Config* getInstance();
        void LoadConfig();
        int serverport;
        bool parseCommandLine(int argc, char**argv);

        private:
        Config();
        std::string configFile;
    };

}
#endif
