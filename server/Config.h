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
        std::string spam_string;
        int max_processes;
        int max_users_per_process;
        int waitingroom_min_waiting;
        int waitingroom_max_waiting;
        bool debugSql;
        bool disableMysql;
        bool strictAllowedList;
        bool noExternalChat;
        unsigned int startTimer;
        unsigned int maxTimer;
        private:
        Config();
        std::string configFile;

        void check_variable(int &,std::string value,std::string name);
        void check_variable(std::string &,std::string value,std::string name);
        void check_variable(bool &,std::string value,std::string name);
    };

}
#endif
