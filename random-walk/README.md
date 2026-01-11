# Random Walk Simulation

Klient-server aplikácia pre simuláciu náhodnej prechádzky (random walk) na toroidálnej mriežke.

## Popis projektu

Tento projekt implementuje distribuovaný systém simulácie náhodnej prechádzky, kde:
- **Server** vykonáva simuláciu a posiela stavy klientovi
- **Klient** zobrazuje priebeh simulácie v reálnom čase

### Princíp simulácie

- Simulácia prebieha na 2D toroidálnej mriežke (svet s "obtočenými" okrajmi)
- Začiatočná pozícia: pravý dolný roh (width-1, height-1)
- Cieľ: dosiahnuť ľavý horný roh (0, 0)
- Pohyb: každý krok je náhodný podľa zadaných pravdepodobností (hore, dole, doľava, doprava)
- Maximálny počet krokov: k_max
- Počet replikácií: reps (simulácia sa opakuje viacnásobne)

## Štruktúra projektu

```
random-walk/
├── bin/                    # Skompilované binárky
│   ├── client             # Klientska aplikácia
│   └── server             # Serverová aplikácia
├── include/               # Verejné hlavičkové súbory
│   ├── net.h              # Sieťové funkcie (TCP)
│   └── protocol.h         # Komunikačný protokol
├── src/
│   ├── client/            # Zdrojové súbory klienta
│   │   ├── client.c/h     # Hlavná logika klienta
│   │   ├── main.c         # Vstupný bod klienta
│   │   ├── menu.c/h       # Interaktívne menu
│   │   └── render.c/h     # Zobrazovanie (placeholder)
│   ├── common/            # Zdieľané súbory
│   │   ├── net.c          # Implementácia TCP komunikácie
│   │   └── protocol.c     # Implementácia protokolu
│   └── server/            # Zdrojové súbory servera
│       ├── server.c/h     # Hlavná logika servera
│       ├── main.c         # Vstupný bod servera
│       ├── config.c/h     # Konfigurácia (placeholder)
│       ├── simulation.c/h # Simulačná logika (placeholder)
│       ├── world.c/h      # Správa sveta (placeholder)
│       └── results.c/h    # Spracovanie výsledkov (placeholder)
├── Makefile               # Build skript
└── README.md              # Táto dokumentácia
```

## Kompilácia

```bash
make
```

Výstup:
- `bin/server` - serverová aplikácia
- `bin/client` - klientska aplikácia

Vyčistenie:
```bash
make clean
```

## Použitie

### Spustenie servera

```bash
./bin/server [port]
```

Príklady:
```bash
./bin/server           # Počúva na porte 5555 (predvolené)
./bin/server 8080      # Počúva na porte 8080
```

### Spustenie klienta

```bash
./bin/client [host] [port]
```

Príklady:
```bash
./bin/client                    # Pripojí sa na 127.0.0.1:5555
./bin/client 192.168.1.100      # Pripojí sa na 192.168.1.100:5555
./bin/client localhost 8080     # Pripojí sa na localhost:8080
```

### Interaktívne menu klienta

Po spustení klienta sa zobrazí menu s možnosťami:

1. **Nová simulácia (spawn server + START)**
   - Automaticky spustí serverový proces
   - Pripojí sa k nemu
   - Pýta sa parametre simulácie:
     - Šírka sveta (W)
     - Výška sveta (H)
     - Maximálny počet krokov (K)
     - Počet replikácií (R)
     - Seed pre RNG (0 = aktuálny čas)
     - Pravdepodobnosti pohybu (%, súčet musí byť 100)

2. **Pripojiť sa k simulácii (iba connect)**
   - Pripojí sa k už bežiacemu serveru
   - Bez spustenia novej simulácie

3. **Koniec**
   - Pošle serveru QUIT správu
   - Ukončí klienta

## Komunikačný protokol

Protokol používa binárne správy s hlavičkou:

### Hlavička správy
```c
typedef struct {
    uint32_t type;      // Typ správy (network byte order)
    uint32_t length;    // Dĺžka payloadu (network byte order)
} msg_header_t;
```

### Typy správ

1. **MSG_HELLO** (1) - Klient → Server
   - Žiadosť o pripojenie
   - Payload: text (napr. "hello-from-client")

2. **MSG_HELLO_ACK** (2) - Server → Klient
   - Potvrdenie pripojenia
   - Payload: žiadny

3. **MSG_START** (3) - Klient → Server
   - Parametre simulácie
   - Payload: `msg_start_t`

4. **MSG_STATE** (4) - Server → Klient
   - Aktuálny stav simulácie
   - Payload: `msg_state_t`
   - Posiela sa po každom kroku

5. **MSG_DONE** (5) - Server → Klient
   - Koniec simulácie
   - Payload: žiadny

6. **MSG_QUIT** (6) - Klient → Server
   - Ukončenie servera
   - Payload: žiadny

### Štruktúry správ

```c
// Parametre simulácie
typedef struct {
    int32_t width;
    int32_t height;
    uint32_t k_max;
    uint32_t reps;
    uint32_t seed;
    uint8_t p_up;
    uint8_t p_down;
    uint8_t p_left;
    uint8_t p_right;
} msg_start_t;

// Stav simulácie
typedef struct {
    int32_t x;
    int32_t y;
    uint32_t step;
    uint32_t rep;
    uint32_t reps_total;
} msg_state_t;
```

## Implementované funkcie

### Sieťová vrstva (net.h/net.c)

- `net_listen()` - Vytvorí počúvajúci TCP socket
- `net_accept()` - Prijme klientske pripojenie
- `net_connect()` - Pripojí sa k serveru
- `net_send_all()` - Odošle všetky bajty
- `net_recv_all()` - Prijme všetky bajty

### Protokolová vrstva (protocol.h/protocol.c)

- `proto_send()` - Odošle správu s hlavičkou
- `proto_recv()` - Prijme správu s hlavičkou

### Server (server.h/server.c)

- `server_run()` - Hlavná funkcia servera
- Vlákno pre príjem správ (`net_thread`)
- Vlákno pre simuláciu (`sim_thread`)
- Thread-safe prístup k zdieľaným dátam pomocou mutex

### Klient (client.h/client.c)

- `client_connect_only()` - Pripojenie bez simulácie
- `client_start_simulation()` - Spustenie simulácie s parametrami
- `client_quit_server_and_close()` - Ukončenie
- Vlákno pre príjem stavov (`recv_thread`)

### Menu (menu.h/menu.c)

- `menu_read_choice()` - Zobrazenie menu a výber možnosti
- `menu_read_int()` - Načítanie celého čísla s validáciou
- `menu_read_uint()` - Načítanie nezáporného čísla
- `menu_read_dir_percents()` - Načítanie pravdepodobností (súčet = 100)

## Technické detaily

### Vláknová bezpečnosť

Server aj klient používajú viacero vlákien:
- **Server**: `main` (accept loop), `net_thread` (príjem správ), `sim_thread` (simulácia)
- **Klient**: `main` (menu loop), `recv_thread` (príjem stavov)

Prístup k zdieľaným dátam je chránený pomocou `pthread_mutex_t`.

### Toroidálna topológia

Funkcia `wrap_i32()` zabezpečuje, že pozície sa "obtáčajú":
- Krok doľava z x=0 vedie na x=width-1
- Krok hore z y=0 vedie na y=height-1
- atď.

### Generátor náhodných čísel

Používa sa `rand_r()` pre thread-safe generovanie náhodných čísel.
Seed sa môže zadať manuálne alebo použiť aktuálny čas.

## Príklad použitia

```bash
# Terminál 1: Spustiť server
./bin/server 5555

# Terminál 2: Spustiť klienta a vytvoriť simuláciu
./bin/client 127.0.0.1 5555
# V menu zvoliť 1 (Nová simulácia)
# Zadať parametre: W=10, H=10, K=200, R=5, seed=0
# Zadať pravdepodobnosti: UP=25, DOWN=25, LEFT=25, RIGHT=25
```

Výstup klienta bude zobrazovať jednotlivé kroky simulácie:
```
[client] rep=1/5 step=1 pos=(9,8)
[client] rep=1/5 step=2 pos=(8,8)
[client] rep=1/5 step=3 pos=(8,7)
...
[client] simulation finished (MSG_DONE)
```

## Poznámky

- Server podporuje iba jedného aktívneho klienta naraz
- Nový klient nahradí starého (starý je odpojený)
- Simulácia pokračuje 100ms pauza medzi krokmi
- Všetky číselné hodnoty v protokole sú v network byte order (big-endian)

## Autor

POS projekt - Náhodná prechádzka (Random Walk)
