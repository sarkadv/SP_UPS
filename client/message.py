from constants import MESSAGE_END, MESSAGE_SEPARATOR


# trida pro dekodovanou zpravu ze serveru
class MessageDecoder:
    def __init__(self, data):
        data = data.replace("\x00", "")
        data = data.replace(MESSAGE_END, "")
        data = data.strip()

        self.command = data.split(MESSAGE_SEPARATOR)[0]
        self.parameters = data.split(MESSAGE_SEPARATOR)[1:]

    def __eq__(self, other):
        return self.command == other.command

    def to_string(self):
        string = self.command

        for parameter in self.parameters:
            string = string + MESSAGE_SEPARATOR
            string = string + parameter

        return string


# trida pro zakodovanou zpravu pro server
class MessageEncoder:
    def __init__(self, command, parameters):
        self.message = command

        for parameter in parameters:
            self.message += MESSAGE_SEPARATOR + str(parameter)

        self.message += MESSAGE_END
        self.message += "\0"

    # vrati zakodovanou zpravu bez ukoncovacich znaku
    def get_raw_message(self):
        raw = self.message.replace(MESSAGE_END, "")
        raw = raw.replace("\0", "")
        return raw
