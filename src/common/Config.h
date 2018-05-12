#ifndef _CONFIG_H_
#define _CONFIG_H_
#include <string>
#include <unistd.h>
namespace ygo
{
    class Config
    {
        public:
        static Config* getInstance();
        bool LoadConfig();
		bool LoadCards();
        bool parseCommandLine(int argc, char**argv);
	
	bool start_server;
	bool start_manager;
	
        /*config data*/
        int serverport;
		int managerport;
		std::string manager_ip;
		
        std::string mysql_username;
        std::string mysql_password;
        std::string mysql_host;
        std::string mysql_database;
        std::string spam_string;
		std::string client_path;
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
