//
// Created by jarja on 13.12.2022.
//

#include "Player.h"

Player::Player()
{
    clientFd = -1;
    score =-1;
    hp = -1;
    isReady = false;
}

Player::Player(int sockFd, int HP)
{
    clientFd = sockFd;
    score = 0;
    hp = HP;
    isReady = false;
}

int Player::getClientFd() const
{
    return clientFd;
}

void Player::setClientFd(int sockFd)
{
    clientFd = sockFd;
}

int Player::getScore() const
{
    return score;
}

void Player::setScore(int score)
{
    this->score=score;
}

int Player::getHp() const
{
    return hp;
}

void Player::setHp(int hp)
{
    this->hp=hp;
}

bool Player::getIsReady() const
{
    return isReady;
}

void Player::setIsReady(bool ready)
{
    this->isReady = ready;
}

void Player::addToScore(int points)
{
    score += points;
}

void Player::removeHp(int num)
{
    hp -= num;
}

bool Player::checkIfPlayerIsAlive() const{
    if(hp>0){
        return true;
    }
    else{
        return false;
    }
}
