#!/usr/bin/env python3
"""Extract spoken EVV Reader UI messages into gettext PO/POT catalogues.

The firmware does not link libintl.  PO is the translator-facing interchange
format; translated strings will be compiled into a small firmware catalogue.
This extractor deliberately follows announcement-building statements instead
of collecting diagnostics, paths and protocol strings from the binary.
"""

from __future__ import annotations

import argparse
import ast
import re
from datetime import date
from pathlib import Path

CALLS = re.compile(
    r"(?P<call>(?:strlcpy|strlcat|snprintf)\s*\(\s*reader_announcement\b.*?\);)",
    re.DOTALL,
)
DIRECT_SPEECH = re.compile(
    r"(?P<call>(?:reader_speak[^;(]*|speak_text[^;(]*|queue_[a-z_]*speech[^;(]*)\s*\(.*?\);)",
    re.DOTALL,
)
UI_ARRAY = re.compile(
    r"static\s+const\s+char\s*\*\s*const\s+"
    r"(?:unlock_names|sort_names|names|announcements)\s*"
    r"(?:\[[^]]*\])?\s*=\s*\{(?P<body>.*?)\};",
    re.DOTALL,
)
STRING = re.compile(r'"(?:\\.|[^"\\])*"')


def c_string(token: str) -> str:
    try:
        return ast.literal_eval(token)
    except (SyntaxError, ValueError):
        return ""


def po_quote(value: str) -> str:
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    escaped = escaped.replace("\t", "\\t").replace("\r", "\\r").replace("\n", "\\n")
    return f'"{escaped}"'


def candidates(path: Path) -> dict[str, set[int]]:
    text = path.read_text(encoding="utf-8", errors="replace")
    found: dict[str, set[int]] = {}
    for matcher in (CALLS, DIRECT_SPEECH):
        for match in matcher.finditer(text):
            statement = match.group("call")
            line = text.count("\n", 0, match.start()) + 1
            for token in STRING.findall(statement):
                value = c_string(token).strip()
                # Exclude format-only fragments and strings that cannot be
                # audible English UI.  Dynamic filenames and values remain
                # arguments to the surrounding translatable format string.
                letters = sum(ch.isalpha() for ch in value)
                if letters < 2 or value.startswith(("/", ".evv")):
                    continue
                found.setdefault(value, set()).add(line)
    for match in UI_ARRAY.finditer(text):
        line = text.count("\n", 0, match.start()) + 1
        for token in STRING.findall(match.group("body")):
            value = c_string(token).strip()
            if sum(ch.isalpha() for ch in value) >= 2:
                found.setdefault(value, set()).add(line)
    return found


def header(project: str, language: str = "") -> str:
    result = (
        '# EVV Reader user-interface messages.\n'
        '# Copyright (C) EVV Reader contributors\n'
        '# This file is distributed under the same license as EVV Reader.\n'
        'msgid ""\nmsgstr ""\n'
        f'"Project-Id-Version: {project}\\n"\n'
        f'"POT-Creation-Date: {date.today().isoformat()}\\n"\n'
        '"MIME-Version: 1.0\\n"\n'
        '"Content-Type: text/plain; charset=UTF-8\\n"\n'
        '"Content-Transfer-Encoding: 8bit\\n"\n'
    )
    if language:
        result += f'"Language: {language}\\n"\n'
    return result + "\n"


def write_catalog(output: Path, messages: dict[str, set[str]], language: str = "") -> None:
    chunks = [header("EVV Reader 0.3-alpha", language)]
    for message in sorted(messages, key=str.casefold):
        refs = " ".join(sorted(messages[message]))
        chunks.append(f"#: {refs}\nmsgid {po_quote(message)}\nmsgstr \"\"\n\n")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("".join(chunks), encoding="utf-8", newline="\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    source_dir = args.root / "firmware" / "main"
    messages: dict[str, set[str]] = {}
    for path in sorted(source_dir.glob("*.c")):
        for message, lines in candidates(path).items():
            refs = messages.setdefault(message, set())
            refs.update(f"firmware/main/{path.name}:{line}" for line in lines)
    translations = args.root / "translations"
    write_catalog(translations / "evvzero.pot", messages)
    french = translations / "fr.po"
    if not french.exists():
        write_catalog(french, messages, "fr")
    print(f"Extracted {len(messages)} UI messages")


if __name__ == "__main__":
    main()
