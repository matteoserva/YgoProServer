#include "Config.h"
#include "data_manager.h"
#include "deck_manager.h"
#include <getopt.h>
#include <signal.h>
#include <event2/thread.h>

const unsigned short PRO_VERSION = 0x1321;
extern const unsigned int BUILD_NUMBER = VERSION;
int enable_log = 2;

namespace ygo
{
using namespace std;
Config* Config::getInstance()
{
    static Config config;
    return &config;
}
bool Config::parseCommandLine(int argc, char**argv)
{
    //true if you must stop
    opterr = 0;
    for (int c; (c = getopt (argc, argv, ":hc:p:")) != -1;)
    {

        switch (c)
        {
        case 'c':
            configFile=optarg;
            cout <<"ss "<<configFile<<endl;
            break;
        case 'h':
            cout<<"-c configfile    for the config file"<<endl;
            cout<<"-h               help "<<endl;
            cout<<"-p num           port "<<endl;
            return true;
        case 'p':
            serverport = stoi(optarg);
            cout<<"command line, port set to: "<<serverport<<endl;
            break;
        case '?':
            if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
            return true;
        case ':':
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            return true;
        default:
            return true;
        }
    }

    if(optind < argc)
    {
        for (int index = optind; index < argc; index++)
            printf ("Non-option argument %s\n", argv[index]);
        return true;
    }
    return false;
}

void Config::check_variable(int &var,std::string value,std::string name)
{
    var=stoi(value);
    printf("caricata la variabile %s con il valore %d\n",name.c_str(),var);

}

void Config::check_variable(std::string &var,std::string value,std::string name)
{
    var = value;
    printf("caricata la variabile %s con il valore %s\n",name.c_str(),value.c_str());


}

void Config::check_variable(bool &var,std::string value,std::string name)
{
    bool val = false;
    std::size_t found = value.find("true");
    if (found!=std::string::npos)
        val=true;

    var = val;
    printf("caricata la variabile %s con il valore %s\n",name.c_str(),value.c_str());
}




void Config::LoadConfig()
{
#ifdef _WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif //_WIN32

    signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
    deckManager.LoadLFList();
    if(wcsstr(deckManager._lfList[0].listName,L"TCG"))
        std::swap(deckManager._lfList[0],deckManager._lfList[1]);

    if(!dataManager.LoadDB("cards.cdb"))
        return;
    //if(!dataManager.LoadStrings("strings.conf"))
      //  return;
    FILE* fp = fopen(configFile.c_str(), "r");
    if(!fp)
    {
        cerr<<"Couldn't open config file: "<<configFile<<endl;
    }
    else
    {
        char linebuf[256];
        char strbuf[32];
        char valbuf[256];
        wchar_t wstr[256];

        fseek(fp, 0, SEEK_END);
        size_t fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        int linenum=0;

        #define CHECK_VARIABLE(VAR) else if(!strcmp(strbuf,#VAR)) check_variable(VAR,valbuf,#VAR)

        for(int linenum=0; ftell(fp) < fsize; linenum++)
        {
            fgets(linebuf, 250, fp);
            int scanfSuccess = sscanf(linebuf, "%s = %[^\n]s", strbuf, valbuf);
            if(scanfSuccess <= 0 || strbuf[0] == '#')
            {
                //ignored line
            }
            else if(scanfSuccess < 2)
            {
                cerr<<"Could not parse line "<<linenum<<": "<<linebuf<<endl;
            }
            else if(!strcmp(strbuf,"serverport"))
            {
                if(serverport)
                    cout<<"the serverport is already set, ignoring the one in the config file"<<endl;
                else
                {
                    serverport = stoi(valbuf);
                    cout<<"serverport set to: "<<serverport<<endl;
                }
                //istringstream ss(valbuf)>>
            }

            CHECK_VARIABLE(mysql_username);
            CHECK_VARIABLE(mysql_password);
            CHECK_VARIABLE(mysql_database);
            CHECK_VARIABLE(mysql_host);
            CHECK_VARIABLE(spam_string);
            CHECK_VARIABLE(max_users_per_process);
            CHECK_VARIABLE(max_processes);
            CHECK_VARIABLE(waitingroom_min_waiting);
            CHECK_VARIABLE(waitingroom_max_waiting);
            CHECK_VARIABLE(noExternalChat);
			CHECK_VARIABLE(debugSql);

            else
                cerr<<"Could not understand the keyword at line"<<linenum<<": "<<strbuf<<endl;
        }
        #undef CHECK_VARIABLE

        fclose(fp);
    }
    if(!serverport)
    {
        cout<<"serverport not set, using default value of 9999"<<endl;
        serverport = 9999;
    }
    strictAllowedList = false;
}

void disMysql(int)
{
    Config::getInstance()->disableMysql=true;


}

void enMysql(int)
{
    Config::getInstance()->disableMysql=false;

}

Config::Config():configFile("server.conf"),serverport(0)
{
    debugSql = true;
    disableMysql = false;
    max_users_per_process = 150;
    max_processes = 3;
    waitingroom_min_waiting = 4;
    waitingroom_max_waiting = 8;
    startTimer = 60;
    maxTimer = 80;
    noExternalChat = false;
    spam_string = "www.ygopro.it <-- this is the official website of this server";
    signal(SIGUSR1,disMysql);
    signal(SIGUSR2,enMysql);
}







}
