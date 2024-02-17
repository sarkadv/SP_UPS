import tkinter as tk
from constants import *


# trida pro propojovana tlacitka
class CustomButton:
    def __init__(self, button: tk.Button, x: int, y: int):
        self.button = button
        self.button.config(command=self.on_click)
        self.x = x  # radek
        self.y = y  # sloupec
        self.available = True
        self.clicked = False
        self.max_connected_btns = self.find_max_connected_btns()

    # vrati cislo maximalniho poctu tlacitek, co se k tomuto tlacitku mohou pripojit
    def find_max_connected_btns(self):
        # rohy
        if self.x == 0 and self.y == 0:
            return 2
        elif self.x == 0 and self.y == DOTS - 1:
            return 2
        elif self.x == DOTS - 1 and self.y == 0:
            return 2
        elif self.x == DOTS - 1 and self.y == DOTS - 1:
            return 2
        # kraje
        elif self.x == 0:
            return 3
        elif self.y == 0:
            return 3
        elif self.x == DOTS - 1:
            return 3
        elif self.y == DOTS - 1:
            return 3
        # ostatni
        else:
            return 4

    def on_click(self):
        if self.available:
            self.clicked = not self.clicked

    def update(self):
        if self.clicked:
            self.button.config(bg=CLICKED_COLOR, relief="solid")
        elif self.available:
            self.button.config(bg=AVAILABLE_COLOR, relief="solid")
        else:
            self.button.config(bg=DEFAULT_COLOR, relief="raised")

    def get_button(self):
        return self.button

    def get_x(self):
        return self.x

    def get_y(self):
        return self.y

    def get_clicked(self):
        return self.clicked

    def set_clicked(self, clicked: bool):
        self.clicked = clicked

    def set_available(self, available: bool):
        self.available = available

    def get_max_connected_btns(self):
        return self.max_connected_btns
