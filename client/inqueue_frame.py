import tkinter as tk
import tkinter.font as tkfont

from constants import BACKGROUND_COLOR, TEXT_COLOR, CONNECTED_COLOR, DISCONNECTED_SHORT_COLOR


# trida pro ramec, ve kterem je mozne odejit z fronty
class InqueueFrame(tk.Frame):

    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent, bg=BACKGROUND_COLOR)
        self.controller = controller

        head_font = tkfont.Font(family="Helvetica", size=36, weight="bold")

        head_label = tk.Label(self, text="DOTS & BOXES", font=head_font, fg=TEXT_COLOR, bg=BACKGROUND_COLOR)
        head_label.pack(side="top", fill="x", pady=50)

        self.nickname_label = tk.Label(self, text="Nickname:", fg=TEXT_COLOR, bg=BACKGROUND_COLOR)
        self.nickname_label.pack(side="top", fill="x", pady=10)

        self.status_label = tk.Label(self, text="Status:", fg=CONNECTED_COLOR, bg=BACKGROUND_COLOR)
        self.status_label.pack(side="top", fill="x", pady=10)

        queue_label = tk.Label(self, text="QUEUED", fg=TEXT_COLOR, bg=BACKGROUND_COLOR)
        queue_label.pack(side="top", fill="x", pady=30)

        self.button_dequeue = tk.Button(self, text="Leave Queue", command=lambda: controller.try_leave_queue())
        self.button_dequeue.pack(padx=150, pady=30)

    def update_frame(self):
        if self.controller.server_available:
            self.status_label.config(text="Status: Connected")
            self.status_label.config(fg=CONNECTED_COLOR)
        else:
            self.status_label.config(text="Status: Server Unavailable")
            self.status_label.config(fg=DISCONNECTED_SHORT_COLOR)
