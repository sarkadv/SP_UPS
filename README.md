# Dots & Boxes

Tahová síťová hra pro dva hráče. Server je napsán v C/C++ a klient v Pythonu s grafickým rozhraním v tkinteru,
komunikují spolu přes TCP. Semestrální práce z předmětu KIV/UPS.

Podrobná dokumentace je v souboru `dokumentace.pdf`.

Hráči se střídají ve spojování dvou sousedních teček na herní ploše 5 × 5. Kdo uzavře čtverec, získá bod a
hraje znovu. Když je plocha zaplněná čtverci, vyhrává hráč s více body. Pravidla jsou popsána na
[Wikipedii](https://en.wikipedia.org/wiki/Dots_and_boxes).

## Vlastnosti

- Přihlášení přezdívkou, zařazení do fronty a spárování s dalším čekajícím hráčem.
- Server kontroluje každý tah, klient tedy nemůže podvádět.
- Server dokáže obsluhovat více klientů a her najednou. Klient ani server není nutné mezi hrami restartovat.
- **Detekce výpadku.** Server posílá každých 5 sekund zprávu `ping` a klient odpovídá `pong`. Klient, který
  neodpovídá, je krátkodobě nedostupný a může se znovu připojit a pokračovat v rozehrané hře. Po 12
  vynechaných odpovědích je odpojen. Klient podle zpráv `ping` zjišťuje dostupnost serveru a v případě výpadku
  se k němu pokouší znovu připojit.
- Hráč vidí stav připojení svého protihráče: v pořádku, dlouho (30 s) neprovedl tah, nebo krátkodobě
  nedostupný.
- Klient, který pošle zprávu nevyhovující protokolu, je odpojen.
- Server zapisuje logy do souboru `log.txt`.

## Server

Server běží na GNU/Linuxu. K překladu je potřeba `make` a `g++` (standard C++11).

```
cd server
make
./server <IP adresa> <port>
```

Pokud nejsou zadány parametry, server načte konfigurační soubor `config.txt`, kde je na prvním řádku IP adresa
(nebo `localhost` či `any`) a na druhém číslo portu. Pokud ani to nejde, použije se `127.0.0.1:10000`.

Server pracuje pseudoparalelně pomocí funkce `select()`.

## Klient

Klient běží na GNU/Linuxu a Windows. Potřebuje Python 3.10 a tkinter 8.6, žádné další knihovny.

```
cd client
python3 main.py
```

Zadejte IP adresu a port serveru a svou přezdívku (nejvýše 20 znaků) a zařaďte se do fronty.

Klient běží na dvou vláknech. Hlavní vlákno aktualizuje grafické rozhraní a vykonává příkazy z fronty, druhé
vlákno přijímá zprávy od serveru pomocí `select()` a na zprávy `ping` odpovídá rovnou.

## Komunikační protokol

Zprávy jsou textové, jednotlivé části se oddělují znakem `|` a každá zpráva končí znakem `\n`.

- **Klient → server:** `login`, `queue`, `leave_queue`, `line_coords`, `leave_game`, `pong`
- **Server → klient:** `ping`, `logged`, `queued`, `start_game`, `play`, `draw_coords`, `invalid_coords`,
  `draw_squares`, `winner`, `opponent_connection`, `opponent_left_game`, `resume_game`, `server_shutdown` a
  další

Popis všech zpráv a jejich parametrů a stavové diagramy hráče a hry jsou v `dokumentace.pdf`.
