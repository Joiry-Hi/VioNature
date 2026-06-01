#!/usr/bin/env python3
"""Sync parameter values from English gameplay.cfg into Chinese gameplay_annotated.cfg.

Usage:
    python3 scripts/sync-config.py [--dry-run]

Reads each key=value from the English config and writes the same value to the
matching key line in the annotated Chinese config.  Comments, formatting, and
section headers are preserved.  Keys that appear in the English config but not
in the annotated config are reported.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ENGLISH = ROOT / "config" / "gameplay.cfg"
ANNOTATED = ROOT / "config" / "gameplay_annotated.cfg"

# Match a config line:  key  =  value   [# optional comment]
KEY_LINE = re.compile(r"^(\s*)([\w_]+)\s*=\s*(\S+)(.*)$")


def parse_english(path: Path) -> dict[str, str]:
    """Return {key: value_str} for every key=value line."""
    values: dict[str, str] = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            m = KEY_LINE.match(line.rstrip("\n"))
            if m:
                values[m.group(2)] = m.group(3)
    return values


def sync(english_values: dict[str, str], dry_run: bool = False) -> int:
    """Update annotated config in-place.  Returns number of changed lines."""
    lines = ANNOTATED.read_text(encoding="utf-8").splitlines(keepends=True)
    changed = 0
    seen = set()

    for i, line in enumerate(lines):
        m = KEY_LINE.match(line.rstrip("\n"))
        if not m:
            continue

        key = m.group(2)
        indent = m.group(1)
        old_val = m.group(3)
        rest = m.group(4)  # trailing comment (including leading spaces)

        if key not in english_values:
            continue

        seen.add(key)
        new_val = english_values[key]

        if old_val == new_val:
            continue

        changed += 1
        new_line = f"{indent}{key} = {new_val}{rest}\n"
        if dry_run:
            old_line = line.rstrip("\n")
            print(f"  {key}: {old_val}  →  {new_val}")
        else:
            lines[i] = new_line

    # Report keys in English config that have no match in annotated
    missing = set(english_values) - seen
    if missing:
        print(f"\n⚠ Keys in {ENGLISH.name} but missing from {ANNOTATED.name}:")
        for k in sorted(missing):
            print(f"  {k} = {english_values[k]}")

    if not dry_run:
        ANNOTATED.write_text("".join(lines), encoding="utf-8")
        print(f"Updated {changed} value(s) in {ANNOTATED.name}")

    return changed


def main() -> None:
    dry_run = "--dry-run" in sys.argv or "-n" in sys.argv

    if dry_run:
        print(f"[DRY RUN] Previewing changes from {ENGLISH.name} → {ANNOTATED.name}\n")

    english_values = parse_english(ENGLISH)
    print(f"Parsed {len(english_values)} keys from {ENGLISH.name}")

    changed = sync(english_values, dry_run)

    if changed == 0:
        print("All values already in sync.")
    elif dry_run:
        print(f"\n{dry_run and 'Would update' or 'Updated'} {changed} line(s). "
              f"Run without --dry-run to apply.")


if __name__ == "__main__":
    main()
