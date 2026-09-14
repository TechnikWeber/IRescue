# Packs

Two different kinds of files live here — don't mix them up.

## 1. Device packs (`.ir`) — one per remote/device

Each learned remote becomes **one** device-specific pack, e.g.
`laufband-remote.ir`, `wohnzimmer-lampe.ir`. This is the Flipper Zero `.ir`
format, so the packs also work directly on a Flipper. IRescue writes these when
you save a learned code or a brute-force hit. These are the packs meant to be
shared on GitHub so others who lost the same remote can just load them.

## 2. Candidate list (`candidates.json`) — for the brute-force "known codes" mode

Built by `tools/import_db.py` from a code database and flashed into the ESP32's
LittleFS as `/candidates.json`. Not stored here in git; see the license notes.

## Where the source databases come from

| Database | License | May we ship it? | Where to get it |
|---|---|---|---|
| [Flipper-IRDB](https://github.com/Lucaslhm/Flipper-IRDB) | **CC0** (public domain) | **Yes** — embed & extend freely | `git clone` it, run the import tool |
| [irdb](https://github.com/probonopd/irdb) | custom, with obligations | **No** — download locally only | see below |
| [LIRC remotes](https://sourceforge.net/projects/lirc-remotes/) | GPL (copyleft) | only under GPL terms | download locally only |

So: **Flipper-IRDB is our default source** and its codes can go straight into a
shipped `candidates.json`. `irdb` and LIRC you download to your own machine and
feed to the import tool locally, but don't commit the result.

### Local download locations

```
# Flipper-IRDB (CC0 — the one we build from)
git clone https://github.com/Lucaslhm/Flipper-IRDB ~/ir-databases/Flipper-IRDB

# irdb (local use only, do not commit)
git clone https://github.com/probonopd/irdb ~/ir-databases/irdb
```

Then build the candidate list, e.g. only lighting codes:

```
python3 tools/import_db.py --src ~/ir-databases/Flipper-IRDB/LED_Lighting \
    --out firmware/data/candidates.json
pio run -t uploadfs
```
