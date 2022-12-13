//
// Created by jarja on 13.12.2022.
//

#include "Room.h"

Room::Room()
{
    roomAlive=false;
}

void Room::start(std::string passwordsList[])
{

    srand (time(nullptr));

    int randomInd = rand() % passwordsList->length()-1;
    password = passwordsList[randomInd];
    passwordLen = password.length();

    sendGame();
}

void Room::checkHp()
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

void Room::end()
{
    //TODO: Implementacja końca gry gdy został tylko jeden gracz.
}

int Room::checkLetter(char letter, int fd)
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

std::string Room::getPassword()
{
    return password;
}

void Room::setPassword(std::string password)
{
    this->password = password;
}

bool Room::getRoomAlive() const
{
    return roomAlive;
}

void Room::setRoomAlive(bool state)
{
    roomAlive=state;
}

int Room::getPlayersCount()
{
    return players.size();
}

void Room::sendGame()
{
    for(auto &[clientFd, player]: players)
    {
        std::string message = password + "|" + std::to_string(player.getHp()) + "|" + std::to_string(player.getScore()) + "#";
        std::cout << "Client [" << clientFd << "]" << " message = " << message;
        write(player.getClientFd(),message.c_str(),message.length());
    }
}

void Room::readyPlayer(int fd, std::string passwordsList[])
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
        start(passwordsList);
    }
}

bool Room::addPlayer(int fd, int MAX_PLAYERS, Player p)
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

void Room::removePlayer(int fd)
{
    players.erase(fd);
}

bool Room::getPlayerAlive(int fd)
{
    return players[fd].checkIfPlayerIsAlive();
}