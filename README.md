# JonixLUG-AQI — Centralina qualità dell'aria

Centralina open source per il monitoraggio della qualità dell'aria: particolato **PM2.5** e **PM10**, **temperatura** e **umidità**, con compensazione algoritmica dei valori di particolato in base all'umidità relativa.

Fork del progetto originale [JonixLUG ABC](https://www.jonixlug.altervista.org/jonixlug-aria-bene-comune/) (GPLv3, 2019) con bugfix, pulizia del codice e invio multi-piattaforma.

## Dove finiscono i dati

Il firmware invia ogni lettura a tre piattaforme in parallelo (ciascuna attivabile/disattivabile da `config.h`):

| Piattaforma | Tipo | Perché |
|---|---|---|
| [Sensor.Community](https://sensor.community/) | Rete globale citizen science | 15k+ stazioni, mappa pubblica, open data |
| [openSenseMap](https://opensensemap.org/) | Rete accademica open data | Università di Münster, download CSV, interpolazione |
| InfluxDB + Grafana | Self-hosted su VPS | Dashboard personalizzata su `aria.farezero.org` |

## Hardware

| Componente | Modello | Pin Wemos |
|---|---|---|
| Microcontrollore | Wemos D1 Mini (ESP8266) | — |
| Temperatura + umidità | DHT22 | D2 (GPIO4), 3.3V, GND |
| Particolato PM2.5/PM10 | SDS011 (Nova Fitness) | D5 (GPIO14), D6 (GPIO12), 5V, GND |

I due sensori condividono il pin GND del Wemos.

```
Wemos D1 Mini
┌──────────────┐
│  3.3V ───────┼──── DHT22 VCC
│  D2 (GPIO4) ─┼──── DHT22 DATA
│  5V ─────────┼──── SDS011 VCC
│  D6 (GPIO12)─┼──── SDS011 TX
│  D5 (GPIO14)─┼──── SDS011 RX
│  GND ────────┼──── DHT22 GND + SDS011 GND
└──────────────┘
```

Alimentazione: qualsiasi sorgente 5V micro-USB, almeno 500mA (consigliato 1A).

## Setup software

1. Installa [Arduino IDE](https://www.arduino.cc/en/software) (1.8.9+)
2. Aggiungi il board manager ESP8266:
   `File → Preferences → Additional Boards Manager URLs` →
   `https://arduino.esp8266.com/stable/package_esp8266com_index.json`
3. Copia la cartella `libraries/` nella cartella librerie di Arduino IDE (es. `~/Arduino/libraries/`)
4. Copia `config.h.example` in `config.h` e modifica i valori
5. Compila e carica `jonixlug-aqi-v2.ino` sulla board Wemos D1 Mini

## Configurazione

Tutti i parametri personalizzabili sono in **`config.h`** (copia da `config.h.example`):

```cpp
// Wi-Fi
const char* WIFI_SSID = "NOME_RETE_WIFI";
const char* WIFI_PASS = "PASSWORD_WIFI";

// Sensor.Community — automatico, basato sul chip ID
const bool ENABLE_SENSOR_COMMUNITY = true;

// openSenseMap — registra un box su opensensemap.org
const bool ENABLE_OPENSENSEMAP = true;
const char* OSM_BOX_ID = "IL_TUO_BOX_ID";
// ... + 4 sensor ID

// InfluxDB — server self-hosted
const bool ENABLE_INFLUXDB = true;
const char* INFLUX_HOST = "167.235.156.83";
```

### Registrazione sensori

- **Sensor.Community**: registra su [devices.sensor.community](https://devices.sensor.community/) — il sensor ID è generato automaticamente dal chip ID dell'ESP8266
- **openSenseMap**: crea un account e registra un box su [opensensemap.org](https://opensensemap.org/), aggiungi 4 sensori (PM10, PM2.5, Temperatura, Umidità) e copia gli ID nel `config.h`
- **InfluxDB**: installa InfluxDB sulla tua VPS e crea il database `airquality`

## Cosa fa il firmware

Ogni ciclo (default 15 min):
1. Si connette al WiFi
2. Legge temperatura e umidità dal DHT22
3. Sveglia l'SDS011, calibra la ventola (15s), raccoglie 10 campioni
4. Scarta i campioni fuori range e calcola la media
5. Normalizza i valori PM in base all'umidità (algoritmo di compensazione)
6. Invia i dati alle piattaforme abilitate
7. Spegne WiFi e sensore, dorme fino al ciclo successivo

## Modifiche V2 rispetto all'originale

- **Invio multi-piattaforma**: Sensor.Community + openSenseMap + InfluxDB al posto di ThingSpeak
- **Fix stack overflow**: le chiamate ricorsive a `loop()` sono state sostituite con `return`
- **Fix HTTP**: parsing risposta e formato richiesta corretti
- **Rimosso codice morto**: include e variabili inutilizzati
- **Configurazione separata** in `config.h` (con template `.example`)
- **HTTP client corretto** con `ESP8266HTTPClient` al posto di richieste raw
- **Campionamento robusto**: i campioni fuori range vengono saltati senza interrompere il ciclo

## Installazione della centralina

- Posizionare in zona ombreggiata, con accesso diretto all'aria esterna
- Protetta da pioggia (es. sotto un balcone)
- Evitare raggi solari diretti sul sensore

## Case 3D

File `.stl` per la stampa 3D del contenitore (design: Alessandro Chiffi / Ondata Studio) disponibili nel [repo originale](https://gitlab.com/JonixLUG/jonixlug-aqi).

## Licenza e attribuzioni

**Codice: [GPLv3](LICENSE)** — **Dati raccolti: CC BY 4.0**

Progetto originale **JonixLUG ABC** — [gitlab.com/JonixLUG](https://gitlab.com/JonixLUG)
- Autori: Dario P. & Vincenzo Q. (Team JonixLUG)
- Partner: [Piersoft](https://www.piersoft.it/), [Peacelink](https://www.peacelink.it/)
- Sito progetto: [jonixlug.altervista.org/jonixlug-aria-bene-comune](https://www.jonixlug.altervista.org/jonixlug-aria-bene-comune/)

V2 by **APS FareZero Makers Fab Lab** — [farezero.org](https://farezero.org)
