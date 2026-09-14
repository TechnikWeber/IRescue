#!/usr/bin/env python3
"""Build a candidate list for IRescue from Flipper-IRDB .ir files.

Flipper-IRDB is CC0 (public domain), so its codes may be embedded and shipped.
Point --src at a checkout (or a subfolder like .../Fans, .../LED_Lighting) and
this dedupes all parsed NEC codes, ranks them by how often they appear across
remotes, and writes a candidates.json for the firmware (flash into LittleFS).

    git clone https://github.com/Lucaslhm/Flipper-IRDB
    python3 tools/import_db.py --src Flipper-IRDB/LED_Lighting \\
        --out firmware/data/candidates.json

Then:  pio run -t uploadfs
"""
import argparse
import json
import pathlib
import re
from collections import Counter

# Flipper stores multi-byte fields little-endian as hex bytes; for standard NEC
# the meaningful byte is the first one.
HEX = re.compile(r"([0-9A-Fa-f]{2})")


def first_byte(field: str) -> int:
    m = HEX.search(field)
    return int(m.group(1), 16) if m else 0


def parse_ir_file(path: pathlib.Path):
    """Yield (protocol, address, command) for every parsed NEC signal."""
    name = proto = addr = cmd = None
    for line in path.read_text(errors="ignore").splitlines():
        line = line.strip()
        if line.startswith("name:"):
            name, proto, addr, cmd = line[5:].strip(), None, None, None
        elif line.startswith("protocol:"):
            proto = line.split(":", 1)[1].strip()
        elif line.startswith("address:"):
            addr = first_byte(line.split(":", 1)[1])
        elif line.startswith("command:"):
            cmd = first_byte(line.split(":", 1)[1])
            if proto and proto.upper() == "NEC" and addr is not None:
                yield ("NEC", addr, cmd, name)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", required=True, help="Flipper-IRDB dir to scan")
    ap.add_argument("--out", default="firmware/data/candidates.json")
    ap.add_argument("--limit", type=int, default=4000, help="max codes to keep")
    args = ap.parse_args()

    src = pathlib.Path(args.src)
    files = list(src.rglob("*.ir"))
    print(f"scanning {len(files)} .ir files under {src}")

    freq = Counter()          # (addr, cmd) -> how many remotes use it
    label = {}                # (addr, cmd) -> a human name we saw
    for f in files:
        seen = set()
        for _, addr, cmd, nm in parse_ir_file(f):
            key = (addr, cmd)
            if key not in seen:      # count each remote once
                freq[key] += 1
                seen.add(key)
            label.setdefault(key, nm or "")

    ranked = sorted(freq.items(), key=lambda kv: kv[1], reverse=True)[: args.limit]
    codes = [
        {"protocol": "NEC", "address": a, "command": c,
         "name": label[(a, c)], "seen": n}
        for (a, c), n in ranked
    ]

    out = pathlib.Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps({"codes": codes}, indent=1))
    print(f"wrote {len(codes)} unique NEC codes -> {out}")
    if codes:
        print("most common addresses:",
              Counter(c["address"] for c in codes).most_common(8))


if __name__ == "__main__":
    main()
