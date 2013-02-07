#include "Config.h"
#include "data_manager.h"
#include "deck_manager.h"
namespace ygo
{
using namespace std;
Config* Config::getInstance()
{
    static Config config;
    return &config;
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
