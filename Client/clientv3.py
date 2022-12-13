import socket
import re
import threading

HOST = "127.0.0.1"
PORT = 54321

password = ""
censored_password = []
used_letters = []
ready = False
in_game = False
room = -1
hp = -1
score = -1


def censor_password(password):
    return ["_" for _ in range(len(password))]


def guess_letter(letter):
    if (letter in password) and (letter not in used_letters):
        used_letters.append(letter)
        for i in range(len(password)):
            if password[i] == letter:
                censored_password[i] = letter
        return True
    else:
        return False


def send_data(sock, data):
    sock.sendall(bytes(data, encoding='utf-8'))


def read_data(sock):
    buffer = ""
    char = sock.recv(1).decode('utf-8')
    while (char != "#"):
        buffer += char
        char = sock.recv(1).decode('utf-8')
    return buffer


def listen(sock):
    global password
    global hp
    global score
    while True:
        command = ""
        buf = read_data(sock)
        for i in range(len(buf)):
            if buf[i] == "#":
                break
            else:
                command += buf[i]

        if (command == "LEFT"):
            print("LEFT OK")
            room = -1
        elif (command == "READY"):
            print("READY OK")
            ready = True
        elif (re.search("ROOM\|[0-4]", command)):
            print("JOINED ROOM", command[5])
            room = int(command[5])
        elif (command == "ERROR"):
            print("ERROR")
        else:
            data = command.split("|")
            print(data)
            password = data[0]
            hp = int(data[1])
            score = int(data[2])
            in_game = True


if __name__ == "__main__":
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOST, PORT))

    thread = threading.Thread(target=listen, args=(sock,))
    thread.start()

    while True:
        if in_game:
            print(f"Haslo: {censored_password}")
            print(f"Wykorzystane literki: {','.join(used_letters)}")

        print("Wybierz odpowiednia komende:", password, censored_password)
        #if room < 0:
        #    print("join|<nr. pokoju>")
        #if not ready and room > 0:
        #    print("ready")
        #if in_game:
        #    print("leave")
        #    print("guess|<litera>")

        command = input()

        if re.search("join\|[0-4]", command):
            send_data(sock, "JOIN|"+command[5]+"#")
            res = read_data(sock)
            room = int(res[5])
        elif re.search("guess\|[a-zA-Z]", command):
            letter = command[6]
            if guess_letter(letter):
                send_data(sock, "GUESS|" + letter + "#")
            else:
                print(f"Już wykorzystałeś literę: {letter}!")
        elif command == "ready":
            send_data(sock, "READY#")
        elif command == "leave":
            send_data(sock, "LEAVE#")
