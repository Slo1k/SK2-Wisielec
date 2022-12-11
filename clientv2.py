import socket
import re
import threading

HOST = "127.0.0.1"
PORT = 8080  #

class Communication:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None
        self.setup_connection()

    def setup_connection(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))

    def send_data(self, data):
        self.sock.send(bytes(data, encoding="utf-8"))

    def read_data(self):
        return self.sock.recv(1024).decode('utf-8')

    def join(self, id):
        self.send_data("join|" + str(id) + "\r\n")

    def ready(self):
        self.send_data("ready")

    def leave(self):
        self.send_data("leave")

    def send_letter(self, letter):
        self.send_data("send|" + letter)

    def listen(self):
        while True:
            response = self.read_data()
            print(response)


class Game:

    def __init__(self, nick):
        self.nick = nick
        self.password = ""
        self.censored_password = ""
        self.used_letters = []
        self.ready = False
        self.in_game = False
        self.room = -1
        self.hp = -1
        self.score = -1

    def censor_password(self):
        self.censored_password = "".join(["_" for _ in range(len(self.password))])

    def guess_letter(self, letter):
        if (letter in self.password) and (letter not in self.used_letters):
            self.used_letters.append(letter)
            for i in range(len(self.password)):
                if self.password[i] == letter:
                    self.censored_password[i] = letter
            return True
        else:
            return False


if __name__ == "__main__":
    communication = Communication(HOST, PORT)

    nick = input("Wpisz swoj nick: ")
    game = Game(nick)

    thread = threading.Thread(target=communication.listen)
    thread.start()

    while True:
        if game.in_game:
            print(f"Haslo: {game.censored_password}")
            print(f"Wykorzystane literki: {','.join(game.used_letters)}")

        print("Wybierz odpowiednia komende:")
        if game.room < 0:
            print("join|<nr. pokoju>")
        if not game.ready:
            print("ready")
        if game.in_game:
            print("leave")
            print("guess|<litera>")
        command = input()

        if re.search("join\|[0-4]", command):
            communication.join(command[5])
        elif re.search("guess\|[a-zA-Z]", command):
            letter = command[5]
            if game.guess_letter(letter):
                communication.send_letter(command[5].upper())
            else:
                print(f"Już wykorzystałeś literę: {letter}!")
        elif command == "ready":
            communication.ready()
        elif command == "leave":
            communication.leave()
