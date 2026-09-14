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

## Repository-Aufbau (geplant)

```
firmware/   ESP32-Firmware (PlatformIO, IRremoteESP8266)
packs/      Code-Packs pro Gerät (.ir, Flipper-kompatibel)
tools/      Skripte: Code-Datenbanken importieren/entdoppeln, Kandidatenlisten bauen
docs/       Konzept und Notizen
```

Das ausführliche Konzept steht in [docs/KONZEPT.md](docs/KONZEPT.md).
