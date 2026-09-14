[English](README.md) · **Deutsch**

# IRescue

Die Infrarot-Fernbedienung von Lampe, LED-Streifen oder Ventilator verloren?
IRescue ist ein ESP32-Werkzeug, das **die verlorenen Codes per Bruteforce
wiederfindet** und später **Fernbedienungen anlernt und ihre Codes als Packs
teilt**, damit andere mit demselben Gerät einfach ein Pack laden können.

> Status: früher Prototyp. Firmware baut und flasht; Hardware-Test steht aus.

## Modi

- **Automatische Suche** mit dem Lichtsensor als Orakel — stoppt beim Blinken.
- **Blind-Modus** — ohne Sensor: schickt einfach alle Codes durch („Sensor-
  Rückmeldung nutzen" abwählen), du prüfst optisch selbst.
- **Manuell senden** — beliebige NEC-Adresse/-Befehl, oder beliebiges Protokoll
  + Hex-Wert, oder die Codeliste einzeln durchklicken und per Auge prüfen.
- **Live-Ansicht** — aktuell gesendeter Code, Fortschrittsbalken und ein
  laufendes Protokoll von allem (gesendet, empfangen, Treffer).
- **Anlernen** einer Fernbedienung (mit IR-Empfänger) und Speichern als Pack.

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

## Loslegen (ganz ohne Toolchain)

1. **Aus dem Browser flashen:** [**Web Flasher**](https://technikweber.github.io/IRescue/)
   in Chrome/Edge öffnen, ESP32 per USB anstecken, „Installieren“. Keine IDE,
   nichts installieren. Andere Wege (esptool, selbst bauen):
   [docs/FLASHEN.md](docs/FLASHEN.md).
2. **Verdrahten:** [docs/HARDWARE.md](docs/HARDWARE.md) · Einkaufsliste:
   [docs/EINKAUFSLISTE.md](docs/EINKAUFSLISTE.md).
3. **Benutzen:** mit dem **offenen** WLAN `IRescue-XXXX` verbinden — die
   Oberfläche öffnet sich von selbst (Captive Portal), sonst
   `http://192.168.4.1/`. Lichtsensor kalibrieren, dann Suche starten.

Fertige Binärdateien liegen in [`webflasher/firmware/`](webflasher/firmware/)
und werden bei jedem Push per CI neu gebaut — jeder kann flashen, kompilieren
und ändern.

## Repository-Aufbau

```
firmware/       PlatformIO-Projekt (ESP32, IRremoteESP8266, async Web-UI)
  src/          Firmware-Quellen (Engine, Oracle, Packs, Web)
  data/         Web-UI aus dem LittleFS (+ candidates.json)
webflasher/     Browser-Flasher (GitHub Pages) + fertige Binärdateien
packs/          Geräte-Packs (.ir, Flipper-kompatibel), eins pro Fernbedienung
tools/          import_db.py: Kandidatenliste aus einer Code-Datenbank bauen
docs/           Konzept, Hardware, Einkaufsliste, Flash- & Build-Anleitung
```

## Datenbanken & Lizenzen

Flipper-IRDB ist **CC0** und darf frei eingebettet/erweitert werden — das ist
die Standardquelle. `irdb` und LIRC haben restriktive/Copyleft-Lizenzen und
werden nur lokal genutzt. Details und Download-Orte: [packs/README.md](packs/README.md).

Das ausführliche Konzept steht in [docs/KONZEPT.md](docs/KONZEPT.md).
