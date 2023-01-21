#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fcntl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <csignal>
#include <sstream>

#define MAX_CLIENTS 3
#define MAX_ROOMS 2
#define MAX_ERRORS 6
#define MAX_EVENTS 10
#define BUFFER_SIZE 512
#define MAX_INCORRECT_GUESSES 7

struct room_t;

struct client_t {
    int sockfd;
    char nickname[BUFFER_SIZE];
    char player_hangman[BUFFER_SIZE];
    int points;
    int errors;
    bool ready = false;
    room_t* room = NULL;
};

struct room_t {
    char room_name[BUFFER_SIZE];
    char room_hangman[BUFFER_SIZE];
    client_t* clients[MAX_CLIENTS];
    int num_clients;
    int place = 0;
    int lost = 0;
};

int listen_fd {};
const int PORT = 54321;
std::unordered_map<int, client_t> clients;
int num_clients = 0;
room_t* rooms[MAX_CLIENTS];
int num_rooms = 0;

void error(const char* msg) {
    perror(msg);
    exit(1);
}

void sig_handler(int signal){
    shutdown(listen_fd, SHUT_RDWR);
    close(listen_fd);
}

//Wyslanie wiadomosci do klienta
void send_to_client(const char* msg, client_t client) {
    printf("%d|SENDING: %s", client.sockfd, msg);
    int n = write(client.sockfd, msg, strlen(msg));
    if (n < 0) {
        error("ERROR writing to socket");
    }
}

//Zresetowanie statystyk klienta
void reset_stats(client_t* client){
    client->points = 0;
    client->errors = 0;
    client->ready = false;
}

//Przeslanie wiadomosci do wszystkich klientow w pokoju
void broadcast(const char* msg, room_t* room) {
    for (int i=0; i < room->num_clients; i++) {
        send_to_client(msg, *(room->clients[i]));
    }
}

//Usuniecie klienta
void remove_client(client_t client) {
    clients.erase(client.sockfd);
}

//Usuniecie klienta z pokoju
void remove_client_from_room(client_t* client, room_t* room){
    int index = -1;
    for (int i=0; i < room->num_clients; i++){
        if (room->clients[i] == client){
            index = i;
            break;
        }
    }
    if (index != -1){
        for (int i=index; i < room->num_clients-1; i++){
            room->clients[i] = room->clients[i+1];
        }
        room->num_clients--;
    }
}

//Znalezienie pokoju w tablicy pokojow
int find_room(room_t* room) {
    for (int i = 0; i < num_rooms; i++) {
        if (rooms[i] == room) {
            return i;
        }
    }
    return -1;
}

//Usuniecie pokoju
void remove_room(room_t* room) {
    int index = find_room(room);
    if (index >= 0) {
        rooms[index] = rooms[num_rooms - 1];
        num_rooms--;
        free(room);
    }
}

void handle_client(client_t &client) {

    char buffer[BUFFER_SIZE];
    int n = read(client.sockfd, buffer, BUFFER_SIZE - 1);

    if (n > 0) {

        buffer[n] = '\0';
        char* token = strtok(buffer, " ");

        //Ustawienie nicku gracza
        if (strcmp(token, "SETNICK") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                int taken = 0;
                for (const auto& [fd, cl]: clients) {
                    if ((strcmp(cl.nickname, token) == 0) && (fd != client.sockfd)) {
                        taken = 1;
                        break;
                    }
                }
                if (taken) {
                    send_to_client("NICKTAKEN\n", client);
                } else {
                    strcpy(client.nickname, token);
                    send_to_client("NICKACCEPTED\n", client);
                }
            } else {
                send_to_client("NICKEMPTY\n", client);
            }
            //Dołączenie do pokoju
        } else if (strcmp(token, "JOIN") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                send_to_client("SPECIFYROOM\n", client);
            } else {
                room_t* room = NULL;
                for (int i = 0; i < num_rooms; i++) {
                    if (strcmp(rooms[i]->room_name, token) == 0) {
                        room = rooms[i];
                        break;
                    }
                }
                if (room == NULL) {
                    if (num_rooms < MAX_ROOMS) {
                        room = (room_t*) malloc(sizeof(room_t));
                        strcpy(room->room_name, token);
                        room->num_clients = 0;
                        rooms[num_rooms++] = room;
                    }
                }
                if (room == NULL) {
                    send_to_client("ROOMNOTEXIST\n", client);
                } else {
                    if (room->num_clients< MAX_CLIENTS) {
                        room->clients[room->num_clients++] = &client;
                        client.room = room;
                        sprintf(buffer, "JOINED|%s\n", client.nickname);
                        broadcast(buffer, room);
						std::string current_players = "CURRENTPLAYERS|";
						for (int i=0; i< room->num_clients; i++){
							current_players += room->clients[i]->nickname + std::string("|");
						}
						current_players += "\n";
						send_to_client(current_players.c_str(), client);
                    } else {
                        send_to_client("ROOMFULL\n", client);
                    }
                }
            }

            //Sprawdzenie jakie pokoje istnieja i ile jest w nich osob
        } else if (strcmp(token, "ROOMS") == 0) {
            if (num_rooms == 0) {
                send_to_client("NOROOMS\n", client);
            } else {
                sprintf(buffer, "Rooms:\n");
                for (int i = 0; i < num_rooms; i++) {
                    sprintf(buffer + strlen(buffer), "ROOM|%s|(%d/%d)\n", rooms[i]->room_name, rooms[i]->num_clients, MAX_CLIENTS);
                }
                send_to_client(buffer, client);
            }

            //Rozpoczecie rozgrywki
        } else if (strcmp(token, "START") == 0) {
            if (client.room == NULL) {
                send_to_client("OUTSIDEROOM\n", client);
            } else {
                room_t *room = client.room;
                if (room->num_clients < 2) {
                    send_to_client("ATLEAST2PLAYERS\n", client);
                } else {
                    int index = -1;
                    for (int i = 0; i < room->num_clients; i++) {
                        if (room->clients[i] == &client) {
                            index = i;
                            break;
                        }
                    }
                    if (index == 0) {
                        sprintf(buffer, "TOSTARTTYPE: ACCEPT\n");
                        broadcast(buffer, room);
                    } else {
                        send_to_client("NOKING\n", client);
                    }
                }
            }

            //Zakceptuj start rozgrywki
        } else if (strcmp(token, "ACCEPT") == 0) {
                room_t* room = client.room;
                client.ready = true;
                bool game_start = true;
                for (int i=0; i < room->num_clients; i++){
                    if (!room->clients[i]->ready) {
                        game_start = false;
                        break;
                    }
                }
                if (game_start){
                    strcpy(room->room_hangman, "KUKULKA");
                    char * tmp = strdup(room->room_hangman);
                    for (char *p = tmp; *p != '\0'; p++) {
                        *p = '_';
                    }
                    for (int i=0; i < room->num_clients; i++) {
                        strcpy(room->clients[i]->player_hangman, tmp);
                        send_to_client(tmp, *(room->clients[i]));
                    }
                }

                //Zgadnij litere
        } else if (strcmp(token, "GUESS") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                send_to_client("SPECIFYLETTER\n", client);
            } else {
                char letter = token[0];
                bool found = false;
                room_t* room = client.room;
                int points_before = client.points;
                for (int k = 0; k < strlen(room->room_hangman); k++) {
                    if (room->room_hangman[k] == letter && client.player_hangman[k] == '_') {
                        found = true;
                        client.points++;
                        client.player_hangman[k] = letter;
                    }
                }
                if (client.errors < MAX_INCORRECT_GUESSES){
                    send_to_client(client.player_hangman, client);
                }
                if (found) {
                    if (strcmp(client.player_hangman, room->room_hangman) == 0) {
						room->place++;
                        sprintf(buffer, "WON|%d|%s\n", room->place,client.nickname);
                        broadcast(buffer, room);
                        reset_stats(&client);
                        remove_client_from_room(&client, room);
                    }
                } else {
                    client.errors++;
                    if (client.errors == MAX_INCORRECT_GUESSES) 
                        sprintf(buffer, "LOST|%d|%s\n", MAX_CLIENTS-room->lost,client.nickname);
                        room->lost++;
                        broadcast(buffer, room);
                        reset_stats(&client);
                        remove_client_from_room(&client, room);
                    }
                }
                if (room->num_clients == 1){
                    sprintf(buffer, "LOST|%d|%s\n", MAX_CLIENTS-room->lost,room->clients[0]->nickname);
                    room->lost++;
                    send_to_client(buffer, *(room->clients[0]));
                    reset_stats(&client);
                    remove_client_from_room(room->clients[0], room);
                    remove_room(room);
                }
                sprintf(buffer, "GUESS|%s|%d|%d\n", client.nickname, client.points, client.errors);
                broadcast(buffer, room);
            }
            //Opuszczenie pokoju
        } else if (strcmp(token, "LEAVE") == 0) {
            bool left = false;
            room_t* room = client.room;
            if (room != NULL){
                reset_stats(&client);
                remove_client_from_room(&client, client.room);
                sprintf(buffer, "LEFT|%s\n", client.nickname);
                broadcast(buffer, room);
                left = true;
                if (room->num_clients == 0){
                    remove_room(room);
                }
            }
            if (!left){
                send_to_client("OUTSIDEROOM\n", client);
            }
            else {
                send_to_client("LEFTROOM\n", client);
            }
        } else {
            send_to_client("INVALIDCOMMAND\n", client);
        }
    }
}

int main(int argc, char** argv) {

    signal(SIGINT, sig_handler);
    signal(SIGTSTP, sig_handler);

    // utworzenie gniazda nasłuchującego
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        std::cerr << "Błąd przy tworzeniu gniazda nasłuchującego" << std::endl;
        return 1;
    }

    // ustawienie opcji SO_REUSEADDR dla gniazda nasłuchującego
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    // podpięcie gniazda nasłuchującego pod adres i port
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listen_fd, reinterpret_cast<sockaddr *>(&serv_addr), sizeof serv_addr) < 0) {
        std::cerr << "Błąd przy podpinaniu gniazda nasłuchującego" << std::endl;
        return 1;
    }

    // ustawienie gniazda nasłuchującego w tryb nasłuchiwania
    if (listen(listen_fd, SOMAXCONN) < 0) {
        std::cerr << "Błąd przy ustawianiu gniazda nasłuchującego w tryb nasłuchiwania" << std::endl;
        return 1;
    }

    std::cout << "Serwer uruchomiony, czekam na połączenia..." << std::endl;

    // utworzenie deskryptora epoll
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        std::cerr << "Błąd przy tworzeniu deskryptora epoll" << std::endl;
        return 1;
    }

    // dodanie gniazda nasłuchującego do kontenera epoll
    epoll_event ev{};
    ev.data.fd = listen_fd;
    ev.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
        std::cerr << "Błąd przy dodawaniu gniazda nasłuchującego do kontenera epoll" << std::endl;
        return 1;
    }

    // pętla główna serwera
    while (true) {
        // oczekiwanie na zdarzenia na gniazdach z kontenera epoll
        epoll_event events[MAX_EVENTS];
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            std::cerr << "Błąd w funkcji epoll_wait" << std::endl;
            return 1;
        }

        // przetworzenie zdarzeń
        for (int i = 0; i < nfds; ++i) {

            int fd = events[i].data.fd;

            if (fd == listen_fd) {
                // połączenie przychodzące - akceptacja i dodanie do kontenera epoll
                sockaddr_in cli_addr{};
                socklen_t cli_addr_size = sizeof cli_addr;
                int cli_fd = accept(listen_fd, reinterpret_cast<sockaddr *>(&cli_addr), &cli_addr_size);
                if (cli_fd < 0) {
                    std::cerr << "Błąd przy akceptacji połączenia" << std::endl;
                    continue;
                }
                std::cout << "Nowe połączenie z " << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port)
                          << std::endl;

                // ustawienie gniazda klienta w tryb nieblokujący
                int flags = fcntl(cli_fd, F_GETFL, 0);
                fcntl(cli_fd, F_SETFL, flags | O_NONBLOCK);

                // dodanie gniazda klienta do kontenera epoll
                ev.data.fd = cli_fd;
                ev.events = EPOLLIN;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cli_fd, &ev) < 0) {
                    std::cerr << "Błąd przy dodawaniu gniazda klienta do kontenera epoll" << std::endl;
                    continue;
                }

                // dodanie klienta do kontenera
                clients[cli_fd] = {cli_fd};
                num_clients++;

            } else {
                // zdarzenie na gniazdzie klienta
                client_t &client = clients[fd];
                if (events[i].events & EPOLLIN) {
                    handle_client(client);
                }
            }
        }
    }
    return 0;
}



