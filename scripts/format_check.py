#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import sys


def iter_source_files(root: pathlib.Path):
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.suffix in {".c", ".h"}:
            yield path


def check_file(path: pathlib.Path) -> list[str]:
    errors: list[str] = []
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()

    for idx, line in enumerate(lines, start=1):
        if line.rstrip(" \t") != line:
            errors.append(f"{path}:{idx}: trailing whitespace")
        if "\t" in line:
            errors.append(f"{path}:{idx}: tab character found")
        if len(line) > 100:
            errors.append(f"{path}:{idx}: line longer than 100 characters")

    if text and not text.endswith("\n"):
        errors.append(f"{path}: missing final newline")

    return errors


def main(argv: list[str]) -> int:
    roots = [pathlib.Path(arg) for arg in argv[1:]]
    if not roots:
        print("usage: format_check.py <path> [<path> ...]", file=sys.stderr)
        return 2

    errors: list[str] = []
    for root in roots:
        if not root.exists():
            errors.append(f"missing path: {root}")
            continue
        for file_path in iter_source_files(root):
            errors.extend(check_file(file_path))

    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1

    print("format-check: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
