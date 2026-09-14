[English](README.md) · **Deutsch**

# IRescue

Die Infrarot-Fernbedienung von Lampe, LED-Streifen oder Ventilator verloren?
IRescue ist ein ESP32-Werkzeug, das **die verlorenen Codes per Bruteforce
wiederfindet** und später **Fernbedienungen anlernt und ihre Codes als Packs
teilt**, damit andere mit demselben Gerät einfach ein Pack laden können.

> Status: Konzept / früher Prototyp, noch nichts benutzbar.

## Funktionsweise

1. **Zuerst bekannte Codes:** Codelisten aus öffentlichen Datenbanken
   (Flipper-IRDB, IRDB, LIRC) abspielen, die wahrscheinlich passen
   (LED-Lampen, LED-Streifen).
2. **Protokoll-Sweep:** alle Kombinationen des häufigsten Protokolls
   systematisch senden (NEC: 256 Adressen × 256 Befehle ≈ 2 Stunden).
3. **Automatische Erkennung:** Ein Lichtsensor beobachtet die Lampe. Viele
   Lampen blinken kurz, wenn sie einen Befehl annehmen. IRescue bemerkt das,
   grenzt den genauen Code ein und protokolliert ihn.
4. **Anlernen & Teilen (geplant):** Ein IR-Empfänger nimmt vorhandene
   Fernbedienungen auf; das Ergebnis wird als Pack im Flipper-kompatiblen
   `.ir`-Format gespeichert.

## Hardware (Prototyp)

| Bauteil | Zweck |
|---|---|
| ESP32-Entwicklungsboard | Steuerung, 38-kHz-Träger über die RMT-Peripherie |
| IR-LED 940 nm (z. B. TSAL6200) + NPN-Transistor/MOSFET + Widerstand | Senden |
| Lichtsensor (z. B. TEMT6000 oder Fototransistor) | Erkennt das Blinken der Lampe |
| IR-Empfänger TSOP38238 / VS1838B (optional) | Fernbedienungen anlernen |

## Schnellstart

```
# 1. Kandidatenliste aus dem CC0-Flipper-IRDB bauen (optional, aber schnell)
git clone https://github.com/Lucaslhm/Flipper-IRDB ~/ir-databases/Flipper-IRDB
python3 tools/import_db.py --src ~/ir-databases/Flipper-IRDB/LED_Lighting \
    --out firmware/data/candidates.json

# 2. Firmware + Web-UI flashen
cd firmware
pio run -t upload      # Firmware
pio run -t uploadfs    # Web-UI (data/ -> LittleFS)

# 3. mit dem WLAN "IRescue-XXXX" verbinden (Passwort: irescue123)
#    http://192.168.4.1/ öffnen -> Sensor kalibrieren -> Suche starten
```

## Repository-Aufbau

```
firmware/       PlatformIO-Projekt (ESP32, IRremoteESP8266, async Web-UI)
  src/          Firmware-Quellen (Engine, Oracle, Packs, Web)
  data/         Web-UI aus dem LittleFS (+ candidates.json)
packs/          Geräte-Packs (.ir, Flipper-kompatibel), eins pro Fernbedienung
tools/          import_db.py: Kandidatenliste aus einer Code-Datenbank bauen
docs/           Konzept und Notizen
```

## Datenbanken & Lizenzen

Flipper-IRDB ist **CC0** und darf frei eingebettet/erweitert werden — das ist
die Standardquelle. `irdb` und LIRC haben restriktive/Copyleft-Lizenzen und
werden nur lokal genutzt. Details und Download-Orte: [packs/README.md](packs/README.md).

Das ausführliche Konzept steht in [docs/KONZEPT.md](docs/KONZEPT.md).
