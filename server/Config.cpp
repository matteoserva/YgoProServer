#include "Config.h"
#include "data_manager.h"
#include "deck_manager.h"
#include <getopt.h>
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
    bool success = true;


    for (int c; (c = getopt (argc, argv, "hc:p:")) != -1;)
    {

        switch (c)
        {

        case 'c':
            configFile=optarg;
            cout <<"ss "<<configFile<<endl;
            break;
        case 'h':
            cout<<"-c configfile"<<endl;
            return true;
        case 'p':
            serverport = stoi(optarg);
            cout<<"command line, port set to: "<<serverport<<endl;
            break;
        case '?':
            if (optopt == 'c')
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);

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
    deckManager.LoadLFList();
    if(!dataManager.LoadDB("cards.cdb"))
        return;
    if(!dataManager.LoadStrings("strings.conf"))
        return;
    FILE* fp = fopen(configFile.c_str(), "r");
    if(!fp)
    {
        cerr<<"Couldn't open config file: "<<configFile<<endl;
        return;
    }
    char linebuf[256];
    char strbuf[32];
    char valbuf[256];
    wchar_t wstr[256];

    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    int linenum=0;
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
            serverport = stoi(valbuf);
            cout<<"serverport set to: "<<serverport<<endl;
            //istringstream ss(valbuf)>>
        }
        else
            cerr<<"Could not understand the keyword at line"<<linenum<<": "<<strbuf<<endl;
    }
    fclose(fp);
}
Config::Config():configFile("server.conf"),serverport(9999)
{

}

}
