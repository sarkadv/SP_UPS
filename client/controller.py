import select
import socket
import time
from tkinter import messagebox

from login_frame import LoginFrame
from enqueue_frame import EnqueueFrame
from game_frame import GameFrame
from inqueue_frame import InqueueFrame
from playground import *
from constants import *
from message import MessageDecoder, MessageEncoder


# trida pro zpracovavani zprav ze serveru
class Controller(tk.Tk):
    exitFlag = False
    client_socket = None
    connected = False
    server_available = False
    last_ping_time = None
    ip = None
    port = None
    nickname = None
    tried_connect = False
    tried_connect_time = None

    def __init__(self):
        tk.Tk.__init__(self)
        self.title("Dots & Boxes")
        self.geometry(DEFAULT_WINDOW_SIZE)
        self.minsize(MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT)
        self.maxsize(MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT)
        self.configure(background=BACKGROUND_COLOR)
        self.protocol("WM_DELETE_WINDOW", self.on_closing)

        container = tk.Frame(self)
        container.pack(side="top", fill="both", expand=True)
        container.grid_rowconfigure(0, weight=1)
        container.grid_columnconfigure(0, weight=1)

        self.frames = {}
        self.current_frame = None
        self.frames["LoginFrame"] = LoginFrame(parent=container, controller=self)
        self.frames["EnqueueFrame"] = EnqueueFrame(parent=container, controller=self)
        self.frames["InqueueFrame"] = InqueueFrame(parent=container, controller=self)
        self.frames["GameFrame"] = GameFrame(parent=container, controller=self)

        self.frames["LoginFrame"].grid(row=0, column=0, sticky="nsew")
        self.frames["EnqueueFrame"].grid(row=0, column=0, sticky="nsew")
        self.frames["InqueueFrame"].grid(row=0, column=0, sticky="nsew")
        self.frames["GameFrame"].grid(row=0, column=0, sticky="nsew")

        self.show_frame("LoginFrame")

    # da zvoleny ramec do popredi tak, aby byl viditelny
    def show_frame(self, page_name):
        self.current_frame = self.frames[page_name]
        frame = self.frames[page_name]
        frame.tkraise()

    def update_gui(self):
        if not self.exitFlag:
            self.update()
            self.current_frame.update_frame()

    def on_closing(self):
        self.exitFlag = True

    # podle prikazu ve zprave vybere a zavola odpovidajici metodu
    def run_command(self, message: MessageDecoder):
        if message.command == PING:
            self.command_pong(message.parameters)
        elif message.command == LOGGED:
            self.command_logged(message.parameters)
        elif message.command == QUEUED:
            self.command_queued(message.parameters)
        elif message.command == START_GAME:
            self.command_start_game(message.parameters)
        elif message.command == PLAY:
            self.command_play(message.parameters)
        elif message.command == DRAW_COORDS:
            self.command_draw_coords(message.parameters)
        elif message.command == DRAW_SQUARES:
            self.command_draw_squares(message.parameters)
        elif message.command == WINNER:
            self.command_winner(message.parameters)
        elif message.command == YOU_LEFT_GAME:
            self.command_you_left_game(message.parameters)
        elif message.command == OPPONENT_LEFT_GAME:
            self.command_opponent_left_game(message.parameters)
        elif message.command == LEFT_QUEUE:
            self.command_left_queue(message.parameters)
        elif message.command == SERVER_SHUTDOWN:
            self.command_server_shutdown()
        elif message.command == OPPONENT_CONNECTION:
            self.command_opponent_connection(message.parameters)
        elif message.command == RESUME_GAME:
            self.command_resume_game(message.parameters)
        else:
            self.wrong_command()

    # pokusi se serveru poslat zpravu
    # pokud se to nepodari, tlacitko btn_to_unblock bude odblokovano
    def try_send_message(self, command, parameters, btn_to_unblock=None):
        encoded = MessageEncoder(command, parameters)
        print("sending message: " + encoded.get_raw_message() + " ... ", end="")
        try:
            self.client_socket.send(encoded.message.encode("utf-8"))
            print(OK_STRING)
        except socket.error:
            if btn_to_unblock is not None:
                btn_to_unblock["state"] = "normal"
            print(ERROR_STRING)
            messagebox.showerror("Error", "Could not send message to the server.")

    # odesle serveru odpoved na zpravu PING
    def command_pong(self, parameters):
        if len(parameters) != 0:
            self.wrong_command()
            return

        self.server_available = True
        self.try_send_message(PONG, [])

    # zpracuje odpoved od serveru, zda se uzivateli podarilo prihlasit
    # pokud ano, zmeni aktivni ramec na EnqueueFrame
    def command_logged(self, parameters):
        if len(parameters) != 1:
            self.wrong_command()
            return

        result = parameters[0]

        # nepodarilo se prihlasit
        if result == ERROR_STRING:
            self.connected = False
            self.server_available = False
            self.client_socket.close()
            self.client_socket = None
            self.show_frame("LoginFrame")
            messagebox.showerror("Error", "Could not log in.")

        # v poradku
        elif result == OK_STRING:
            self.show_frame("EnqueueFrame")
            self.frames["EnqueueFrame"].nickname_label.configure(text="Nickname: "
                                                                      + self.frames["LoginFrame"].nickname_text.get())
        else:
            self.wrong_command()

        self.unblock_all_buttons()

    # zpracuje odpoved od serveru, zda se uzivateli podarilo zaradit do fronty
    # pokud ano, zmeni aktivni ramec na InqueueFrame
    def command_queued(self, parameters):
        if not (self.current_frame == self.frames["EnqueueFrame"] or self.current_frame == self.frames["GameFrame"]):
            self.wrong_command()
            return

        if len(parameters) != 1:
            self.wrong_command()
            return

        result = parameters[0]

        if result == ERROR_STRING:
            messagebox.showerror("Error", "Could not queue.")

        elif result == OK_STRING:
            self.show_frame("InqueueFrame")
            self.frames["InqueueFrame"].nickname_label.configure(text="Nickname: "
                                                                      + self.frames["LoginFrame"].nickname_text.get())
        else:
            self.wrong_command()

        self.frames["EnqueueFrame"].button_queue["state"] = "normal"

    # zpracuje zpravu od serveru o startu hry
    # pokud je zprava v poradku, zmeni se aktivni ramec na GameFrame
    def command_start_game(self, parameters):
        if not (self.current_frame == self.frames["EnqueueFrame"] or self.current_frame == self.frames["InqueueFrame"]):
            self.wrong_command()
            return

        if len(parameters) != 2:
            self.wrong_command()
            return

        my_number = parameters[0]
        opponent_name = parameters[1]

        if not my_number.isnumeric():
            self.wrong_command()
            return

        my_number = int(my_number)

        if not (my_number == 1 or my_number == 2):
            self.wrong_command()
            return

        opponent_name = opponent_name.replace(" ", "")

        if len(opponent_name) < 1:
            self.wrong_command()
            return

        if len(opponent_name) > NAME_LENGTH:
            self.wrong_command()
            return

        self.frames["GameFrame"].my_name = self.frames["LoginFrame"].nickname_text.get()
        self.frames["GameFrame"].my_name_label.config(text="Me: " + str(self.frames["GameFrame"].my_name))
        self.frames["GameFrame"].my_number = my_number
        self.frames["GameFrame"].playground.my_number = my_number
        self.frames["GameFrame"].opponent_name = opponent_name
        self.frames["GameFrame"].opponent_name_label.config(text="Opponent: "
                                                                 + str(self.frames["GameFrame"].opponent_name))
        self.show_frame("GameFrame")

    # zpracuje zpravu od serveru o prave aktivnim hraci
    def command_play(self, parameters):
        if not self.current_frame == self.frames["GameFrame"]:
            self.wrong_command()
            return

        if len(parameters) != 1:
            self.wrong_command()
            return

        active_player_number = parameters[0]

        if not active_player_number.isnumeric():
            self.wrong_command()
            return

        active_player_number = int(active_player_number)

        if not (active_player_number == 1 or active_player_number == 2):
            self.wrong_command()
            return

        self.frames["GameFrame"].playground.active_player = active_player_number

    # zpracuje zpravu od serveru o nove usecce mezi tlacitky
    # pokud je zprava v poradku, do grafu propojenych tlacitek je pridana nova hrana
    def command_draw_coords(self, parameters):
        if not self.current_frame == self.frames["GameFrame"]:
            self.wrong_command()
            return

        if len(parameters) != 2:
            self.wrong_command()
            return

        coord1 = parameters[0]
        coord2 = parameters[1]

        if not coord1.isnumeric():
            self.wrong_command()
            return

        if not coord2.isnumeric():
            self.wrong_command()
            return

        coord1 = int(coord1)
        coord2 = int(coord2)

        if coord1 < 0 or coord1 > DOTS * DOTS - 1:
            self.wrong_command()
            return

        if coord2 < 0 or coord2 > DOTS * DOTS - 1:
            self.wrong_command()
            return

        if not (abs(coord1 - coord2) == 1 or abs(coord1 - coord2) == DOTS):
            self.wrong_command()
            return

        x1 = coord1 // DOTS
        y1 = coord1 % DOTS
        x2 = coord2 // DOTS
        y2 = coord2 % DOTS

        vertex1 = y1 * DOTS + x1
        vertex2 = y2 * DOTS + x2

        # uz propojene
        if self.frames["GameFrame"].playground.graph.is_neighbor(vertex1, vertex2):
            self.wrong_command()
            return

        self.frames["GameFrame"].playground.graph.add_edge(vertex1, vertex2)

    # zpracuje zpravu od serveru o novych ctvercich
    # pokud je zprava v poradku, do seznamu dokoncenych ctvercu aktivniho hrace je pridan novy
    def command_draw_squares(self, parameters):
        if not self.current_frame == self.frames["GameFrame"]:
            self.wrong_command()
            return

        if len(parameters) < 2:
            self.wrong_command()
            return

        active_player_number = parameters[0]

        if not active_player_number.isnumeric():
            self.wrong_command()
            return

        active_player_number = int(active_player_number)

        if not (active_player_number == 1 or active_player_number == 2):
            self.wrong_command()
            return

        coords = parameters[1:]

        for coord in coords:
            if not coord.isnumeric():
                self.wrong_command()
                return

            coord = int(coord)

            if coord < 0 or coord > DOTS * DOTS - 1:
                self.wrong_command()
                return

        if active_player_number == 1:
            for coord in coords:
                if (int(coord) + 1) % DOTS == 0:  # prava krajni pozice
                    self.wrong_command()
                    return

                if int(coord) + DOTS >= DOTS * DOTS:  # dolni krajni pozice
                    self.wrong_command()
                    return

                x = int(coord) // DOTS
                y = int(coord) % DOTS

                vertex = y * DOTS + x

                if vertex in self.frames["GameFrame"].playground.finished_squares_p1:
                    self.wrong_command()
                    return

                if vertex in self.frames["GameFrame"].playground.finished_squares_p2:
                    self.wrong_command()
                    return

                self.frames["GameFrame"].playground.finished_squares_p1.append(vertex)
        elif active_player_number == 2:
            for coord in coords:
                if (int(coord) + 1) % DOTS == 0:  # prava krajni pozice
                    self.wrong_command()
                    return

                if int(coord) + DOTS >= DOTS * DOTS:  # dolni krajni pozice
                    self.wrong_command()
                    return

                x = int(coord) // DOTS
                y = int(coord) % DOTS

                vertex = y * DOTS + x

                if vertex in self.frames["GameFrame"].playground.finished_squares_p1:
                    self.wrong_command()
                    return

                if vertex in self.frames["GameFrame"].playground.finished_squares_p2:
                    self.wrong_command()
                    return

                self.frames["GameFrame"].playground.finished_squares_p2.append(vertex)

    # zpracuje zpravu od serveru o vitezi hry
    # pokud je zprava v poradku, je zobrazen popup box s vitezem a jeho skorem
    # serveru je poslana zprava LEAVE_GAME
    def command_winner(self, parameters):
        if not self.current_frame == self.frames["GameFrame"]:
            self.wrong_command()
            return

        if len(parameters) != 2:
            self.wrong_command()
            return

        winner = parameters[0]
        score = parameters[1]

        if not winner.isnumeric():
            self.wrong_command()
            return

        winner = int(winner)

        if not (winner == 0 or winner == 1 or winner == 2):
            self.wrong_command()
            return

        if not score.isnumeric():
            self.wrong_command()
            return

        score = int(score)

        if score < 0 or score > (DOTS - 1) * (DOTS - 1):
            self.wrong_command()
            return

        winner_string = ""

        if int(winner) == 1:
            if self.frames["GameFrame"].playground.my_number == 1:
                winner_string = ("The winner is " + self.frames["GameFrame"].my_name + " with a score of "
                                 + str(score) + "!")
            else:
                winner_string = ("The winner is " + self.frames["GameFrame"].opponent_name + " with a score of "
                                 + str(score) + "!")
        elif int(winner) == 2:
            if self.frames["GameFrame"].playground.my_number == 2:
                winner_string = ("The winner is " + self.frames["GameFrame"].my_name + " with a score of "
                                 + str(score) + "!")
            else:
                winner_string = ("The winner is " + self.frames["GameFrame"].opponent_name + " with a score of "
                                 + str(score) + "!")
        elif int(winner) == 0 and score == 0:
            winner_string = "The game ended in a tie."
        else:
            self.wrong_command()
            return

        self.frames["GameFrame"].button_leave_game["state"] = "disabled"
        messagebox.showinfo("Winner", winner_string)
        self.try_send_message(LEAVE_GAME, [], self.frames["GameFrame"].button_leave_game)

    # zpracuje zpravu od serveru o odchodu uzivatele ze hry
    # pokud je zprava v poradku, je uzivatel ze hry odebran a vracen na Enqueue Frame
    def command_you_left_game(self, parameters):
        if not self.current_frame == self.frames["GameFrame"]:
            self.wrong_command()
            return

        if len(parameters) != 1:
            self.wrong_command()
            return

        result = parameters[0]

        if result == OK_STRING:
            self.show_frame("EnqueueFrame")
            self.frames["GameFrame"].reset()
        elif result == ERROR_STRING:
            messagebox.showerror("Error", "Could not leave game.")
        else:
            self.wrong_command()
            return

        self.frames["GameFrame"].button_leave_game["state"] = "normal"

    # zpracuje zpravu od serveru o odchodu oponenta ze hry
    # pokud je zprava v poradku, je uzivatel ze hry odebran a vracen do fronty (Inqueue Frame)
    def command_opponent_left_game(self, parameters):
        if not self.current_frame == self.frames["GameFrame"]:
            self.wrong_command()
            return

        if len(parameters) != 0:
            self.wrong_command()
            return

        messagebox.showwarning("Opponent left", "Opponent left the game. You will be returned to the queue.")
        self.frames["GameFrame"].reset()

    # zpracuje zpravu od serveru o odchodu uzivatele z fronty
    # pokud je zprava v poradku, je uzivatel z fronty odebran a vracen na Enqueue Frame
    def command_left_queue(self, parameters):
        if not self.current_frame == self.frames["InqueueFrame"]:
            self.wrong_command()
            return

        if len(parameters) != 1:
            self.wrong_command()
            return

        result = parameters[0]

        if result == OK_STRING:
            self.show_frame("EnqueueFrame")
        elif result == ERROR_STRING:
            messagebox.showerror("Error", "Could not leave queue.")
        else:
            self.wrong_command()
            return

        self.frames["InqueueFrame"].button_dequeue["state"] = "normal"

    # server se vypina, klient se musi odpojit
    # je zobrazen LoginFrame
    def command_server_shutdown(self):
        self.connected = False
        self.server_available = False
        self.client_socket.close()
        self.client_socket = None
        messagebox.showerror("Server shutdown", "You will be disconnected.")
        self.show_frame("LoginFrame")
        self.unblock_all_buttons()

    # zpracuje zpravu od serveru o stavu pripojeni oponenta
    # 0 - connected, 1 - inactive, 2 - short disconnect
    def command_opponent_connection(self, parameters):
        if not self.current_frame == self.frames["GameFrame"]:
            self.wrong_command()
            return

        if len(parameters) != 1:
            self.wrong_command()
            return

        status = parameters[0]

        if not status.isnumeric():
            self.wrong_command()
            return

        status = int(status)

        if not (status == 0 or status == 1 or status == 2):
            self.wrong_command()
            return

        if status == 0:  # ok
            self.frames["GameFrame"].opponent_status_label.config(text="Status: Connected")
            self.frames["GameFrame"].opponent_status_label.config(fg=CONNECTED_COLOR)
        elif status == 1:  # neaktivni, pinguje
            self.frames["GameFrame"].opponent_status_label.config(text="Status: Inactive")
            self.frames["GameFrame"].opponent_status_label.config(fg=INACTIVE_COLOR)
        elif status == 2:  # nepinguje
            self.frames["GameFrame"].opponent_status_label.config(text="Status: Short Disconnect")
            self.frames["GameFrame"].opponent_status_label.config(fg=DISCONNECTED_SHORT_COLOR)
        else:
            self.wrong_command()
            return

    # zpracuje zpravu od serveru o pokracovani hry
    # server posle, kdyz se jedna o vracejiciho se hrace
    # pokud je zprava v poradku, je zobrazen Enqueue Frame a pokracuje se ve hre
    def command_resume_game(self, parameters):
        if not self.current_frame == self.frames["EnqueueFrame"]:
            self.wrong_command()
            return

        if len(parameters) < 5:
            self.wrong_command()
            return

        if parameters[0] == ERROR_STRING:
            messagebox.showerror("Resume game", "Could not resume game.")
            self.frames["LoginFrame"].button_login["state"] = "normal"
            return

        my_number = parameters[0]
        opponent_name = parameters[1]

        if not my_number.isnumeric():
            self.wrong_command()
            return

        my_number = int(my_number)

        if not (my_number == 1 or my_number == 2):
            self.wrong_command()
            return

        opponent_name = opponent_name.replace(" ", "")

        if len(opponent_name) > NAME_LENGTH:
            self.wrong_command()
            return

        if len(opponent_name) < 1:
            self.wrong_command()
            return

        active_player_number = parameters[2]

        if not active_player_number.isnumeric():
            self.wrong_command()
            return

        active_player_number = int(active_player_number)

        if not (active_player_number == 1 or active_player_number == 2):
            self.wrong_command()
            return

        my_score = parameters[3]
        opponent_score = parameters[4]

        if not my_score.isnumeric():
            self.wrong_command()
            return

        if not opponent_score.isnumeric():
            self.wrong_command()
            return

        my_score = int(my_score)
        opponent_score = int(opponent_score)

        if my_score < 0 or my_score > (DOTS - 1) * (DOTS - 1):
            self.wrong_command()
            return

        if opponent_score < 0 or opponent_score > (DOTS - 1) * (DOTS - 1):
            self.wrong_command()
            return

        my_squares = []
        if my_score > 0:
            if len(parameters) < 5 + my_score:
                self.wrong_command()
                return
            for i in range(5, 5 + my_score):
                coord = parameters[i]
                if not coord.isnumeric():
                    self.wrong_command()
                    return

                coord = int(coord)

                if coord < 0 or coord + DOTS >= DOTS * DOTS:
                    self.wrong_command()
                    return

                my_squares.append(str(coord))

        # obsahuje duplicity
        if len(my_squares) != len(set(my_squares)):
            self.wrong_command()
            return

        opponent_squares = []
        if opponent_score > 0:
            if len(parameters) < 5 + my_score + opponent_score:
                self.wrong_command()
                return
            for i in range(5 + my_score, 5 + my_score + opponent_score):
                coord = parameters[i]
                if not coord.isnumeric():
                    self.wrong_command()
                    return

                coord = int(coord)

                if coord < 0 or coord + DOTS >= DOTS * DOTS:
                    self.wrong_command()
                    return

                opponent_squares.append(str(coord))

        # obsahuje duplicity
        if len(my_squares) != len(set(my_squares)):
            self.wrong_command()
            return

        line_coords = parameters[5 + my_score + opponent_score:]
        if len(line_coords) % 2 != 0:
            self.wrong_command()
            return

        test_graph = Graph(DOTS * DOTS)
        for i in range(0, len(line_coords), 2):
            coord1 = line_coords[i]
            coord2 = line_coords[i + 1]

            if not coord1.isnumeric():
                self.wrong_command()
                return

            if not coord2.isnumeric():
                self.wrong_command()
                return

            coord1 = int(coord1)
            coord2 = int(coord2)

            if coord1 < 0 or coord1 > DOTS * DOTS - 1:
                self.wrong_command()
                return

            if coord2 < 0 or coord2 > DOTS * DOTS - 1:
                self.wrong_command()
                return

            if not (abs(coord1 - coord2) == 1 or abs(coord1 - coord2) == DOTS):
                self.wrong_command()
                return

            x1 = coord1 // DOTS
            y1 = coord1 % DOTS
            x2 = coord2 // DOTS
            y2 = coord2 % DOTS

            vertex1 = y1 * DOTS + x1
            vertex2 = y2 * DOTS + x2

            # uz propojene
            if test_graph.is_neighbor(vertex1, vertex2):
                self.wrong_command()
                return

            test_graph.add_edge(vertex1, vertex2)

        self.command_start_game([str(my_number), opponent_name])
        self.command_play([str(active_player_number)])

        for i in range(0, len(line_coords), 2):
            self.command_draw_coords([line_coords[i], line_coords[i + 1]])

        if my_number == 1:
            my_squares.insert(0, '1')
            opponent_squares.insert(0, '2')
            if my_score > 0:
                self.command_draw_squares(my_squares)
            if opponent_score > 0:
                self.command_draw_squares(opponent_squares)
        elif my_number == 2:
            my_squares.insert(0, '2')
            opponent_squares.insert(0, '1')
            if my_score > 0:
                self.command_draw_squares(my_squares)
            if opponent_score > 0:
                self.command_draw_squares(opponent_squares)

        self.frames["LoginFrame"].button_login["state"] = "normal"

    # posle serveru zpravu s cisly propojenych bodu
    def try_connect_dots(self, dot1: int, dot2: int):
        self.try_send_message(LINE_COORDS, [dot1, dot2])

    # pokusi se pripojit na danou IP adresu a port
    # ceka maximalne 10 sekund na zpravu LOGGED|OK - pripojeni probehlo v poradku
    # pokud nic neprijde nebo prijde neco jineho, pripojeni se nezdarilo
    def try_login(self, ip, port, nickname):
        self.frames["LoginFrame"].button_login["state"] = "disabled"

        ip = ip.replace(" ", "").lower()
        port = port.replace(" ", "")

        if ip == "localhost":
            ip = "127.0.0.1"

        # otestovani validity IP
        try:
            socket.inet_aton(ip)
        except socket.error:
            messagebox.showerror("Error", "The IP is invalid.")
            self.frames["LoginFrame"].button_login["state"] = "normal"
            return

        if not port.isnumeric():
            messagebox.showerror("Error", "The port is invalid.")
            self.frames["LoginFrame"].button_login["state"] = "normal"
            return

        if int(port) < 0 or int(port) > 65535:
            messagebox.showerror("Error", "The port is invalid.")
            self.frames["LoginFrame"].button_login["state"] = "normal"
            return

        # otestovani validity prezdivky
        nickname_stripped = nickname.replace(" ", "")
        self.frames["LoginFrame"].nickname_text.set(nickname_stripped)

        if ("\\" in nickname_stripped) or (MESSAGE_SEPARATOR in nickname_stripped):
            messagebox.showerror("Error", "Nickname contains banned characters.")
            self.frames["LoginFrame"].button_login["state"] = "normal"
            return

        if len(nickname_stripped) < 1:
            messagebox.showerror("Error", "Nickname empty.")
            self.frames["LoginFrame"].button_login["state"] = "normal"
            return

        if len(nickname_stripped) > NAME_LENGTH:
            messagebox.showerror("Error", "Nickname too long.")
            self.frames["LoginFrame"].button_login["state"] = "normal"
            return

        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.settimeout(10)

        try:
            self.client_socket.connect((ip, int(port)))
        except socket.error:
            self.client_socket.close()
            self.client_socket = None
            messagebox.showerror("Error", "Could not connect to the server.")
            self.frames["LoginFrame"].button_login["state"] = "normal"
            return

        self.client_socket.setblocking(False)
        self.try_send_message(LOGIN, [nickname_stripped], self.frames["LoginFrame"].button_login)

        # cekame na zpravu LOGGED
        ready = select.select([self.client_socket], [], [], 10)
        if ready[0]:
            try:
                data = self.client_socket.recv(MESSAGE_BUFFER).decode("utf-8").replace("\x00", "")
            except OSError:
                self.client_socket.close()
                self.client_socket = None
                messagebox.showerror("Error", "Could not log in.")
                self.frames["LoginFrame"].button_login["state"] = "normal"
                return
            except UnicodeDecodeError:
                self.client_socket.close()
                self.client_socket = None
                messagebox.showerror("Error", "Could not log in.")
                self.frames["LoginFrame"].button_login["state"] = "normal"
                return

            message = MessageDecoder(data)

            print("received message: " + message.to_string())
            print("running command: " + message.to_string())

            if message.command == LOGGED:
                if len(message.parameters) != 1:
                    self.client_socket.close()
                    self.client_socket = None
                    messagebox.showerror("Error", "Could not log in.")
                    self.frames["LoginFrame"].button_login["state"] = "normal"
                    return

                result = message.parameters[0]

                # nepodarilo se prihlasit
                if result == ERROR_STRING:
                    self.client_socket.close()
                    self.client_socket = None
                    messagebox.showerror("Error", "Could not log in.")
                    self.frames["LoginFrame"].button_login["state"] = "normal"
                    return

                # v poradku
                elif result == OK_STRING:
                    self.show_frame("EnqueueFrame")
                    self.frames["EnqueueFrame"].nickname_label.configure(text="Nickname: "
                                                                              + self.frames[
                                                                                  "LoginFrame"].nickname_text.get())
                else:
                    self.client_socket.close()
                    self.client_socket = None
                    messagebox.showerror("Error", "Could not log in.")
                    self.frames["LoginFrame"].button_login["state"] = "normal"
                    return

            else:
                self.client_socket.close()
                self.client_socket = None
                messagebox.showerror("Error", "Could not connect to the server.")
                self.frames["LoginFrame"].button_login["state"] = "normal"
                return
        else:
            self.client_socket.close()
            self.client_socket = None
            messagebox.showerror("Error", "Could not connect to the server.")
            self.frames["LoginFrame"].button_login["state"] = "normal"
            return

        # prikaz LOGGED se provedl spravne
        self.connected = True
        self.server_available = True
        self.last_ping_time = time.time()

        self.ip = ip
        self.port = port
        self.nickname = nickname

    # klient se pokusi se znovu pripojit k nedostupnemu serveru
    # zavre socket a zkusi se znovu zalogovat z noveho
    def try_reconnect(self):
        # self.block_all_buttons()
        # self.connected = False
        self.server_available = False
        self.client_socket.close()

        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        try:
            self.client_socket.connect((self.ip, int(self.port)))
            self.server_available = True
            self.last_ping_time = time.time()
        except socket.error:
            return

        self.client_socket.setblocking(False)

        self.try_send_message(LOGIN, [self.nickname], self.frames["LoginFrame"].button_login)

    # klient se pokusi se zaradit do fronty na serveru
    def try_queue(self):
        self.frames["EnqueueFrame"].button_queue["state"] = "disabled"
        self.try_send_message(QUEUE, [], self.frames["EnqueueFrame"].button_queue)

    # klient se odpoji od serveru
    def disconnect(self):
        if self.current_frame == self.frames["EnqueueFrame"]:
            self.connected = False
            self.server_available = False
            self.client_socket.close()
            self.client_socket = None
            self.show_frame("LoginFrame")
            self.unblock_all_buttons()

    # server zaslal zpravu nevyhovujici protokolu, pripojeni bude ukonceno
    def wrong_command(self):
        if self.connected:
            self.frames["GameFrame"].reset()
            self.connected = False
            self.server_available = False
            self.client_socket.close()
            self.client_socket = None
            messagebox.showerror("Server sending wrong commands", "You will be disconnected.")
            self.show_frame("LoginFrame")

            self.unblock_all_buttons()

    # klient se pokusi odejit z fronty na serveru
    def try_leave_queue(self):
        self.frames["InqueueFrame"].button_dequeue["state"] = "disabled"
        self.try_send_message(LEAVE_QUEUE, [], self.frames["InqueueFrame"].button_dequeue)

    # klient se pokusi odejit ze hry
    def try_leave_game(self):
        self.frames["GameFrame"].button_leave_game["state"] = "disabled"
        self.try_send_message(LEAVE_GAME, [], self.frames["GameFrame"].button_leave_game)

    # zablokuje tlacitka na vsech ramcich
    def unblock_all_buttons(self):
        self.frames["LoginFrame"].button_login["state"] = "normal"
        self.frames["EnqueueFrame"].button_queue["state"] = "normal"
        self.frames["EnqueueFrame"].button_disconnect["state"] = "normal"
        self.frames["InqueueFrame"].button_dequeue["state"] = "normal"
        self.frames["GameFrame"].button_leave_game["state"] = "normal"

    # odblokuje tlacitka na vsech ramcich
    def block_all_buttons(self):
        self.frames["LoginFrame"].button_login["state"] = "disabled"
        self.frames["EnqueueFrame"].button_queue["state"] = "disabled"
        self.frames["EnqueueFrame"].button_disconnect["state"] = "disabled"
        self.frames["InqueueFrame"].button_dequeue["state"] = "disabled"
        self.frames["GameFrame"].button_leave_game["state"] = "disabled"
