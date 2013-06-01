#include "ExternalChat.h"
#include <dirent.h>
#include <stdio.h>

//{"id":"1370101637.4086.51aa178563c099.72780478","sender":"f79556c7709d11e00e774b912b2244ac659987ac","recipient":"channel|xxx","type":"msg","body":"fsd\u2192\u2193","timestamp":1370101637}

static const char* PATH = "./ExternalChat_dir";
namespace ygo
{

ExternalChat* ExternalChat::getInstance()
{
    static ExternalChat ec;
    return &ec;
}

void ExternalChat::broadcastMessage(GameServerChat* msg)
{
    char buffer[1024];
    char* bufferp=buffer;
    wchar_t * msgp = msg->messaggio;
    for(; *msgp != 0; msgp++)
    {
        if(*msgp < 255)
        {
            *bufferp = *msgp;
            bufferp++;
        }
        else
        {
            int n = sprintf(bufferp,"\\u%d",*msgp);
            bufferp += n;
        }
    }
    *bufferp=0;

    DIR *dir = opendir(PATH);
    struct dirent *entry = readdir(dir);

    char randomname[20];
    gen_random(randomname,10);

    char filename[128];

    char stringbuf[512];

    sprintf(stringbuf,"{\"id\":\"%s\",\"sender\":\"000\",\"recipient\":\"channel|xxx\",\"type\":\"msg\",\"body\":\"%s\",\"timestamp\":%u}",randomname,buffer,(unsigned)time(NULL));

    while (entry != NULL)
    {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name,"000") && entry->d_name[0]!= '.')
        {
            snprintf(filename,128,"%s/%s/messages/%s",PATH,entry->d_name,randomname);
            if(FILE* fp = fopen(filename, "w"))
            {
                fwrite(stringbuf,strlen(stringbuf),1,fp);
                fclose(fp);
            }

            printf("filename %s\n",filename);
        }
        entry = readdir(dir);
    }
    closedir(dir);
    printf("external broadcast: %s\n",buffer);


}
std::list<GameServerChat> ExternalChat::getPendingMessages()
{
    std::list<GameServerChat> lista;
    char dirpath[128];
    char filename[512];
    char linebuf[256];
    snprintf(dirpath,128,"%s/000/messages",PATH);

    DIR *dir = opendir(dirpath);
    struct dirent *entry = readdir(dir);
    while (entry != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            snprintf(filename,512,"%s/%s",dirpath,entry->d_name);
            if(FILE* fp = fopen(filename, "r"))
            {
                char nome[20];
                fgets(linebuf, 50, fp);
                strncpy(nome,linebuf,20);
                GameServerChat gss;


                fgets(linebuf, 200, fp);

                swprintf(gss.messaggio,250,L"[%hs<('_')>]:%hs",nome,linebuf);
                gss.isAdmin=false;
                gss.type=MessageType::CHAT;
                lista.push_back(gss);
                unlink(filename);
                //fwrite(stringbuf,strlen(stringbuf),1,fp);
                fclose(fp);
            }

            printf("filename %s\n",filename);
        }
        entry = readdir(dir);
    }

    return lista;
}

ExternalChat::ExternalChat()
{


}

void ExternalChat::gen_random(char *s, const int len) {
    static const char alphanum[] = "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    s[len] = 0;
}

}
