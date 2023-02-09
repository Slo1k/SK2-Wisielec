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
is_king = False
in_game = False
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
                    print("FROM SERVER:", mess.strip())
                    self.new_message.emit(mess.strip())


class NickScreen(QDialog):
    def __init__(self):
        super(NickScreen, self).__init__()
        loadUi("nick.ui", self)
        self.nickSubmitButton.clicked.connect(self.set_nick)

    def set_nick(self):
        global my_nick
        my_nick = self.nickTextEdit.toPlainText()
        if not my_nick:
            self.nickErrorLabel.setText(f"Nick nie może być pusty!")
        else:
            if len(my_nick) > 15:
                self.nickErrorLabel.setText(f"Nick nie może być dłuższy niż 15 znaków!")
            else:
                message = "SETNICK " + my_nick.strip()
                send_message(message)
                try:
                    status = client_socket.recv(1024).decode("utf-8").strip()
                    if len(status) > 0:
                        if status == "NICKTAKEN":
                            self.nickErrorLabel.setText(f"Nick: [{my_nick}] jest już zajęty!")
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
        global is_king
        global in_room
        global players_count
        global current_players
        chosen_room = self.roomComboBox.currentText().split("|")[0]
        send_message("JOIN " + chosen_room)
        status = client_socket.recv(1024).decode("utf-8").strip().split()
        for stat in status:
            if stat == "ROOMFULL":
                msg = QMessageBox()
                msg.setWindowTitle("Błąd!")
                msg.setText("Pokój jest pełny lub gra już się rozpoczęła!")
                msg.exec_()
                self.update_combobox()
                break
            if stat == "JOINED|" + my_nick:
                in_room = True
                current_players[my_nick] = [0, 0, 0]
            if stat.startswith("CURRENTPLAYERS"):
                for player in stat.split("|")[1:-1]:
                    players_count += 1
                    current_players[player] = [players_count, 0, 0]
            if stat.startswith("ISKING"):
                is_king = True
        if in_room:
            hangman = HangmanScreen()
            widget.addWidget(hangman)
            widget.setCurrentIndex(widget.currentIndex() + 1)


class HangmanScreen(QDialog):
    def __init__(self):
        super(HangmanScreen, self).__init__()
        loadUi("hangman.ui", self)
        self.server_thread = ServerThread()
        self.server_thread.new_message.connect(self.update_screen)
        self.server_thread.start()
        self.letterButtons = QButtonGroup()
        self.letterButtons.setExclusive(True)
        self.display_start()
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
        self.hangmanPictures = [
            self.hangmanLabelMyHangman,
            self.hangmanLabelPlayer1Hang,
            self.hangmanLabelPlayer2Hang,
            self.hangmanLabelPlayer3Hang,
            self.hangmanLabelPlayer4Hang,
            self.hangmanLabelPlayer5Hang,
            self.hangmanLabelPlayer6Hang,
            self.hangmanLabelPlayer7Hang,
            self.hangmanLabelPlayer8Hang,
        ]
        self.hangmanScores = [
            self.hangmanLabelMyScore,
            self.hangmanLabelPlayer1,
            self.hangmanLabelPlayer2,
            self.hangmanLabelPlayer3,
            self.hangmanLabelPlayer4,
            self.hangmanLabelPlayer5,
            self.hangmanLabelPlayer6,
            self.hangmanLabelPlayer7,
            self.hangmanLabelPlayer8,
        ]
        self.hangmanResults = [
            self.hangmanLabelMyResult,
            self.hangmanLabelResult1,
            self.hangmanLabelResult2,
            self.hangmanLabelResult3,
            self.hangmanLabelResult4,
            self.hangmanLabelResult5,
            self.hangmanLabelResult6,
            self.hangmanLabelResult7,
            self.hangmanLabelResult8
        ]
        self.disable_letter_buttons()
        self.setup_current_players()
        self.letterButtons.buttonClicked.connect(self.guess_letter)
        self.hangmanButtonStart.clicked.connect(self.start_game)
        self.hangmanButtonAccept.clicked.connect(self.accept_game)

    def update_screen(self, message):
        global hangman_password
        global hidden_hangman_password
        global players_count
        if message.startswith("JOINED"):
            players_count += 1
            player_nick = message.split("|")[1]
            current_players[player_nick] = [players_count, 0, 0]
            self.hangmanScores[players_count].setText(f"{player_nick}: 0")
            self.hangmanScores[players_count].setVisible(True)
            self.hangmanPictures[players_count].setPixmap(QtGui.QPixmap(f"images/hangman0.png"))
            self.hangmanPictures[players_count].setVisible(True)
        elif message.startswith("CURRENTPLAYERS"):
            for player in message.split("|")[1:-1]:
                players_count += 1
                current_players[player] = [players_count, 0, 0]
            self.setup_current_players()
        elif message.startswith("ATLEAST2PLAYERS"):
            msg = QMessageBox()
            msg.setWindowTitle("Błąd!")
            msg.setText("Do rozpoczęcia rozgrywki potrzeba minimum 2 graczy!")
            msg.exec_()
        elif message.startswith("LEFT"):
            player = message.split("|")[1]
            player_ind, _, _ = current_players[player]
            if in_game:
                self.hangmanPictures[player_ind].setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
            else:
                self.update_current_players(player)
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
                    self.hangmanLabelMyScore.setVisible(False)
                    self.hangmanLabelMyScore.setText(f"Twój wynik:\n{points}")
                    self.hangmanLabelMyScore.setVisible(True)
                    current_players[who] = [points, errors]
                else:
                    self.hangmanLabelMyHangman.setVisible(False)
                    self.hangmanLabelMyHangman.setPixmap(QtGui.QPixmap(f"images/hangman{errors}.png"))
                    self.hangmanLabelMyHangman.setVisible(True)
            else:
                player_ind, player_points, player_errors = current_players[who]
                self.hangmanScores[player_ind].setText(f"{who}: {points}")
                self.hangmanPictures[player_ind].setPixmap(QtGui.QPixmap(f"images/hangman{errors}.png"))
                current_players[who] = [player_ind, points, errors]
        elif message.startswith("START"):
            self.display_accept()
        elif message.startswith("HANGMANSTART"):
            hangman_length = int(len(message.split("|")[1]))
            hangman_password = message.split("|")[1]
            hidden_hangman_password = ["_" for _ in range(hangman_length)]
            self.display_hangman_hidden()
            self.enable_letter_buttons()
        elif message.startswith("LOST") or message.startswith("WON"):
            won_lost, place, who = message.split("|")
            if who == my_nick and won_lost == "LOST":
                self.hangmanLabelMyHangman.setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
                self.hangmanResults[0].setText(f"Miejse {place}")
                self.hangmanResults[0].setStyleSheet("color: red")
                self.hangmanResults[0].setVisible(True)
                msg = QMessageBox()
                msg.setWindowTitle("Przegrałeś!")
                msg.setText(f"Przegrałeś i zająłeś {place} miejsce!")
                msg.exec_()
                self.reset_player_stats()
                self.leave()
            elif who == my_nick and won_lost == "WON":
                self.hangmanResults[0].setText(f"Miejse {place}")
                self.hangmanResults[0].setStyleSheet("color: green")
                self.hangmanResults[0].setVisible(True)
                msg = QMessageBox()
                msg.setWindowTitle("Wygraleś!")
                msg.setText(f"Wygrałeś i zająłeś {place} miejsce!")
                msg.exec_()
                self.reset_player_stats()
                self.leave()
            elif won_lost == "LOST":
                player_ind, _, _ = current_players[who]
                self.hangmanPictures[player_ind].setPixmap(QtGui.QPixmap(f"images/hangman6.png"))
                self.hangmanResults[player_ind].setText(f"Miejse {place}")
                self.hangmanResults[player_ind].setStyleSheet("color: red")
                self.hangmanResults[player_ind].setVisible(True)
                del current_players[who]
            elif won_lost == "WON":
                player_ind, _, _ = current_players[who]
                self.hangmanResults[player_ind].setText(f"Miejse {place}")
                self.hangmanResults[player_ind].setStyleSheet("color: green")
                self.hangmanResults[player_ind].setVisible(True)
                del current_players[who]
        elif message.startswith("ISKING"):
            self.hangmanButtonStart.setVisible(True)

        self.display_start()
        self.update()
        self.repaint()

    def setup_current_players(self):
        global current_players
        print("USTAWIENIA!")
        for key, value in current_players.items():
            print(f"KEY: {key} VALUE: {value}")
            self.hangmanScores[value[0]].setText(f"{key}: 0")
            self.hangmanScores[value[0]].setVisible(True)
            self.hangmanPictures[value[0]].setVisible(True)

    def update_current_players(self, player):
        global players_count
        player_ind, _, _ = current_players[player]
        for key, value in current_players.items():
            self.hangmanScores[value[0]].setVisible(False)
            self.hangmanPictures[value[0]].setVisible(False)
        del current_players[player]
        players_count -= 1
        for key, value in current_players.items():
            if value[0] > player_ind:
                current_players[key] = [value[0]-1, value[1], value[2]]
        self.setup_current_players()

    def display_accept(self):
        if self.hangmanButtonStart.isVisible():
            self.hangmanButtonStart.setVisible(False)
        self.hangmanButtonAccept.setVisible(True)

    def display_hangman_hidden(self):
        self.hangmanLabelPassword.setVisible(False)
        self.hangmanLabelPassword.setText(" ".join(hidden_hangman_password))
        self.hangmanLabelPassword.setVisible(True)

    def accept_game(self):
        send_message("ACCEPT")
        self.hangmanButtonAccept.setDisabled(True)

    def display_start(self):
        if is_king:
            self.hangmanButtonStart.setVisible(True)

    def start_game(self):
        send_message("START")

    def reset_player_stats(self):
        global hangman_password
        global hidden_hangman_password
        global players_count
        global in_game
        global in_room
        global is_king

        players_count = 0
        hangman_password = ""
        hidden_hangman_password = []
        in_game = False
        in_room = False
        is_king = False

    def leave(self):
        global in_room
        global players_count
        current_players.clear()
        players_count = 0
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
        pass