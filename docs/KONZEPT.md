# IRescue – Konzept

## 0. Vorab-Check: Ist es überhaupt Infrarot?

Viele Deckenlampen mit Fernbedienung funken per **2,4 GHz** (z. B. MiLight-artig)
oder **433 MHz** statt Infrarot. Dann läuft jede IR-Suche ins Leere.

Hinweise auf IR:
- Musste man mit der Fernbedienung auf die Lampe *zielen*? → IR
- Sichtbares dunkles/rotes Fenster bzw. „Knubbel“ am Lampenrand oder am Netzteil → IR-Empfänger
- Funktionierte die Fernbedienung durch Wände oder aus einem anderen Raum? → Funk
- Musste man die Fernbedienung „koppeln“? → fast sicher Funk

Außerdem lohnt ein Blick auf Typenschild/Modellnummer: Wenn das Modell bekannt
ist, gibt es den Code eventuell schon fertig in einer Datenbank.

## 1. Zeitbudget: Warum reines Bruteforce nur für NEC klappt

Günstige LED-Lampen verwenden ganz überwiegend das **NEC-Protokoll** bei 38 kHz.

Ein NEC-Frame: 9 ms Burst + 4,5 ms Pause + 32 Bits + Stoppbit.
Beim Standard-NEC sind das `Adresse, ~Adresse, Befehl, ~Befehl`, also immer
16 Einsen und 16 Nullen → konstant **≈ 67,5 ms**. Mit Pause bis zum nächsten
Frame rechnen wir **≈ 110 ms pro Versuch**.

| Suchraum | Kombinationen | Dauer (einfach gesendet) |
|---|---|---|
| Befehle bei bekannter Adresse | 256 | ≈ 30 s |
| Standard-NEC (8-Bit-Adresse) | 65 536 | ≈ 2 h |
| Extended NEC (16-Bit-Adresse) | 16,7 Mio. | ≈ 3 Wochen ❌ |
| andere Protokolle (RC5, RC6, Sony, …) | je nach Protokoll | eher Kandidatenliste |

→ **Standard-NEC komplett durchprobieren ist realistisch (eine Nacht).**
Extended NEC und exotische Protokolle nur über Listen bekannter Adressen.

## 2. Suchstrategie (in dieser Reihenfolge)

1. **Kandidatenliste aus Datenbanken** (Minuten)
   - [Flipper-IRDB](https://github.com/Lucaslhm/Flipper-IRDB): **CC0/Public
     Domain** → dürfen wir einbetten, umbauen und mit-veröffentlichen.
     Das ist unsere Standardquelle. Kategorien LED_Lighting, Fans.
   - [irdb](https://github.com/probonopd/irdb): **eigene Lizenz mit Auflagen**
     (bis zu 3 Geräte kostenlos stellen, GitHub-Issue eröffnen, Hinweis
     einbauen) → **nicht mit-ausliefern**, nur lokal herunterladen und nutzen.
   - [LIRC-Remotes](https://sourceforge.net/projects/lirc-remotes/): GPL
     (Copyleft) → nur unter GPL-Bedingungen, lokal herunterladen.
   - `tools/import_db.py` entdoppelt Flipper-IRDB zu einer Liste
     `(Protokoll, Adresse, Befehl)`, sortiert nach Häufigkeit → `candidates.json`.
     Details und Download-Orte: [../packs/README.md](../packs/README.md).
2. **Häufige NEC-Adressen × alle 256 Befehle** (Minuten)
   - Aus Schritt 1 ergeben sich die typischen Adressen (bei LED-Strips z. B.
     `0x00`, `0xEF00`/`0x00EF`-Varianten); jede Adresse kostet nur ≈ 30 s.
3. **Kompletter Standard-NEC-Sweep** (≈ 2 h, über Nacht)
4. **Extended NEC / andere Protokolle** nur mit Adressen aus den Datenbanken.

Ob ein Treffer da ist, merkt man sehr früh: Sobald **ein** Befehl blinkt, ist
die Adresse bekannt, und die restlichen Tasten findet man in 30 Sekunden.

## 3. Automatische Treffer-Erkennung

Die Lampe geht kurz aus, wenn sie einen Befehl erkennt – das ist das Orakel.

- **Sensor:** TEMT6000 (analog) oder Fototransistor am ADC des ESP32,
  Abtastung ~1 kHz. Ein LDR ist evtl. zu träge für ein kurzes Flackern.
- **Filter:** Lampen flimmern oft mit 100 Hz (Netz-/PWM-Ripple) → gleitender
  Mittelwert über 10–20 ms, dann Einbruch gegen eine Grundlinie erkennen.
- **Abschirmung:** Sensor nicht in den IR-Strahl halten (Silizium sieht 940 nm);
  am besten Sensor mit Röhrchen direkt an den Lampenschirm.
- **Latenz:** Die Lampe reagiert mit Verzögerung (Dekodieren + Reaktion,
  geschätzt 50–300 ms). Daher:
  - Schnell-Durchlauf; bei Blink-Ereignis die letzten *N* Codes merken
  - danach diese *N* Codes einzeln und langsam wiederholen → exakter Code
  - jeden Treffer mindestens 2× bestätigen, bevor er gespeichert wird
- **Nebenwirkungen:** Man trifft zwangsläufig auch „Aus“, „Nachtlicht“,
  „Dimmen auf Minimum“. Dauerhaft dunkel ≠ Blinken → als Treffer „Aus“
  werten, dann gezielt den Einschaltbefehl suchen (oft derselbe Toggle).

## 4. Hardware

```
ESP32 GPIO ──[1 kΩ]── Basis NPN (BC337 / 2N2222)
                      Kollektor ── IR-LED Kathode
                      Emitter   ── GND
5 V ──[≈ 33 Ω]── IR-LED Anode           (≈ 100 mA Pulsstrom)

TEMT6000 SIG ── ESP32 ADC1-Pin (z. B. GPIO34), VCC 3,3 V
TSOP38238 OUT ── ESP32 GPIO (optional, fürs Anlernen)
```

Genaue Werte hängen von LED und Transistor ab, das legen wir beim Aufbau fest.
ADC1-Pins verwenden, weil ADC2 mit aktivem WLAN nicht nutzbar ist.

## 5. Software

- **PlatformIO + Arduino-Framework**, Bibliothek **IRremoteESP8266**
  (unterstützt ESP32, Senden und Empfangen vieler Protokolle).
- **Web-UI von Anfang an** (entschieden): Der ESP32 (DevKit-C) macht einen
  eigenen Accesspoint `IRescue-XXXX` auf. Damit ist die Oberfläche ohne
  WLAN-Einrichtung erreichbar. UI zeigt Sensor live, Fortschritt,
  Pause/Weiter, Treffer testen und speichern, Anlernen.
  Umsetzung: ESPAsyncWebServer + WebSocket, UI im LittleFS.
- Fortschritt im NVS/Flash sichern, damit ein Sweep nach Stromausfall
  weiterläuft. *(noch TODO)*

## 6. Packs (Anlernen & Teilen)

**Entschieden:** eingelernte Fernbedienungen werden **je Gerät als ein eigenes
Pack** gespeichert (z. B. `laufband-remote.ir`, `wohnzimmer-lampe.ir`), nicht
in eine große gemeinsame Datenbank gemischt. Genau diese Geräte-Packs teilen
wir später auf GitHub. Format: das **Flipper-`.ir`-Format**. Es ist verbreitet,
menschenlesbar und Packs sind direkt mit Flipper Zero nutzbar:

```
Filetype: IR signals file
Version: 1
#
name: Power
type: parsed
protocol: NEC
address: 00 00 00 00
command: 45 00 00 00
```

Zusätzliche Metadaten (Hersteller, Modell, Foto der Fernbedienung, wer es
getestet hat) als Kommentarzeilen oder in einer `pack.json` daneben.
Rohsignale (`type: raw`) für unbekannte Protokolle.

Eigene Bruteforce-Treffer landen automatisch als Pack-Entwurf, so dass jeder
Fund später anderen hilft.

## Entscheidungen (2026-09-14)

- Lampe ist IR. ✔
- Board: ESP32 **DevKit-C** vorhanden. ✔
- **Web-UI von Anfang an** (AP-Modus). ✔
- Datenbank: **Flipper-IRDB (CC0)** einbetten/erweitern; irdb & LIRC nur lokal. ✔
- Packs **je Gerät einzeln** (z. B. „Laufband-Remote“). ✔

## Offene Fragen / TODO

- Bauteile: IR-LED-Typ, Transistor, Lichtsensor vorhanden? Sonst Einkaufsliste.
- Fortschritt über Stromausfall hinweg im NVS sichern.
- Extended-NEC-Byte-Layout beim Pack-Export.
- Nicht-NEC-Protokolle beim Anlernen sauber exportieren (aktuell nur roh).
