import tkinter as tk

from constants import BACKGROUND_COLOR, TEXT_COLOR, GRID_COLOR, DOTS, PLAYER1_COLOR, PLAYER2_COLOR, CONNECTED_COLOR, \
    DISCONNECTED_SHORT_COLOR
from playground import Playground


# trida pro ramec, ve kterem je hrana samotna hra
class GameFrame(tk.Frame):

    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent, bg=BACKGROUND_COLOR)
        self.controller = controller

        self.my_score = 0
        self.opponent_score = 0
        self.my_number = 0
        self.my_name = None
        self.opponent_name = None

        self.stats_frame = tk.Frame(self, bg=BACKGROUND_COLOR)
        self.stats_frame.columnconfigure(0, weight=1)
        self.stats_frame.columnconfigure(1, weight=1)

        self.my_name_label = tk.Label(self.stats_frame, text="Me: " + str(self.my_name), font=18,
                                      bg=BACKGROUND_COLOR, fg=TEXT_COLOR)
        self.my_name_label.grid(row=0, column=0, sticky=tk.W)

        self.my_score_label = tk.Label(self.stats_frame, text="My Score: " + str(self.my_score), font=18,
                                       bg=BACKGROUND_COLOR, fg=TEXT_COLOR)
        self.my_score_label.grid(row=1, column=0, sticky=tk.W)

        self.my_status_label = tk.Label(self.stats_frame, text="Status: Connected", font=18,
                                        bg=BACKGROUND_COLOR, fg=CONNECTED_COLOR)
        self.my_status_label.grid(row=2, column=0, sticky=tk.W)

        self.opponent_name_label = tk.Label(self.stats_frame, text="Opponent: " + str(self.opponent_name), font=18,
                                            bg=BACKGROUND_COLOR, fg=TEXT_COLOR)
        self.opponent_name_label.grid(row=0, column=1, sticky=tk.E)

        self.opponent_score_label = tk.Label(self.stats_frame, text="Opponent's Score: " + str(self.opponent_score),
                                             font=18,
                                             bg=BACKGROUND_COLOR, fg=TEXT_COLOR)
        self.opponent_score_label.grid(row=1, column=1, sticky=tk.E)

        self.opponent_status_label = tk.Label(self.stats_frame, text="Status: Connected", font=18,
                                              bg=BACKGROUND_COLOR, fg=CONNECTED_COLOR)
        self.opponent_status_label.grid(row=2, column=1, sticky=tk.E)

        self.stats_frame.pack(pady=20, padx=40, fill="x")

        self.button_grid = tk.Canvas(self, bg=GRID_COLOR)
        for i in range(DOTS):
            self.button_grid.columnconfigure(i, weight=1)
        self.button_grid.pack(padx=10, pady=10)

        self.playground = Playground(self.controller, self.button_grid, self.my_number)

        self.turn_label = tk.Label(self, text="Player " + str(self.playground.active_player) + "'s Turn", font=18,
                                   bg=BACKGROUND_COLOR, fg=TEXT_COLOR)
        self.turn_label.pack(pady=10)

        self.button_leave_game = tk.Button(self, text="Leave Game", command=lambda: controller.try_leave_game())
        self.button_leave_game.pack(padx=150, pady=30)

    def update_frame(self):
        self.playground.update()
        self.update_stats()
        self.update_turn()

        if self.controller.server_available:
            self.my_status_label.config(text="Status: Connected")
            self.my_status_label.config(fg=CONNECTED_COLOR)
        else:
            self.my_status_label.config(text="Status: Server Unavailable")
            self.my_status_label.config(fg=DISCONNECTED_SHORT_COLOR)

    # update skore
    def update_stats(self):
        self.my_score = self.playground.get_my_score()
        self.opponent_score = self.playground.get_opponent_score()

        self.my_score_label.config(text="Score: " + str(self.my_score))
        self.opponent_score_label.config(text="Score: " + str(self.opponent_score))

        if self.my_number == 1:
            self.my_score_label.config(fg=PLAYER1_COLOR)
            self.my_name_label.config(fg=PLAYER1_COLOR)
            self.opponent_score_label.config(fg=PLAYER2_COLOR)
            self.opponent_name_label.config(fg=PLAYER2_COLOR)
        elif self.my_number == 2:
            self.my_score_label.config(fg=PLAYER2_COLOR)
            self.my_name_label.config(fg=PLAYER2_COLOR)
            self.opponent_score_label.config(fg=PLAYER1_COLOR)
            self.opponent_name_label.config(fg=PLAYER1_COLOR)

    # update hrace na tahu
    def update_turn(self):
        if self.playground.active_player == 0:
            self.turn_label.config(text="")
        elif self.playground.active_player == self.my_number:
            self.turn_label.config(text=str(self.my_name) + "'s Turn")
            if self.my_number == 1:
                self.turn_label.config(fg=PLAYER1_COLOR)
            else:
                self.turn_label.config(fg=PLAYER2_COLOR)
        else:
            self.turn_label.config(text=str(self.opponent_name) + "'s Turn")
            if self.my_number == 1:
                self.turn_label.config(fg=PLAYER2_COLOR)
            else:
                self.turn_label.config(fg=PLAYER1_COLOR)

    # reset pro novou hru
    def reset(self):
        self.my_score = 0
        self.opponent_score = 0
        self.playground = Playground(self.controller, self.button_grid, self.my_number)
        self.button_grid.pack(pady=10)
