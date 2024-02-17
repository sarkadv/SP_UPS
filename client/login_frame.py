import tkinter as tk
import tkinter.font as tkfont

from constants import BACKGROUND_COLOR, TEXT_COLOR


# trida pro ramec, ve kterem je mozne se pripojit na server
class LoginFrame(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent, bg=BACKGROUND_COLOR)
        self.controller = controller
        self.ip_text = tk.StringVar()
        self.port_text = tk.StringVar()
        self.nickname_text = tk.StringVar()

        head_font = tkfont.Font(family="Helvetica", size=36, weight="bold")
        head_label = tk.Label(self, text="DOTS & BOXES", font=head_font, fg=TEXT_COLOR, bg=BACKGROUND_COLOR)
        head_label.pack(side="top", fill="x", pady=50)

        self.ip_label = tk.Label(self, text="IP Adress: ", fg=TEXT_COLOR, bg=BACKGROUND_COLOR)
        self.ip_label.pack(fill="x", padx=150, pady=10)

        self.ip_entry = tk.Entry(self, textvariable=self.ip_text)
        self.ip_entry.pack(fill="x", padx=150, pady=10)

        self.port_label = tk.Label(self, text="Port: ", fg=TEXT_COLOR, bg=BACKGROUND_COLOR)
        self.port_label.pack(fill="x", padx=150, pady=10)

        self.port_entry = tk.Entry(self, textvariable=self.port_text)
        self.port_entry.pack(fill="x", padx=150, pady=10)

        self.nickname_label = tk.Label(self, text="Nickname: ", fg=TEXT_COLOR, bg=BACKGROUND_COLOR)
        self.nickname_label.pack(fill="x", padx=150, pady=10)

        self.nickname_entry = tk.Entry(self, textvariable=self.nickname_text)
        self.nickname_entry.pack(fill="x", padx=150, pady=10)

        self.button_login = tk.Button(self, text="Log in",
                                      command=lambda: controller.try_login(self.ip_entry.get(), self.port_entry.get(),
                                                                           self.nickname_entry.get()))
        self.button_login.pack(padx=150, pady=30)

    def update_frame(self):
        pass
