import socket
import threading
import sys


class Client:

    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.nickname = None
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.connect((self.host, self.port))
        receive_thread = threading.Thread(target=self.receive_messages)
        receive_thread.start()

    def receive_messages(self):
        while True:
            try:
                message = self.client_socket.recv(1024).decode("utf-8")
                if len(message) > 0:
                    print(message)
            except:
                print("An error occurred while receiving a message.")
                self.client_socket.close()
                sys.exit()

    def send_message(self, message):
        self.client_socket.send(bytes(message, "utf-8"))

    def send_command(self, command):
        self.send_message(command)


client = Client("localhost", 54321)

while True:
    message = input()
    client.send_command(message)
