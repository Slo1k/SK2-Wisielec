//
// Created by jarja on 13.12.2022.
//

#ifndef PROJEKT_ROOM_H
#define PROJEKT_ROOM_H

#include <map>
#include <string>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <error.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "Player.h"

class Room
{

private:
    std::map<int, Player> players;
    std::string password;
    int passwordLen;
    bool roomAlive;

public:

    Room();

    void start(std::string passwordsList[]);

    void checkHp();

    void end();

    int checkLetter(char letter, int fd);

    std::string getPassword();

    void setPassword(std::string password);

    bool getRoomAlive() const;

    void setRoomAlive(bool state);

    int getPlayersCount();

    void sendGame();

    void readyPlayer(int fd, std::string passwordsList[]);

    bool addPlayer(int fd, int MAX_PLAYERS, Player p);

    void removePlayer(int fd);

    bool getPlayerAlive(int fd);
};


#endif //PROJEKT_ROOM_H
