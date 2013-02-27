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
        bool parseCommandLine(int argc, char**argv);

        /*config data*/
        int serverport;
        std::string mysql_username;
        std::string mysql_password;
        std::string mysql_host;
        std::string mysql_database;

        private:
        Config();
        std::string configFile;
    };

}
#endif
