#include <stdio.h>
#include <cstring>
#include <unordered_map>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fcntl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <csignal>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <vector>

#define MAX_CLIENTS 9
#define MAX_ROOMS 4
#define MAX_EVENTS 10
#define BUFFER_SIZE 512
#define MAX_INCORRECT_GUESSES 6

struct room_t;

struct client_t {
    int sockfd;
    char nickname[BUFFER_SIZE];
    char player_hangman[BUFFER_SIZE];
    int points = 0;
    int errors = 0;
    bool isking = false;
    bool ready = false;
    room_t* room = NULL;
};

struct room_t {
    char room_name[BUFFER_SIZE];
    char room_hangman[BUFFER_SIZE];
    client_t* clients[MAX_CLIENTS];
    bool in_game = false;
    int clients_at_start = 0;
    int num_clients;
    int place;
    int lost;
};

int listen_fd {};
std::unordered_map<int, client_t> clients;
int num_clients = 0;
room_t* rooms[MAX_CLIENTS];
int num_rooms = 0;
std::string file_name = "words.txt";

void error(const char* msg) {
    perror(msg);
    exit(1);
}

void sig_handler(int signal){
    shutdown(listen_fd, SHUT_RDWR);
    close(listen_fd);
}


std::string get_random_word(const std::string& file_name){
    std::ifstream file(file_name);
    if (!file.is_open()){
        std::cerr << "Blad przy otwieraniu pliku: " << file_name << std::endl;
        return "";
    }
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(file, line)){
        lines.push_back(line);
    }
    file.close();
    if (lines.empty()){
        std::cerr << "Plik jest pusty!" << std::endl;
        return "";
    }
    int random_index = rand() % lines.size();
    return lines[random_index];
}

//Wyslanie wiadomosci do klienta
void send_to_client(const char* msg, client_t client) {
    printf("%d|SENDING: %s", client.sockfd, msg);
    int n = write(client.sockfd, msg, strlen(msg));
    if (n < 0) {
        error("Blad przy pisaniu do socketa");
    }
}

//Zresetowanie statystyk klienta
void reset_stats(client_t* client){
    client->room = NULL;
    client->points = 0;
    client->errors = 0;
    client->isking = false;
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
        client->room = NULL;
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
void reset_room(room_t* room) {
    int index = find_room(room);
    rooms[index]->in_game = false;
    rooms[index]->clients_at_start = 0;
    rooms[index]->num_clients = 0;
    rooms[index]->place = 1;
    rooms[index]->lost = 0;
}

int handle_client(client_t &client) {

    char buffer[BUFFER_SIZE];
    int n = read(client.sockfd, buffer, BUFFER_SIZE - 1);

    if (n > 0) {

        buffer[n] = '\0';
        char *token = strtok(buffer, " ");

        if (strcmp(token, "SETNICK") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                int taken = 0;
                for (const auto &[fd, cl]: clients) {
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
            }
        } else if (strcmp(token, "JOIN") == 0) {
            token = strtok(NULL, " ");
            room_t *room;
            for (int i = 0; i < num_rooms; i++) {
                if (strcmp(rooms[i]->room_name, token) == 0) {
                    room = rooms[i];
                    break;
                }
            }
            if (room->num_clients < MAX_CLIENTS && !(room->in_game)) {
                std::string current_players = "CURRENTPLAYERS|";
                for (int i = 0; i < room->num_clients; i++) {
                    current_players += room->clients[i]->nickname + std::string("|");
                }
                current_players += "\n";
                room->clients[room->num_clients++] = &client;
                client.room = room;
                sprintf(buffer, "JOINED|%s\n", client.nickname);
                broadcast(buffer, room);
                send_to_client(current_players.c_str(), client);
                if (room->num_clients == 1) {
                    client.isking = true;
                    send_to_client("ISKING\n", client);
                }
            } else {
                send_to_client("ROOMFULL\n", client);
            }
        } else if (strcmp(token, "ROOMS") == 0) {
                sprintf(buffer, "Rooms:\n");
                for (int i = 0; i < num_rooms; i++) {
                    sprintf(buffer + strlen(buffer), "ROOM|%s|(%d/%d)\n", rooms[i]->room_name, rooms[i]->num_clients,
                            MAX_CLIENTS);
                }
                send_to_client(buffer, client);
        } else if (strcmp(token, "START") == 0) {
                room_t *room = client.room;
                if (room->num_clients < 2) {
                    send_to_client("ATLEAST2PLAYERS\n", client);
                } else {
                    if (client.isking){
                        room->in_game = true;
                        room->clients_at_start = room->num_clients;
                        sprintf(buffer, "START\n");
                        broadcast(buffer, room);
                    }
                }
        } else if (strcmp(token, "ACCEPT") == 0) {
            room_t *room = client.room;
            client.ready = true;
            bool game_start = true;
            for (int i = 0; i < room->num_clients; i++) {
                std::cout << room->clients[i]->ready << std::endl;
                if (!room->clients[i]->ready) {
                    game_start = false;
                    break;
                }
            }
            if (game_start) {
                std::string random_word = get_random_word(file_name);
                if (random_word.empty()) {
                    std::cerr << "Brak hasel do wylosowania!";
                    return 1;
                } else {
                    strcpy(room->room_hangman, random_word.c_str());
                    char *tmp = strdup(room->room_hangman);
                    for (char *p = tmp; *p != '\0'; p++) {
                        *p = '_';
                    }
                    std::string hangman_message = "HANGMANSTART|";
                    hangman_message += room->room_hangman;
                    hangman_message += "\n";
                    for (int i = 0; i < room->num_clients; i++) {
                        strcpy(room->clients[i]->player_hangman, tmp);
                        send_to_client(hangman_message.c_str(), *(room->clients[i]));
                    }
                }
            }
        } else if (strcmp(token, "GUESS") == 0) {
            token = strtok(NULL, " ");
            char letter = token[0];
            bool found = false;
            bool won_lost = false;
            room_t *room = client.room;
            int points_before = client.points;
            for (int k = 0; k < strlen(room->room_hangman); k++) {
                if (room->room_hangman[k] == letter && client.player_hangman[k] == '_') {
                    found = true;
                    client.points++;
                    client.player_hangman[k] = letter;
                }
            }
            if (found) {
                if (strcmp(client.player_hangman, room->room_hangman) == 0) {
                    sprintf(buffer, "WON|%d|%s\n", room->place, client.nickname);
                    broadcast(buffer, room);
                    remove_client_from_room(&client, room);
                    reset_stats(&client);
                    room->place++;
                    won_lost = true;
                }
            } else {
                client.errors++;
                if (client.errors == MAX_INCORRECT_GUESSES){
                    sprintf(buffer, "LOST|%d|%s\n", room->clients_at_start - room->lost, client.nickname);
                    room->lost++;
                    broadcast(buffer, room);
                    remove_client_from_room(&client, room);
                    reset_stats(&client);
                    won_lost = true;
                }
            }
            if (room->num_clients == 1) {
                sprintf(buffer, "LOST|%d|%s\n", room->clients_at_start - room->lost, room->clients[0]->nickname);
                room->lost++;
                send_to_client(buffer, *(room->clients[0]));
                remove_client_from_room(room->clients[0], room);
                reset_stats(room->clients[0]);
                reset_room(room);
            }
            if (!won_lost){
                sprintf(buffer, "GUESS|%s|%d|%d\n", client.nickname, client.points, client.errors);
                broadcast(buffer, room);
            }
        } else if (strcmp(token, "LEAVE") == 0) {
            room_t *room = client.room;
            if (room != NULL) {
                if (!(room->in_game)){
                    bool was_king = false;
                    if (client.isking){
                        was_king = true;
                    }
                    sprintf(buffer, "LEFT|%s\n", client.nickname);
                    broadcast(buffer, room);
                    remove_client_from_room(&client, client.room);
                    reset_stats(&client);
                    if (room->num_clients == 0) {
                        reset_room(room);
                    } else {
                        if (was_king){
                            send_to_client("ISKING", *(room->clients[0]));
                        }
                    }
                } else {
                    sprintf(buffer, "LOST|%d|%s\n", room->clients_at_start - room->lost, client.nickname);
                    room->lost++;
                    broadcast(buffer, room);
                    remove_client_from_room(&client, room);
                    reset_stats(&client);
                    if (room->num_clients == 1) {
                        sprintf(buffer, "LOST|%d|%s\n", room->clients_at_start - room->lost, room->clients[0]->nickname);
                        room->lost++;
                        send_to_client(buffer, *(room->clients[0]));
                        std::cout << "1 Before: " << room->clients[0]->ready << std::endl;
                        reset_stats(room->clients[0]);
                        std::cout << "1 After: " << room->clients[0]->ready << std::endl;
                        remove_client_from_room(room->clients[0], room);
                        reset_room(room);
                    }
                }
            }
        } else {
            send_to_client("INVALIDCOMMAND\n", client);
        }
    } else {
        room_t *room = client.room;
        if (room != NULL) {
            if (!(room->in_game)){
                bool was_king = false;
                if (client.isking){
                    was_king = true;
                }
                remove_client_from_room(&client, client.room);
                reset_stats(&client);
                sprintf(buffer, "LEFT|%s\n", client.nickname);
                broadcast(buffer, room);
                if (room->num_clients == 0) {
                    reset_room(room);
                } else {
                    if (was_king){
                        room->clients[0]->isking = true;
                        send_to_client("ISKING", *(room->clients[0]));
                    }
                }
            } else {
                sprintf(buffer, "LOST|%d|%s\n", room->clients_at_start - room->lost, client.nickname);
                room->lost++;
                broadcast(buffer, room);
                reset_stats(&client);
                remove_client_from_room(&client, room);
                if (room->num_clients == 1) {
                    sprintf(buffer, "LOST|%d|%s\n", room->clients_at_start - room->lost, room->clients[0]->nickname);
                    room->lost++;
                    send_to_client(buffer, *(room->clients[0]));
                    std::cout << "2 Before: " << room->clients[0]->ready << std::endl;
                    reset_stats(room->clients[0]);
                    std::cout << "2 After: " << room->clients[0]->ready << std::endl;
                    remove_client_from_room(room->clients[0], room);
                    reset_room(room);
                }
            }
        }
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {

    if (argc != 2) {
        std::cerr << "Wymagany jest 1 argument (port)";
    }
    char* endp;
    long PORT = strtol(argv[1], &endp, 10);
    if (*endp || PORT > 65535 || PORT < 1){
        return 1;
    }

    signal(SIGINT, sig_handler);
    signal(SIGTSTP, sig_handler);


    for (int i = 0; i < MAX_ROOMS; i++) {
        room_t * room = (room_t *) malloc(sizeof(room_t));
        std::string room_name = "Pokoj" + std::to_string(i+1);
        strcpy(room->room_name, room_name.c_str());
        room->num_clients = 0;
        room->place = 1;
        room->lost = 0;
        rooms[num_rooms++] = room;
    }

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
                    int status = handle_client(client);
                    if (status == 1){
                        std::cerr << "Klient odlaczyl sie" << std::endl;
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client.sockfd, &events[i]);
                        clients.erase(client.sockfd);
                    }
                }
            }
        }
    }
    return 0;
}



