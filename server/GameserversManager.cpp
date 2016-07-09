#include "GameserversManager.h"
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "GameServer.h"
#include "debug.h"
#include "Statistics.h"
#include <time.h>

#include "ExternalChat.h"
#include "MySqlWrapper.h"
using namespace std;
namespace ygo
{



volatile bool needsReboot;
GameServer *child_gameserver = nullptr;
void sigterm_handler(int signum)
{
    static int timesPressed = 0;
    if(!needsReboot && child_gameserver)
        child_gameserver->StopServer();
    needsReboot=true;
    ++timesPressed;

    printf("SIGINT received: now %d\n",timesPressed);
    if(timesPressed ==10)
        kill(getpid(),SIGKILL);
	else if(timesPressed ==6)
        kill(getpid(),SIGQUIT);
    else if(timesPressed == 5)
        kill(getpid(),SIGTERM);
}

GameServerStats::GameServerStats(): rooms(0),players(0)
{
    pid = getpid();
}

bool GameserversManager::serversAlmostFull()
{
    int threeshold = Config::getInstance()->max_users_per_process/10;
    threeshold = max(threeshold,1);
    threeshold = min(threeshold,5);
    log(VERBOSE,"high threeshold = %d\n",threeshold);
    bool isAlmostFull = getNumPlayersInAliveChildren()+threeshold > getNumAliveChildren()*Config::getInstance()->max_users_per_process;
    return isAlmostFull;
}
bool GameserversManager::serversAlmostEmpty()
{
    int threeshold = 0.5*Config::getInstance()->max_users_per_process;
    threeshold = min(threeshold,30);
    threeshold = max(threeshold,1);
    log(VERBOSE,"low threeshold = %d\n",threeshold);
    bool isAlmostEmpty = getNumPlayersInAliveChildren() <= (getNumAliveChildren()-1) * Config::getInstance()->max_users_per_process -threeshold;
    return isAlmostEmpty;
}

int GameserversManager::getNumAliveChildren()
{
    int alive=0;
    for(auto it = children.cbegin(); it != children.cend(); ++it)
        if(it->second.isAlive)
            alive++;
    return alive;
}
int GameserversManager::getNumPlayersInAliveChildren()
{
    int numTotal=0;
    for(auto it = children.cbegin(); it!=children.cend(); ++it)
        if(it->second.isAlive)
            numTotal += it->second.players;
    return numTotal;
}

void GameserversManager::ShowStats()
{
    static time_t last_showstats = 0;

    if(time(NULL) - last_showstats > 5)
    {
        last_showstats = time(NULL);
        for(auto it = children.cbegin(); it != children.cend(); ++it)
        {
            ChildInfo gss = it->second;
            printf("pid: %5d, rooms: %3d, users %3d",gss.pid,gss.rooms,gss.players);
            if(!it->second.isAlive)
                printf("  *dying*");
            printf("\n");

        }
        printf("children: %2d, alive %2d, rooms: %3d, players alive:%3d, players: %3d\n",
               (int)children.size(),getNumAliveChildren(),getNumRooms(),getNumPlayersInAliveChildren(),getNumPlayers());
        for(auto it = children.cbegin(); it != children.cend(); ++it)
        {
            ChildInfo gss = it->second;
            Statistics::ServerStats serverStats(gss.pid,gss.players,gss.rooms,Config::getInstance()->max_users_per_process,it->second.isAlive?std::string("ALIVE"):std::string("DYING"));
            Statistics::getInstance()->SendStatisticsRow(serverStats);

        }

    }
}

GameserversManager::GameserversManager():maxchildren(4)
{
   // signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);
    prepara_segnali();
}

void GameserversManager::chatCallback(std::wstring message,bool isAdmin,void*ptr)
{
    GameserversManager* that = (GameserversManager*)ptr;


    /*for(auto it = that->children.cbegin(); it != that->children.cend(); ++it)
    {
            if(it->first)
            ChildInfo gss = it->second;
            printf("pid: %5d, rooms: %3d, users %3d",gss.pid,gss.rooms,gss.players);
            if(!it->second.isAlive)
                printf("  *dying*");
            printf("\n");
    }*/


}

void GameserversManager::child_loop(int receive_fd)
{

    ygo::GameServer* gameServer = new ygo::GameServer();
    close(0);

    if(!gameServer->StartServer(server_fd,receive_fd))
    {
        printf("cannot start the gameserver\n");
        exit(1);
    }
    child_gameserver = gameServer;
    GameServer::ServerThread(gameServer);
    child_gameserver = nullptr;
    delete gameServer;

    exit(EXIT_SUCCESS);
}




int GameserversManager::spawn_gameserver()
{
    int pid;


    int s_pair[2];
    socketpair(PF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0, s_pair);

    MySqlWrapper::getInstance()->disconnect();
    pid = fork();
    MySqlWrapper::getInstance()->connect();

    if(pid)
    {
        close (s_pair[1]);
        ChildInfo gss;
        gss.pid = pid;
        gss.isAlive=true;
        gss.last_update=time(NULL);
        children[s_pair[0]] = gss;
        //gss.players=0;
        //aliveChildren.insert(pipefd[0]);
        cout<<"child created "<<pid<<", now: "<<children.size()<<endl;
        return pid;
    }

    ExternalChat::getInstance()->disconnect();
    Statistics::getInstance()->StopThread();
    Statistics::getInstance()->setNumPlayers(0);
    Statistics::getInstance()->setNumRooms(0);
    close (s_pair[0]);
    for(auto it = children.cbegin(); it != children.cend(); ++it)
        closeChild(it->first);
    children.clear();

    isFather = false;
    child_loop(s_pair[1]);
    return 0;

}

int GameserversManager::getNumRooms()
{
    int rooms = 0;
    for(auto it = children.cbegin(); it != children.cend(); ++it)
        rooms += it->second.rooms;
    return rooms;
}
int GameserversManager::getNumPlayers()
{
    int pl = 0;
    for(auto it = children.cbegin(); it != children.cend(); ++it)
        pl += it->second.players;
    return pl;
}
bool GameserversManager::handleChildMessage(int child_fd)
{
    //true is OK
    byte buffer[sizeof(GameServerChat)];
    int bytesread = read(child_fd,buffer,sizeof(MessageType));
    if(bytesread != sizeof(MessageType))
        return false;

    MessageType type = *((MessageType *)buffer);

    int remaining;
    switch(type)
    {
    case STATS:
        remaining = sizeof(GameServerStats)-sizeof(MessageType);
        break;
    case CHAT:
        remaining = sizeof(GameServerChat)-sizeof(MessageType);
        break;

    default:
        return false;
    }

    bytesread = read(child_fd,buffer+sizeof(MessageType),remaining);

    if((int)bytesread < remaining)
        return false;

    if(type == STATS)
    {
        GameServerStats* gss = (GameServerStats*)buffer;

        log(VERBOSE,"il figlio ha spedito un messaggio\n");
        children[child_fd].players = gss->players;
        children[child_fd].rooms= gss->rooms;
        children[child_fd].isAlive= gss->isAlive;
        children[child_fd].last_update = time(NULL);
        Statistics::getInstance()->setNumPlayers(getNumPlayers());
        Statistics::getInstance()->setNumRooms(getNumRooms());
    }
    else if(type == CHAT)
    {
        GameServerChat* gss = (GameServerChat*)buffer;

        for(auto it = children.cbegin(); it != children.cend(); ++it)
        {
            if(it->first == child_fd)
                continue;
            write(it->first,buffer,sizeof(GameServerChat));
        }
        ExternalChat::getInstance()->broadcastMessage((GameServerChat*) gss);

        /*FILE* fp = fopen("cm-error.log", "at");
        if(fp)
        {
            fprintf(fp, "server crashato pid: %d\n", (int) getpid());
            fclose(fp);
        } */



    }
    // while(1);
    return true;
}

void GameserversManager::parent_loop()
{
    maxchildren = Config::getInstance()->max_processes;


    spawn_gameserver();

    fd_set rfds;
    Statistics::getInstance()->StartThread();
    ExternalChat::getInstance()->connect();
    MySqlWrapper::getInstance()->connect();

    while(true)
    {
        FD_ZERO(&rfds);
        int max_fd=0;
        for(auto it = children.cbegin(); it != children.cend(); ++it)
        {
            max_fd = max(max_fd,it->first);
            FD_SET(it->first,&rfds);
        }
        timeval timeout = {2, 0};
        auto retval = select(max_fd + 1, &rfds, NULL, NULL, &timeout);

        if(needsReboot && server_fd )
        {
            close(server_fd);
            server_fd = 0;
            Statistics::getInstance()->StopThread();
        }
        if(retval > 0)
        {

            int child_fd = 0;
            for(auto it = children.cbegin(); it != children.cend(); ++it)
            {
                if(FD_ISSET(it->first,&rfds))
                {
                    child_fd = it->first;
                    break;
                }
            }


            if(!handleChildMessage(child_fd))
            {
                //il figlio ha chiuso
                printf("figlio terminato,fd: %d, pid: %d\n",child_fd,children[child_fd].pid);
                int status;
                //waitpid(children[child_fd].pid,&status,WNOHANG);
                closeChild(child_fd);
                children.erase(child_fd);

                cout<<"child exited , remaining: "<<children.size()<<endl;


                if(needsReboot && children.size() == 0)
                {
                    printf("PADRE:figli terminati. esco\n");
                    exit(0);
                }
            }
        }
        std::list<GameServerChat> lista =  ExternalChat::getInstance()->getPendingMessages();

        for(auto lit = lista.cbegin(); lit!= lista.cend(); ++lit)
            for(auto it = children.cbegin(); it != children.cend(); ++it)
            {
                write(it->first,&(*lit),sizeof(GameServerChat));
            }


        ShowStats();

        static time_t lastDeadCheck = 0;
        time_t now = time(NULL);
        if(now - lastDeadCheck > 5)
        {
            for(auto it = children.cbegin(); it != children.cend(); ++it)
            {
                ChildInfo gss = it->second;
                if(!gss.isAlive && gss.rooms == 0 )
                    kill(gss.pid,SIGINT);
                else if(now -gss.last_update > 30)
                    kill(gss.pid,SIGINT);

            }
            lastDeadCheck = time(NULL);
        }

        if(needsReboot)
            continue;
        if(!needsReboot && serversAlmostFull() && getNumAliveChildren()<maxchildren)
        {
            spawn_gameserver();
            printf("figli vivi %d, utenti vivi %d\n",getNumAliveChildren(),getNumPlayersInAliveChildren());
        }
        if(serversAlmostEmpty())
        {

            int chosen_one = 0;
            int chosen_one_players = Config::getInstance()->max_users_per_process +1;
            int chosen_pid=0;
            for(auto it = children.cbegin(); it != children.cend(); ++it)
                if(it->second.isAlive && it->second.players<chosen_one_players)
                {
                    chosen_one = it->first;
                    chosen_pid=it->second.pid;
                    chosen_one_players = it->second.players;
                }

            children[chosen_one].isAlive = false;
            kill(chosen_pid,SIGINT);
            if(children.size()-getNumAliveChildren() > Config::getInstance()->max_processes)
                killOneTerminatingServer();
        }
    }

    exit(0);


}
void GameserversManager::killOneTerminatingServer()
{
    printf("troppi figli morenti, ne uccido uno\n");

    int chosen_one = 0;
    int chosen_one_players = Config::getInstance()->max_users_per_process +1;
    int chosen_pid=0;
    for(auto it = children.cbegin(); it != children.cend(); ++it)
        if(!it->second.isAlive && it->second.players < chosen_one_players)
        {
            chosen_one_players = it->second.players;
            chosen_one = it->first;
            chosen_pid = it->second.pid;
        }
    kill(chosen_one,SIGKILL);


}
void GameserversManager::closeChild(int child)
{
    close(child);
    //close(children[child].child_fd);

}
void GameserversManager::StartServer(int port)
{
    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
    if (::bind(server_fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) == -1)
    {
        printf("errore bind\n");
        return ;
    }
    evutil_make_socket_nonblocking(server_fd);

    parent_loop();

}
}


