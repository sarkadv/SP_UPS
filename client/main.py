from controller import *
from threading import Thread
import time
import select


# vstupni bod programu
# vykonava prikazy ve fronte a aktualizuje GUI
def main():
    controller = Controller()
    message_queue = []

    thread_select = Thread(target=select_function, args=(controller, message_queue))
    thread_select.start()

    while not controller.exitFlag:
        controller.update_gui()

        while len(message_queue) > 0:
            decoded = message_queue[0]
            print("running command: " + decoded.to_string())
            controller.run_command(decoded)
            message_queue.remove(decoded)

    thread_select.join()
    exit()


# funkce, ktera bezi ve vlakne
# prijima zpravy a pokud se jedna o zpravu PING, odpovi na ni
def select_function(controller, message_queue: list):
    reconnect_timer = time.time()
    while not controller.exitFlag:
        if controller.connected:

            # nedostupnost serveru
            if time.time() - controller.last_ping_time > PING_MAX_SECONDS:
                controller.server_available = False

            # klient se periodicky bude snazit k serveru znovu pripojit
            if (not controller.server_available) and (time.time() - reconnect_timer > RECONNECT_TIME):
                controller.try_reconnect()
                print("ahoh")
                reconnect_timer = time.time()

            # prijem zprav ze serveru
            try:
                ready = select.select([controller.client_socket], [], [], 0)
            except ValueError:
                continue

            if ready[0]:
                try:
                    data = controller.client_socket.recv(MESSAGE_BUFFER).decode('utf-8').replace("\x00", "")
                except OSError:
                    continue
                except UnicodeDecodeError:
                    if MessageDecoder("invalid") not in message_queue:
                        message_queue.append(MessageDecoder("invalid"))
                    continue

                messages = data.split(MESSAGE_END)
                for message in messages:
                    if len(message) > 0:
                        decoded = MessageDecoder(message)
                        print("received message: " + decoded.to_string())

                        if decoded.command == PING:
                            print("running command: " + decoded.to_string())
                            controller.run_command(decoded)
                            controller.last_ping_time = time.time()
                        else:
                            message_queue.append(decoded)


main()
