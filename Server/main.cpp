#include <iostream>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <error.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <mutex>
#include <vector>
#include <map>

#include "Player.h"
#include "Room.h"

using namespace std;

#define MAX_PLAYERS 11
#define MAX_ROOMS 10
#define PORT 54321
#define BUFFER_SIZE 50
#define HP 7

bool RUNNING = true;

map<int, Room> rooms;

vector<int> clientsFds;

pthread_mutex_t roomsMutex;

pthread_mutex_t clientsFdsMutex;

const string passwordsList[]={"kotek", "dzwig", "piesek"};


struct thread_data{
	int clientFd;
	int roomId;
};

char *readData(int fd, bool *connected){

	char * buffer = new char[BUFFER_SIZE] {};
    char * tmp = new char[2] {};

    do
    {
        int fail=read(fd,tmp,1);
        if(fail<0)
        {
            cout << "Błąd przy próbie odczytu wiadomosci\n";
            pthread_exit(nullptr);
        }
		if(fail==0)
        {
			*connected=false;
			break;
		}
        strcat(buffer,tmp);
    } while(strcmp(tmp,"#") != 0);

    delete[] tmp;

    return buffer;
};

bool connectToRoom(thread_data *t_data, int roomId)
{
	bool join = false;

	Player tmp_p(t_data->clientFd, HP);
	if(rooms[roomId].getRoomAlive())
    {

		int result = rooms[roomId].addPlayer(t_data->clientFd, MAX_PLAYERS, tmp_p);
        printf("Client [%d] result in connectToRoom: %d\n", t_data->clientFd, result);

		if(result>0)
        {
			join = true;
			t_data->roomId=roomId;
		}
	}
	return join;
}

void createRooms()
{
    for (int i=0; i < MAX_ROOMS; i++)
    {
        rooms[i] = Room();
        rooms[i].setRoomAlive(true);
    }
}

void * clientHandler(void  *t_data)
{
	pthread_detach(pthread_self());

    pthread_mutex_lock(&clientsFdsMutex);
    auto *data = (thread_data*)t_data;

    printf("Client [%d] in client handler\n", data->clientFd);

    if (clientsFds.size() > MAX_PLAYERS * MAX_ROOMS)
    {
        printf("Client [%d] server full\n", data->clientFd);

        string temp = "SERVERFULL#";
        write(data->clientFd,temp.c_str(),temp.length());
        close(data->clientFd);
        pthread_mutex_unlock(&clientsFdsMutex);
        terminate();
    }
    else
    {
        printf("Client [%d] server not full\n", data->clientFd);
        clientsFds.push_back(data->clientFd);
        pthread_mutex_unlock(&clientsFdsMutex);

        bool connected = true;
        bool room = false;
        char * buffer;

        while(connected && RUNNING){

            buffer = readData(data->clientFd, &connected);
            if(!buffer)
            {
                break;
            }

            if(!strncmp(buffer,"JOIN", 4) && !room)
            {
                int id = buffer[5] - '0';
                printf("Client [%d] JOIN to room %d\n", data->clientFd, id);
                int result;
                string temp = "ERROR#";

                pthread_mutex_lock(&roomsMutex);
                result = connectToRoom(data, id);
                if(result)
                {
                    room = true;
                    temp = "ROOM|" + to_string(id) + "#";
                    data->roomId=id;
                }
                write(data->clientFd,temp.c_str(),temp.length());
                pthread_mutex_unlock(&roomsMutex);
            }
            
            if(!strncmp(buffer,"READY", 5) && room)
            {
                printf("Client [%d] READY\n", data->clientFd);
                pthread_mutex_lock(&roomsMutex);
                string temp = "READY#";
                write(data->clientFd, temp.c_str(), temp.length());
                rooms[data->roomId].readyPlayer(data->clientFd, const_cast<string *>(passwordsList));
                pthread_mutex_unlock(&roomsMutex);
            }

            if(!strncmp(buffer,"LEAVE", 5) && room)
            {
                printf("Client [%d] LEAVE\n", data->clientFd);
                pthread_mutex_lock(&roomsMutex);
                rooms[data->roomId].removePlayer(data->clientFd);
                pthread_mutex_unlock(&roomsMutex);
                room=false;

                string temp = "LEFT#";
                write(data->clientFd,temp.c_str(),temp.length());
            }

            if(!strncmp(buffer,"GUESS", 5) && room && rooms[data->roomId].getRoomAlive() && rooms[data->roomId].getPlayerAlive(data->clientFd))
            {
                char letter = buffer[6];
                printf("Client [%d] GUESS %c\n", data->clientFd, letter);
                rooms[data->roomId].checkLetter(letter, data->clientFd);
            }
            delete buffer;
        }

        close(data->clientFd);
        if(data->roomId>-1){
            rooms[data->roomId].removePlayer(data->clientFd);
        }
        delete data;
    }
}

int main(int argc, char ** argv) {
    
    pthread_mutex_init(&roomsMutex,NULL);
    pthread_mutex_init(&clientsFdsMutex,NULL);
    
    createRooms();

    sockaddr_in myAddr {};
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myAddr.sin_port = htons((uint16_t)PORT);

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1){
		perror("socket failed");
		return 1;
	}

	int fail = bind(fd, (sockaddr*) &myAddr, sizeof(myAddr));
	if(fail){
		perror("bind failed");
		return 1;
	}

	fail = listen(fd, 1);
	if(fail){
		perror("listen failed");
		return 1;
	}

	while(RUNNING){
		int clientFd = accept(fd, nullptr, nullptr);
		if(clientFd == -1){
			perror("accept failed");
			return 1;
		} else
        {
            printf("Client socket: %d\n", clientFd);
        }
        
		pthread_t clientThread;
		auto* t_data = new thread_data;

		t_data->clientFd = clientFd;
		t_data->roomId= -1;

        pthread_create(&clientThread, NULL,clientHandler, (void *)t_data);
	}

	close(fd);
    pthread_mutex_destroy(&clientsFdsMutex);
    pthread_mutex_destroy(&roomsMutex);
	return(0);
}
