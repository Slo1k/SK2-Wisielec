//
// Created by jarja on 13.12.2022.
//

#include <string>
#include <map>

#ifndef PROJEKT_PLAYER_H
#define PROJEKT_PLAYER_H


class Player {
private:
    int clientFd;
    int score;
    int hp;
    bool isReady;

public:
    Player();

    Player(int sockFd, int HP);

    int getClientFd() const;

    void setClientFd(int sockFd);

    int getScore() const;

    void setScore(int score);

    int getHp() const;

    void setHp(int hp);

    bool getIsReady() const;

    void setIsReady(bool ready);

    void addToScore(int points);

    void removeHp(int num);

    bool checkIfPlayerIsAlive() const;
};


#endif //PROJEKT_PLAYER_H
