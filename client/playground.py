from button import *
from graph import *


# trida pro spravu herni plochy, obsahuje canvas pro vykreslovani tlacitek, jejich propojeni a ctvercu
class Playground:
    def __init__(self, controller, button_grid: tk.Canvas, my_number):
        self.controller = controller
        self.button_grid = button_grid
        self.buttons = [[CustomButton for _ in range(DOTS)] for _ in range(DOTS)]
        self.padding = DEFAULT_PADDING
        self.clicked_buttons = []
        self.graph = Graph(DOTS * DOTS)
        self.active_player = 0
        self.my_number = my_number
        self.finished_squares_p1 = []
        self.finished_squares_p2 = []

        for x in range(DOTS):
            for y in range(DOTS):
                self.buttons[x][y] = CustomButton(tk.Button(self.button_grid, text="", width=1, height=1, borderwidth=1,
                                                            relief="solid", bg=DEFAULT_COLOR), x, y)
                self.buttons[x][y].get_button().grid(row=x, column=y, padx=self.padding, pady=self.padding,
                                                     sticky=tk.W + tk.E)

    def update(self):
        self.button_grid.delete("all")

        # tento uzivatel na tahu
        if self.active_player == self.my_number:
            self.find_clicked_buttons()

            # zadne zakliknute tlacitko
            if len(self.clicked_buttons) == 0:
                self.make_all_btns_available()
            # jedno zakliknute tlacitko
            elif len(self.clicked_buttons) == 1:
                self.set_available_btns()
            # dve zakliknuta tlacitka - poslani zpravy serveru
            elif len(self.clicked_buttons) == 2:
                self.controller.try_connect_dots(get_index_in_graph_server(self.clicked_buttons[0].get_x(),
                                                                           self.clicked_buttons[0].get_y()),
                                                 get_index_in_graph_server(self.clicked_buttons[1].get_x(),
                                                                           self.clicked_buttons[1].get_y()))
                self.clicked_buttons[0].set_clicked(False)
                self.clicked_buttons[1].set_clicked(False)
                self.clicked_buttons.clear()
                self.make_all_btns_unavailable()
                self.active_player = 0
        # oponent na tahu
        else:
            self.set_available_btns()

        for x in range(DOTS):
            for y in range(DOTS):
                self.buttons[x][y].update()

        self.draw_squares()
        self.draw_lines()

    # prida zakliknute tlacitko do seznamu self.clicked_buttons
    def find_clicked_buttons(self):
        for x in range(DOTS):
            for y in range(DOTS):
                if self.buttons[x][y].get_clicked():
                    if not (self.buttons[x][y] in self.clicked_buttons):
                        self.clicked_buttons.append(self.buttons[x][y])
                else:
                    if self.buttons[x][y] in self.clicked_buttons:
                        self.clicked_buttons.remove(self.buttons[x][y])
                        self.make_all_btns_available()

    # vybarvi tlacitka mozna k zakliknuti, na ostatni tlacitka neni mozne kliknout
    def set_available_btns(self):
        if self.active_player == self.my_number:
            for x in range(DOTS):
                for y in range(DOTS):
                    if self.buttons[x][y] == self.clicked_buttons[0]:
                        self.buttons[x][y].set_available(True)
                    elif are_adjacent(self.buttons[x][y], self.clicked_buttons[0]):
                        if not self.graph.is_neighbor(get_index_in_graph(x, y),
                                                      get_index_in_graph(self.clicked_buttons[0].get_x(),
                                                                         self.clicked_buttons[0].get_y())):
                            self.buttons[x][y].set_available(True)
                        else:
                            self.buttons[x][y].set_available(False)
                    else:
                        self.buttons[x][y].set_available(False)
        else:
            self.make_all_btns_unavailable()

    # vsechna tlacitka udela kliknutelna
    def make_all_btns_available(self):
        for x in range(DOTS):
            for y in range(DOTS):
                if (len(self.graph.neighbors(get_index_in_graph(x, y)))
                        == self.buttons[x][y].get_max_connected_btns()):
                    self.buttons[x][y].set_available(False)
                else:
                    self.buttons[x][y].set_available(True)

    # vsechna tlacitka udela zablokovana
    def make_all_btns_unavailable(self):
        for x in range(DOTS):
            for y in range(DOTS):
                self.buttons[x][y].set_available(False)

    # vykresli propojeni mezi tlacitky
    def draw_lines(self):
        for x in range(DOTS):
            for y in range(DOTS):
                for neighbor in self.graph.neighbors(get_index_in_graph(x, y)):
                    self.draw_line(self.get_btn_by_index(neighbor).get_button(), self.buttons[x][y].get_button())

    def draw_line(self, btn1: tk.Button, btn2: tk.Button):
        self.button_grid.create_line(btn1.winfo_x() + btn1.winfo_width() / 2,
                                     btn1.winfo_y() + btn1.winfo_height() / 2,
                                     btn2.winfo_x() + btn2.winfo_width() / 2,
                                     btn2.winfo_y() + btn2.winfo_height() / 2,
                                     fill=LINE_COLOR, width=5)

    # vykresli hotove ctverce
    def draw_squares(self):
        for vertex in self.finished_squares_p1:
            self.draw_square(self.get_btn_by_index(vertex).get_button(), PLAYER1_COLOR)

        for vertex in self.finished_squares_p2:
            self.draw_square(self.get_btn_by_index(vertex).get_button(), PLAYER2_COLOR)

    def draw_square(self, btn1: tk.Button, color):
        self.button_grid.create_rectangle(btn1.winfo_x() + btn1.winfo_width() / 2,
                                          btn1.winfo_y() + btn1.winfo_height() / 2,
                                          btn1.winfo_x() + btn1.winfo_width() * 1.5 + self.padding * 2,
                                          btn1.winfo_y() + btn1.winfo_height() * 1.5 + self.padding * 2,
                                          fill=color, width=0)

    # vrati tlacitko podle indexu v grafu
    def get_btn_by_index(self, index: int):
        x = index % DOTS
        y = index // DOTS

        return self.buttons[x][y]

    # vrati skore tohoto uzivatele
    def get_my_score(self):
        if self.my_number == 1:
            return len(self.finished_squares_p1)
        else:
            return len(self.finished_squares_p2)

    # vrati skore oponenta
    def get_opponent_score(self):
        if self.my_number == 1:
            return len(self.finished_squares_p2)
        else:
            return len(self.finished_squares_p1)


# vrati True, pokud jsou tlacitka spojitelna (vedle sebe), jinak vrati False
def are_adjacent(btn1: CustomButton, btn2: CustomButton):
    if btn1.get_x() == btn2.get_x() + 1 and btn1.get_y() == btn2.get_y():
        return True
    elif btn1.get_x() == btn2.get_x() - 1 and btn1.get_y() == btn2.get_y():
        return True
    elif btn1.get_x() == btn2.get_x() and btn1.get_y() == btn2.get_y() + 1:
        return True
    elif btn1.get_x() == btn2.get_x() and btn1.get_y() == btn2.get_y() - 1:
        return True
    else:
        return False


# vrati index tlacitka v grafu
def get_index_in_graph(x: int, y: int):
    return y * DOTS + x


# vrati index tlacitka v grafu na serveru
def get_index_in_graph_server(x: int, y: int):
    return x * DOTS + y
