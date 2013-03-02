#include "Config.h"
#include "data_manager.h"
#include "deck_manager.h"
#include <getopt.h>
#include <signal.h>
#include <event2/thread.h>

const unsigned short PRO_VERSION = 0x1300;
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
void Config::LoadConfig()
{
#ifdef _WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif //_WIN32

    signal(SIGPIPE, SIG_IGN);

    deckManager.LoadLFList();
    if(!dataManager.LoadDB("cards.cdb"))
        return;
    if(!dataManager.LoadStrings("strings.conf"))
        return;
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
        #define CHECK_NUMERIC_VARIABLE(VAR) else if(!strcmp(strbuf,#VAR)) VAR = stoi(valbuf)
        #define CHECK_STRING_VARIABLE(VAR) else if(!strcmp(strbuf,#VAR)) VAR = valbuf

        for(int linenum=0; ftell(fp) < fsize; linenum++)
        {
            fgets(linebuf, 250, fp);
            int scanfSuccess = sscanf(linebuf, "%s = %s", strbuf, valbuf);
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

            CHECK_STRING_VARIABLE(mysql_username);
            CHECK_STRING_VARIABLE(mysql_password);
            CHECK_STRING_VARIABLE(mysql_database);
            CHECK_STRING_VARIABLE(mysql_host);
            CHECK_NUMERIC_VARIABLE(max_users_per_process);
            CHECK_NUMERIC_VARIABLE(max_processes);
            CHECK_NUMERIC_VARIABLE(waitingroom_min_waiting);
            CHECK_NUMERIC_VARIABLE(waitingroom_max_waiting);

            else
                cerr<<"Could not understand the keyword at line"<<linenum<<": "<<strbuf<<endl;
        }
        #undef CHECK_STRING_VARIABLE
        #undef CHECK_NUMERIC_VARIABLE
        fclose(fp);
    }
    if(!serverport)
    {
        cout<<"serverport not set, using default value of 9999"<<endl;
        serverport = 9999;
    }
}
Config::Config():configFile("server.conf"),serverport(0)
{
    max_users_per_process = 150;
    max_processes = 3;
    int waitingroom_min_waiting = 4;
    int waitingroom_max_waiting = 8;
}

}
