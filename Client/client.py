import sys
from PyQt5 import QtGui
from PyQt5.uic import loadUi
from PyQt5.QtCore import QThread, pyqtSignal
from PyQt5.QtWidgets import QDialog, QApplication, QWidget, QStackedWidget, QButtonGroup, QMessageBox
import socket

current_players = {}
players_count = 0
my_nick = None
in_room = False
hangman_password = ""
hidden_hangman_password = []
sent_letter = ""


def send_message(message):
    try:
        client_socket.send(bytes(message, "utf-8"))
    except:
        print("Zerwano połączenie z serwerem")
        sys.exit()


class ServerThread(QThread):
    new_message = pyqtSignal(str)

    def __init__(self):
        super().__init__()

    def run(self):
        while True:
            message = client_socket.recv(1024).decode("utf-8").strip().split("\n")
            if len(message) >= 1:
                for mess in message:
                    self.new_message.emit(mess.strip())


class NickScreen(QDialog):
    def __init__(self):
        super(NickScreen, self).__init__()
        loadUi("nick.ui", self)
        self.nickSubmitButton.clicked.connect(self.set_nick)

    def set_nick(self):
        global my_nick
        my_nick = self.nickTextEdit.toPlainText()
        message = "SETNICK " + my_nick
        send_message(message)
        try:
            status = client_socket.recv(1024).decode("utf-8").strip()
            if len(status) > 0:
                if status == "NICKTAKEN":
                    self.nickErrorLabel.setText(f"Nick: [{my_nick}] jest już zajęty!")
                elif status == "NICKEMPTY":
                    self.nickErrorLabel.setText(f"Nick nie może być pusty!")
                else:
                    rooms = RoomsScreen()
                    widget.addWidget(rooms)
                    widget.setCurrentIndex(widget.currentIndex() + 1)
        except:
            print("Błąd przy otrzymywaniu odpowiedzi od serwera")
            client_socket.close()
            sys.exit()


class RoomsScreen(QDialog):
    def __init__(self):
        super(RoomsScreen, self).__init__()
        loadUi("room.ui", self)
        self.update_combobox()
        self.roomSubmitButton.clicked.connect(self.join_room)

    def update_combobox(self):
        self.roomComboBox.clear()
        send_message("ROOMS")
        rooms = client_socket.recv(1024).decode("utf-8").strip().split()[1:]
        for room in rooms:
            self.roomComboBox.addItem(room[5:])

    def join_room(self):
        global in_room
        global players_count
        chosen_room = self.roomComboBox.currentText().split("|")[0]
        send_message("JOIN " + chosen_room)
        status = client_socket.recv(1024).decode("utf-8").strip().split()
        for stat in status:
            if stat == "ROOMFULL":
                msg = QMessageBox()
                msg.setWindowTitle("Błąd!")
                msg.setText("Pokój jest pełny lub gra już się rozpoczęła")
                msg.exec_()
                self.update_combobox()
                break
            elif stat == "JOINED|" + my_nick:
                in_room = True
                current_players[my_nick] = [0, 0, 0]
            elif stat.startswith("CURRENTPLAYERS"):
                for player in stat.split("|")[1:-1]:
                    players_count += 1
                    current_players[player] = [players_count, 0, 0]
        if in_room:
            hangman = HangmanScreen()
            widget.addWidget(hangman)
            widget.setCurrentIndex(widget.currentIndex() + 1)


class HangmanScreen(QDialog):
    def __init__(self):
        super(HangmanScreen, self).__init__()
        loadUi("hangman.ui", self)
        self.is_king()
        self.server_thread = ServerThread()
        self.server_thread.new_message.connect(self.update_screen)
        self.server_thread.start()
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
        self.disable_letter_buttons()
        self.setup_current_players()

        self.letterButtons.buttonClicked.connect(self.guess_letter)
        self.hangmanButtonStart.clicked.connect(self.start_game)
        self.hangmanButtonAccept.clicked.connect(self.accept_game)
        self.hangmanButtonLeave.clicked.connect(self.leave)

    def update_screen(self, message):
        global hangman_password
        global hidden_hangman_password
        global players_count
        if message.startswith("JOINED"):
            players_count += 1
            player_nick = message.split("|")[1]
            current_players[player_nick] = [players_count, 0, 0]
            if players_count == 1:
                self.hangmanLabelPlayer1.setText(f"{player_nick}: 0")
                self.hangmanLabelPlayer1.setVisible(True)
                self.hangmanLabelPlayer1Hang.setVisible(True)
            elif players_count == 2:
                self.hangmanLabelPlayer2.setText(f"{player_nick}: 0")
                self.hangmanLabelPlayer2.setVisible(True)
                self.hangmanLabelPlayer2Hang.setVisible(True)
            elif players_count == 3:
                self.hangmanLabelPlayer3.setText(f"{player_nick}: 0")
                self.hangmanLabelPlayer3.setVisible(True)
                self.hangmanLabelPlayer3Hang.setVisible(True)
            elif players_count == 4:
                self.hangmanLabelPlayer4.setText(f"{player_nick}: 0")
                self.hangmanLabelPlayer4.setVisible(True)
                self.hangmanLabelPlayer4Hang.setVisible(True)
            elif players_count == 5:
                self.hangmanLabelPlayer5.setText(f"{player_nick}: 0")
                self.hangmanLabelPlayer5.setVisible(True)
                self.hangmanLabelPlayer5Hang.setVisible(True)
            elif players_count == 6:
                self.hangmanLabelPlayer6.setText(f"{player_nick}: 0")
                self.hangmanLabelPlayer6.setVisible(True)
                self.hangmanLabelPlayer6Hang.setVisible(True)
            elif players_count == 7:
                self.hangmanLabelPlayer7.setText(f"{player_nick}: 0")
                self.hangmanLabelPlayer7.setVisible(True)
                self.hangmanLabelPlayer7Hang.setVisible(True)
            elif players_count == 8:
                self.hangmanLabelPlayer8.setText(f"{player_nick}: 0")
                self.hangmanLabelPlayer8.setVisible(True)
                self.hangmanLabelPlayer8Hang.setVisible(True)
        elif message.startswith("ATLEAST2PLAYERS"):
            msg = QMessageBox()
            msg.setWindowTitle("Błąd!")
            msg.setText("Do rozpoczęcia rozgrywki potrzeba minimum 2 graczy!")
            msg.exec_()
        elif message.startswith("LEFT"):
            player = message.split("|")[1]
            player_ind, _, _ = current_players[player]
            if player_ind == 1:
                self.hangmanLabelPlayer1Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
            elif player_ind == 2:
                self.hangmanLabelPlayer2Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
            elif player_ind == 3:
                self.hangmanLabelPlayer3Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
            elif player_ind == 4:
                self.hangmanLabelPlayer4Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
            elif player_ind == 5:
                self.hangmanLabelPlayer5Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
            elif player_ind == 6:
                self.hangmanLabelPlayer6Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
            elif player_ind == 7:
                self.hangmanLabelPlayer7Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
            elif player_ind == 8:
                self.hangmanLabelPlayer8Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
        elif message.startswith("GUESS"):
            _, who, points, errors = message.split("|")
            points = int(points)
            errors = int(errors)
            current_points = current_players[who][1]
            if my_nick == who:
                if current_points < points:
                    for i in range(len(hangman_password)):
                        if hangman_password[i] == sent_letter:
                            hidden_hangman_password[i] = sent_letter
                    self.display_hangman_hidden()
                    self.hangmanLabelMyScore.setText(f"Twój wynik:\n{points}")
                    current_players[who] = [points, errors]
                else:
                    self.hangmanLabelMyHangman.setPixmap(QtGui.QPixmap(f"images/hangman{errors}.png"))
            else:
                player_ind, player_points, player_errors = current_players[who]
                if player_ind == 1:
                    self.hangmanLabelPlayer1.setText(f"{who}: {points}")
                    self.hangmanLabelPlayer1Hang.setPixmap(QtGui.QPixmap(f"images/hangman{errors}.png"))
                elif player_ind == 2:
                    self.hangmanLabelPlayer2.setText(f"{who}: {points}")
                    self.hangmanLabelPlayer2Hang.setPixmap(QtGui.QPixmap(f"images/hangman{errors}.png"))
                elif player_ind == 3:
                    self.hangmanLabelPlayer3.setText(f"{who}: {points}")
                    self.hangmanLabelPlayer3Hang.setPixmap(QtGui.QPixmap(f"images/hangman{errors}.png"))
                elif player_ind == 4:
                     self.hangmanLabelPlayer4.setText(f"{who}: {points}")
                     self.hangmanLabelPlayer4Hang.setPixmap(QtGui.QPixmap(f"images/hangman{errors}.png"))
                elif player_ind == 5:
                    self.hangmanLabelPlayer5.setText(f"{who}: {points}")
                    self.hangmanLabelPlayer5Hang.setPixmap(QtGui.QPixmap(f"images/hangman{errors}.png"))
                elif player_ind == 6:
                    self.hangmanLabelPlayer6.setText(f"{who}: {points}")
                    self.hangmanLabelPlayer6Hang.setPixmap(QtGui.QPixmap(f"images/hangman{errors}.png"))
                elif player_ind == 7:
                    self.hangmanLabelPlayer7.setText(f"{who}: {points}")
                    self.hangmanLabelPlayer7Hang.setPixmap(QtGui.QPixmap(f"images/hangman{errors}.png"))
                elif player_ind == 8:
                    self.hangmanLabelPlayer8.setText(f"{who}: {points}")
                    self.hangmanLabelPlayer8Hang.setPixmap(QtGui.QPixmap(f"images/hangman{errors}.png"))
                current_players[who] = [player_ind, points, errors]
        elif message.startswith("START"):
            self.display_accept()
        elif message.startswith("HANGMANSTART"):
            self.hangmanButtonLeave.setDisabled(False)
            hangman_length = int(len(message.split("|")[1]))
            hangman_password = message.split("|")[1]
            hidden_hangman_password = ["_" for _ in range(hangman_length)]
            self.display_hangman_hidden()
            self.enable_letter_buttons()
        elif message.startswith("LOST") or message.startswith("WON"):
            won_lost, place, who = message.split("|")
            if who == my_nick and won_lost == "LOST":
                msg = QMessageBox()
                msg.setWindowTitle("Przegrałeś!")
                msg.setText(f"Przegrałeś i zająłeś {place} miejsce!")
                msg.exec_()
                self.leave()
            elif who == my_nick and won_lost == "WON":
                msg = QMessageBox()
                msg.setWindowTitle("Wygraleś!")
                msg.setText(f"Wygrałeś i zająłeś {place} miejsce!")
                msg.exec_()
                self.leave()
            elif won_lost == "LOST":
                player_ind, _, _ = current_players[who]
                if player_ind == 1:
                    self.hangmanLabelPlayer1Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
                elif player_ind == 2:
                    self.hangmanLabelPlayer2Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
                elif player_ind == 3:
                    self.hangmanLabelPlayer3Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
                elif player_ind == 4:
                     self.hangmanLabelPlayer4Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
                elif player_ind == 5:
                    self.hangmanLabelPlayer5Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
                elif player_ind == 6:
                    self.hangmanLabelPlayer6Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
                elif player_ind == 7:
                    self.hangmanLabelPlayer7Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
                elif player_ind == 8:
                    self.hangmanLabelPlayer8Hang.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
        self.is_king()
        self.repaint()

    def setup_current_players(self):
        for key, value in current_players.items():
            if value[0] == 1:
                self.hangmanLabelPlayer1.setText(f"{key}: 0")
                self.hangmanLabelPlayer1.setVisible(True)
                self.hangmanLabelPlayer1Hang.setVisible(True)
            elif value[0] == 2:
                self.hangmanLabelPlayer2.setText(f"{key}: 0")
                self.hangmanLabelPlayer2.setVisible(True)
                self.hangmanLabelPlayer2Hang.setVisible(True)
            elif value[0] == 3:
                self.hangmanLabelPlayer3.setText(f"{key}: 0")
                self.hangmanLabelPlayer3.setVisible(True)
                self.hangmanLabelPlayer3Hang.setVisible(True)
            elif value[0] == 4:
                self.hangmanLabelPlayer4.setText(f"{key}: 0")
                self.hangmanLabelPlayer4.setVisible(True)
                self.hangmanLabelPlayer4Hang.setVisible(True)
            elif value[0] == 5:
                self.hangmanLabelPlayer5.setText(f"{key}: 0")
                self.hangmanLabelPlayer5.setVisible(True)
                self.hangmanLabelPlayer5Hang.setVisible(True)
            elif value[0] == 6:
                self.hangmanLabelPlayer6.setText(f"{key}: 0")
                self.hangmanLabelPlayer6.setVisible(True)
                self.hangmanLabelPlayer6Hang.setVisible(True)
            elif value[0] == 7:
                self.hangmanLabelPlayer7.setText(f"{key}: 0")
                self.hangmanLabelPlayer7.setVisible(True)
                self.hangmanLabelPlayer7Hang.setVisible(True)
            elif value[0] == 8:
                self.hangmanLabelPlayer8.setText(f"{key}: 0")
                self.hangmanLabelPlayer8.setVisible(True)
                self.hangmanLabelPlayer8Hang.setVisible(True)


    def display_accept(self):
        if self.hangmanButtonStart.isVisible():
            self.hangmanButtonStart.setVisible(False)
        self.hangmanButtonAccept.setVisible(True)
        self.hangmanButtonLeave.setDisabled(True)

    def display_hangman_hidden(self):
        self.hangmanLabelPassword.setText(" ".join(hidden_hangman_password))
        self.hangmanLabelPassword.setVisible(True)

    def accept_game(self):
        send_message("ACCEPT")
        self.hangmanButtonAccept.setDisabled(True)

    def is_king(self):
        if len(current_players) == 1:
            self.hangmanButtonStart.setVisible(True)

    def start_game(self):
        send_message("START")

    def leave(self):
        global in_room
        current_players.clear()
        in_room = False
        send_message("LEAVE")
        self.server_thread.terminate()
        widget.removeWidget(self)
        widget.setCurrentIndex(widget.currentIndex())

    def disable_letter_buttons(self):
        for button in self.letterButtons.buttons():
            button.setDisabled(True)

    def enable_letter_buttons(self):
        for button in self.letterButtons.buttons():
            button.setDisabled(False)

    def guess_letter(self, btn):
        global sent_letter
        sent_letter = btn.text()
        send_message("GUESS " + btn.text())
        btn.setDisabled(True)


if __name__ == "__main__":
    HOST = sys.argv[1]
    PORT = int(sys.argv[2])

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((HOST, PORT))

    app = QApplication(sys.argv)
    nick = NickScreen()
    widget = QStackedWidget()
    widget.addWidget(nick)
    widget.setFixedHeight(800)
    widget.setFixedWidth(1000)
    widget.show()

    try:
        sys.exit(app.exec_())
    except:
        send_message("LEAVE")