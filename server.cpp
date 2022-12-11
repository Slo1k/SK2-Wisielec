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

using namespace std;

#define MAX_PLAYERS 11
#define MAX_ROOMS 10
#define PORT 8085
#define BUFFER_SIZE 50
#define HP 7
#define PASSWORDS_NUM 3

bool RUNNING = true;

map<int, Room> rooms;

vector<int> clientsFds;

pthread_mutex_t roomsMutex;

pthread_mutex_t clientsFdsMutex;

const string passwordsList[]={"kotek", "dzwig", "piesek"};

void sending_message(int id, string tresc, int len){
	write(id,tresc.c_str(),len);
}

class Player{
	private:
	    int clientFd;
	    int score;
	    int hp;
	    bool isReady;

	public:
        Player()
        {
            clientFd = -1;
            score =-1;
            hp = -1;
            isReady = false;
        }

        Player(int sockFd)
        {
            clientFd = sockFd;
            score = 0;
            hp = HP;
            isReady = false;
        }


        int getClientFd() const
        {
            return clientFd;
        }

        void setClientFd(int sockFd)
        {
            clientFd = sockFd;
        }

        int getScore() const
        {
            return score;
        }

        //Być może zbędne
        void setScore(int score)
        {
            this->score=score;
        }


        int getHp() const
        {
            return hp;
        }

        void setHp(int hp)
        {
            this->hp=hp;
        }

        bool getIsReady() const
        {
            return isReady;
        }

        void setIsReady(bool ready)
        {
            this->isReady = ready;
        }
        
        void addToScore(int points)
        {
            score += points;
        }

        void removeHp(int num)
        {
            hp -= num;
        }

        bool checkIfPlayerIsAlive() const{
            if(hp>0){
                return true;
            }
            else{
                return false;
            }
        }
};

class Room
{
    private:
        map<int, Player> players;
        string password;
        int passwordLen;
        bool roomAlive;

    public:

        Room()
        {
            roomAlive=false;
        }

        void start()
        {

            srand (time(nullptr));

            int randomInd = rand() % PASSWORDS_NUM;
            password = passwordsList[randomInd];
            passwordLen = password.length();

            sendGame();
        }

        void checkHp()
        {
            if (players.size() <= 1)
            {
                end();
            }

            for(auto &[clientFd, player]: players)
            {
                if(player.getHp()>0)
                {
                    break;
                }
            }
        }

        void end()
        {
            //TODO: Implementacja końca gry gdy został tylko jeden gracz.
        }

        int checkLetter(char letter, int fd)
        {
            int count = 0;
            for(int i=0; i<passwordLen; i++)
            {
                if(password[i] == letter)
                {
                    count++;
                }
            }

            if(count>0)
            {
                players[fd].addToScore(count);
            }
            else
            {
                players[fd].removeHp(1);
            }

            checkHp();

            sendGame();

            return count;
        }
        
        string getPassword()
        {
            return password;
        }
    
        void setPassword(string password)
        {
            this->password = password;
        }

        bool getRoomAlive() const
        {
            return roomAlive;
        }
    
        void setRoomAlive(bool state)
        {
            roomAlive=state;
        }

        void sendGame()
        {
            for(auto &[clientFd, player]: players)
            {
                string message = password + "|" + to_string(player.getHp()) + "|" + to_string(player.getScore()) + "\n";
                sending_message(player.getClientFd(),message,message.length());
            }
        }

        void readyPlayer(int fd)
        {
            int readyPlayers = 0;

            for (auto &[clientFd, player]: players)
            {
                if (player.getClientFd() == fd)
                {
                    player.setIsReady(true);
                }
                if (player.getIsReady())
                {
                    readyPlayers++;
                }
            }

            if(players.size()>1 && players.size()==readyPlayers)
            {
                roomAlive=true;
                start();
            }
        }

        bool addPlayer(int fd, Player p)
        {
            if (players.size() < MAX_PLAYERS)
            {
                players[fd] = p;
                return true;
            } 
            else
            {
                return false;
            }
        }
    
        void removePlayer(int fd)
        {
            players.erase(fd);
        }

        bool getPlayerAlive(int fd)
        {
            return players[fd].checkIfPlayerIsAlive();
        }
};

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
    } while(strcmp(tmp,"\n") != 0);

    delete[] tmp;

    return buffer;
};

bool connectToRoom(thread_data *t_data, int roomId)
{
	bool join = false;

	Player tmp_p(t_data->clientFd);
	if(rooms[roomId].getRoomAlive())
    {

		int result = rooms[roomId].addPlayer(tmp_p);

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
        rooms[i] = new Room();    
    }
}

void * clientHandler(void  *t_data)
{
	pthread_detach(pthread_self());

    pthread_mutex_lock(&clientsFdsMutex);
    auto *data = (thread_data*)t_data;

    if (clientsFds.size() > MAX_PLAYERS * MAX_ROOMS)
    {
        close(data->clientFd);
        pthread_mutex_unlock(&clientsFdsMutex);
    }
    else
    {
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

            if(!strncmp(buffer,"join", 4) && !room)
            {
                int id = buffer[5] - '0';
                int result;
                string temp = "Error\n";

                pthread_mutex_lock(&roomsMutex);
                result = connectToRoom(data, id);
                if(result)
                {
                    room = true;
                    temp = to_string(id);
                    temp.append("\n");
                    data->roomId=id;
                }
                sending_message(data->clientFd,temp,temp.length());
                pthread_mutex_unlock(&roomsMutex);
            }
            
            if(!strncmp(buffer,"ready", 5) && room)
            {
                pthread_mutex_lock(&roomsMutex);
                rooms[data->roomId].readyPlayer(data->clientFd);
                pthread_mutex_unlock(&roomsMutex);
            }

            if(!strncmp(buffer,"leave", 5) && room)
            {
                pthread_mutex_lock(&roomsMutex);
                rooms[data->roomId].removePlayer(data->clientFd);
                pthread_mutex_unlock(&roomsMutex);
                room=false;

                string temp = "left room " + to_string(data->roomId) + "\n";
                sending_message(data->clientFd,temp,temp.length());
            }

            if(!strncmp(buffer,"guess", 5) && room && rooms[data->roomId].getRoomAlive && rooms[data->roomId].getPlayerAlive(data->clientFd))
            {
                char letter = buffer[6];
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
		}
        
		pthread_t clientThread;
		auto* t_data = new thread_data;
        
		t_data->clientFd = clientFd;
		t_data->roomId= -1;

        pthread_create(&clientThread, NULL,clientHandler, (void *)t_data);
	}

	close(fd);
    for (auto r: rooms)
    {
        delete r;
    }
    pthread_mutex_destroy(&roomsMutex);
	return(0);
}
