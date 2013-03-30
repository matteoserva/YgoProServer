#include "GameserversManager.h"
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "GameServer.h"
#include "debug.h"
#include "Statistics.h"
#include <time.h>
using namespace std;
namespace ygo
{

static volatile bool needsReboot;
void sigterm_handler(int signum)
{
    static int timesPressed = 0;
    needsReboot=true;
    ++timesPressed;
    if(timesPressed ==10)
        kill(getpid(),SIGKILL);
    else if(++timesPressed >= 5)
        kill(getpid(),SIGQUIT);
}

GameServerStats::GameServerStats(): rooms(0),players(0)
{
    pid = getpid();
}

bool GameserversManager::serversAlmostFull()
{
    int threeshold = Config::getInstance()->max_users_per_process*9/10;
    threeshold = max(threeshold,1);
    threeshold = min(threeshold,5);
    log(VERBOSE,"high threeshold = %d\n",threeshold);
    return getNumPlayersInAliveChildren()+threeshold > getNumAliveChildren()*Config::getInstance()->max_users_per_process;
}
bool GameserversManager::serversAlmostEmpty()
{
    int threeshold = 0.5*Config::getInstance()->max_users_per_process;
    threeshold = min(threeshold,50);
    threeshold = max(threeshold,1);
    log(VERBOSE,"low threeshold = %d\n",threeshold);
    return getNumPlayersInAliveChildren() <= (getNumAliveChildren()-1) * Config::getInstance()->max_users_per_process -threeshold;
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
    static time_t last_update = 0;

    if(time(NULL) - last_update > 5)
    {
        for(auto it = children.cbegin(); it != children.cend(); ++it)
        {
            ChildInfo gss = it->second;
            printf("pid: %5d, rooms: %3d, users %3d",gss.pid,gss.rooms,gss.players);
            if(!it->second.isAlive)
                printf("  *dying*");
            printf("\n");
        }
        printf("children: %2d, alive %2d, rooms: %3d, players: %3d,in alive servers:%3d\n",
               (int)children.size(),getNumAliveChildren(),getNumRooms(),getNumPlayers(),getNumPlayersInAliveChildren());
        last_update = time(NULL);
    }
}

GameserversManager::GameserversManager():maxchildren(4)
{
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);
}

void GameserversManager::chatCallback(std::wstring message,bool isAdmin,void*ptr)
{
    GameserversManager* that = (GameserversManager*)ptr;

    GameServerChat gsc;
    gsc.type = CHAT;
    wcscpy(gsc.messaggio,message.c_str());
    gsc.isAdmin = isAdmin;
    write(that->parent_fd,&gsc,sizeof(GameServerChat));
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

    ygo::GameServer* gameServer = new ygo::GameServer(server_fd);
    close(0);
    gameServer->setChatCallback(GameserversManager::chatCallback,this);

    if(!gameServer->StartServer())
    {
        printf("cannot start the gameserver\n");
        exit(1);
    }
    else
    {
        bool isRebooting = false;


        fd_set rfds;






        while(1)
        {
            FD_ZERO(&rfds);
            int max_fd=0;

            FD_SET(receive_fd,&rfds);
            timeval timeout = {1, 0};
            auto retval = select(receive_fd + 1, &rfds, NULL, NULL, &timeout);


            if(retval > 0)
            {
                GameServerChat gsc;
                int bytesread = read(receive_fd,&gsc,sizeof(gsc));
                gameServer->injectChatMessage(gsc.messaggio,gsc.isAdmin);
            }
            //printf("spedisco il messaggiofiglio\n");
            GameServerStats gss;
            gss.rooms = Statistics::getInstance()->getNumRooms();
            gss.players = Statistics::getInstance()->getNumPlayers();
            gss.isAlive = !needsReboot;
            gss.type = STATS;
            write(parent_fd,&gss,sizeof(GameServerStats));
            GameServer::CheckAliveThread(gameServer);
            if (needsReboot)
            {
                if(!isRebooting)
                {
                    cout<<"rebooting\n";
                    gameServer->StopServer();
                    isRebooting = true;
                }

                if(gameServer->getNumPlayers() == 0)
                {
                    delete gameServer;
                    exit(0);
                }
            }
        }
    }
    exit(EXIT_SUCCESS);
}




int GameserversManager::spawn_gameserver()
{
    int pid;
    int pipefd[2];
    int pipefd2[2];

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(pipefd2) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if(pid = fork())
    {
        close (pipefd[1]);
        close (pipefd2[0]);
        ChildInfo gss;
        gss.pid = pid;
        gss.child_fd = pipefd2[1];
        gss.isAlive=true;
        children[pipefd[0]] = gss;
        //gss.players=0;
        //aliveChildren.insert(pipefd[0]);
        cout<<"child created "<<pid<<", now: "<<children.size()<<endl;
        return pid;
    }

    Statistics::getInstance()->StopThread();
    Statistics::getInstance()->setNumPlayers(0);
    Statistics::getInstance()->setNumRooms(0);
    close(pipefd[0]);
    close(pipefd2[1]);
    for(auto it = children.cbegin(); it != children.cend(); ++it)
        closeChild(it->first);
    children.clear();

    isFather = false;
    parent_fd = pipefd[1];
    child_loop(pipefd2[0]);
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

        Statistics::getInstance()->setNumPlayers(getNumPlayers());
        Statistics::getInstance()->setNumRooms(getNumRooms());
    }
    else if(type == CHAT)
    {
        GameServerStats* gss = (GameServerStats*)buffer;

        for(auto it = children.cbegin(); it != children.cend(); ++it)
        {
            if(it->first == child_fd)
               continue;
            write(it->second.child_fd,buffer,sizeof(GameServerChat));
        }
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
        if(retval <= 0)
            continue;

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
            waitpid(children[child_fd].pid,&status,0);
            closeChild(child_fd);
            children.erase(child_fd);

            cout<<"child exited , remaining: "<<children.size()<<endl;


            if(needsReboot && children.size() == 0)
            {
                printf("PADRE:figli terminati. esco\n");
                exit(0);
            }




        }


        ShowStats();
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
                }

            children[chosen_one].isAlive = false;
            kill(chosen_pid,SIGTERM);
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
    close(children[child].child_fd);

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


