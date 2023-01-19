import sys

from PyQt5.QtCore import pyqtSignal
from PyQt5.uic import loadUi
from PyQt5 import QtWidgets
from PyQt5.QtWidgets import QDialog, QApplication, QWidget, QStackedWidget, QButtonGroup
import socket
import threading


class Client:

    def __init__(self, host, port, pyqt_app):
        self.pyqt_app = pyqt_app
        self.host = host
        self.port = port
        self.nick = None
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



class NickScreen(QDialog):

    def __init__(self):
        super(NickScreen, self).__init__()
        loadUi("nick.ui", self)
        self.nickSubmitButton.clicked.connect(self.gotochooserooms)

    def gotochooserooms(self):
        nick = self.nickTextEdit.toPlainText()
        print(f"NICK {nick}")
        rooms = RoomsScreen()
        widget.addWidget(rooms)
        widget.setCurrentIndex(widget.currentIndex()+1)

    def handler(self, data):
        print("Otrzymano nowe dane")


class RoomsScreen(QDialog):
    def __init__(self):
        super(RoomsScreen, self).__init__()
        loadUi("room.ui", self)
        self.roomSubmitButton.clicked.connect(self.gotohangman)

    def gotohangman(self):
        print("JOIN|" + self.roomComboBox.currentText())
        hangman = HangmanScreen()
        widget.addWidget(hangman)
        widget.setCurrentIndex(widget.currentIndex()+1)


class HangmanScreen(QDialog):
    def __init__(self):
        super(HangmanScreen, self).__init__()
        loadUi("hangman.ui", self)
        self.letterButtons = QButtonGroup()
        self.letterButtons.setExclusive(True)
        self.letterButtons.addButton(self.hangmanButtonA)
        self.letterButtons.addButton(self.hangmanButtonB)
        self.letterButtons.addButton(self.hangmanButtonC)
        self.letterButtons.addButton(self.hangmanButtonD)
        self.letterButtons.addButton(self.hangmanButtonE)
        self.letterButtons.addButton(self.hangmanButtonF)
        self.letterButtons.addButton(self.hangmanButtonG)
        self.letterButtons.addButton(self.hangmanButtonH)
        self.letterButtons.addButton(self.hangmanButtonI)
        self.letterButtons.addButton(self.hangmanButtonJ)
        self.letterButtons.addButton(self.hangmanButtonK)
        self.letterButtons.addButton(self.hangmanButtonL)
        self.letterButtons.addButton(self.hangmanButtonM)
        self.letterButtons.addButton(self.hangmanButtonN)
        self.letterButtons.addButton(self.hangmanButtonO)
        self.letterButtons.addButton(self.hangmanButtonP)
        self.letterButtons.addButton(self.hangmanButtonQ)
        self.letterButtons.addButton(self.hangmanButtonR)
        self.letterButtons.addButton(self.hangmanButtonS)
        self.letterButtons.addButton(self.hangmanButtonT)
        self.letterButtons.addButton(self.hangmanButtonU)
        self.letterButtons.addButton(self.hangmanButtonV)
        self.letterButtons.addButton(self.hangmanButtonW)
        self.letterButtons.addButton(self.hangmanButtonX)
        self.letterButtons.addButton(self.hangmanButtonY)
        self.letterButtons.addButton(self.hangmanButtonZ)

        self.letterButtons.buttonClicked.connect(self.gotoletters)

        self.hangmanButtonLeave.clicked.connect(self.gotoleave)

    def gotoleave(self):
        print("LEAVE")
        widget.removeWidget(self)
        widget.setCurrentIndex(widget.currentIndex())

    def gotoletters(self, btn):
        print("GUESS|" + btn.text())


app = QApplication(sys.argv)
nick = NickScreen()
widget = QStackedWidget()
widget.addWidget(nick)
widget.setFixedHeight(800)
widget.setFixedWidth(1000)
widget.show()

# client = Client("localhost", 54321, app)


try:
    sys.exit(app.exec_())
except:
    print("LEAVE")
