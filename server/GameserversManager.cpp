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
    return aliveChildren.size();
}
int GameserversManager::getNumPlayersInAliveChildren()
{
    int numTotal=0;
    for(auto it = aliveChildren.begin(); it!=aliveChildren.end(); ++it)
        numTotal += children[*it].players;
    return numTotal;
}

void GameserversManager::ShowStats()
{
    static time_t last_update = 0;

    if(time(NULL) - last_update > 5)
    {
        for(auto it = children.cbegin(); it != children.cend(); ++it)
        {
            GameServerStats gss = it->second;
            printf("pid: %5d, rooms: %3d, users %3d",gss.pid,gss.rooms,gss.players);
            if(aliveChildren.find(it->first) == aliveChildren.end())
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


void GameserversManager::child_loop(int parent_fd)
{
    ygo::GameServer* gameServer = new ygo::GameServer(server_fd);

    if(!gameServer->StartServer())
    {
        printf("cannot start the gameserver\n");
        exit(1);
    }
    else
    {
        bool isRebooting = false;
        while(1)
        {
            sleep(1);
            //printf("spedisco il messaggiofiglio\n");
            GameServerStats gss;
            gss.rooms = Statistics::getInstance()->getNumRooms();
            gss.players = Statistics::getInstance()->getNumPlayers();
            gss.isAlive = !needsReboot;
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

                if(gss.players == 0)
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
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if(pid = fork())
    {
        close (pipefd[1]);
        GameServerStats gss;
        gss.pid = pid;
        children[pipefd[0]] = gss;
        gss.players=0;
        aliveChildren.insert(pipefd[0]);
        cout<<"child created "<<pid<<", now: "<<children.size()<<endl;
        return pid;
    }

    Statistics::getInstance()->StopThread();
    Statistics::getInstance()->setNumPlayers(0);
    Statistics::getInstance()->setNumRooms(0);
    close(pipefd[0]);
    for(auto it = children.cbegin(); it != children.cend(); ++it)
        close(it->first);
    children.clear();

    isFather = false;

    child_loop(pipefd[1]);
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
        GameServerStats gss;
        int bytesread = read(child_fd,&gss,sizeof(gss));

        log(VERBOSE,"letti: %d su %d\n",bytesread,sizeof(gss));
        if((int)bytesread < (int) sizeof(gss))
        {
            //il figlio ha chiuso
            printf("figlio terminato,fd: %d, pid: %d\n",child_fd,children[child_fd].pid);
            int status;
            waitpid(children[child_fd].pid,&status,0);
            close(child_fd);
            children.erase(child_fd);

            cout<<"child exited , remaining: "<<children.size()<<endl;

            bool wasAlive;
            if(aliveChildren.find(child_fd) != aliveChildren.end())
            {
                wasAlive=true;
                aliveChildren.erase(child_fd);
            }
            else
            {
                wasAlive=false;
            }


            if (!needsReboot && wasAlive)
            {
                /* non ricreo immediatamente il figlio crashato
                spawn_gameserver();
                */
                printf("figlio crashato\n");
            }

            if(needsReboot)
            {
                if(children.size() == 0)
                    exit(0);
            }


        }
        else
        {
            //ricevuto messaggio dal figlio
            log(VERBOSE,"il figlio ha spedito un messaggio\n");
            children[child_fd] = gss;
            if(!gss.isAlive && aliveChildren.find(child_fd) != aliveChildren.end())
            {
                aliveChildren.erase(child_fd);
            }
            Statistics::getInstance()->setNumPlayers(getNumPlayers());
            Statistics::getInstance()->setNumRooms(getNumRooms());
            // while(1);

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
            for(auto it = aliveChildren.cbegin(); it != aliveChildren.cend(); ++it)
                if(children[*it].players<chosen_one_players)
                {
                    chosen_one = *it;
                    chosen_pid=children[*it].pid;
                }

            aliveChildren.erase(chosen_one);
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
        if(aliveChildren.find(it->first) == aliveChildren.end())
            if(it->second.players < chosen_one_players)
            {
                chosen_one_players = it->second.players;
                chosen_one = it->first;
                chosen_pid = it->second.pid;
            }
    kill(chosen_one,SIGKILL);


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


