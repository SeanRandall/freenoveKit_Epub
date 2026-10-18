#!/usr/bin/env python3
"""Build the Markdown-backed Retroreader website documentation."""

from pathlib import Path

import markdown


ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
PAGES = {
    "preparing-the-sd-card.md": ("Preparing the SD card", "preparing-the-sd-card.html"),
    "wireless-file-transfer.md": ("Wireless file transfer", "wireless-file-transfer.html"),
    "using-the-reader.md": ("Using the reader", "using-the-reader.html"),
    "substitutions.md": ("Pronunciation substitutions", "substitutions.html"),
    "changelog.md": ("Changelog", "changelog.html"),
}


def navigation(current: str) -> str:
    links = [
        ("Home", "index.html"),
        ("Get a kit", "get-a-kit.html"),
        ("Why not?", "why-not.html"),
        ("Compare", "compare.html"),
        ("Voice samples", "voices.html"),
        ("Install", "install.html"),
        ("Documentation", "using-the-reader.html"),
        ("Changelog", "changelog.html"),
        ("About Sean", "about.html"),
    ]
    rendered = []
    for label, href in links:
        current_attr = ' aria-current="page"' if href == current else ""
        rendered.append(f'<a{current_attr} href="{href}">{label}</a>')
    rendered.append(
        '<a href="https://github.com/SeanRandall/freenoveKit_Epub">GitHub</a>'
    )
    return "".join(rendered)


def build_page(source: Path, title: str, output: Path) -> None:
    body = markdown.markdown(
        source.read_text(encoding="utf-8-sig"),
        extensions=["extra", "sane_lists"],
        output_format="html5",
    )
    page = f"""<!doctype html>
<html lang="en-GB">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>{title} — Retroreader</title>
  <link rel="stylesheet" href="style.css">
</head>
<body>
  <a class="skip-link" href="#content">Skip to content</a>
  <header class="site-header"><a class="brand" href="index.html">Retroreader</a><nav aria-label="Main navigation">{navigation(output.name)}</nav></header>
  <main id="content">
{body}
  </main>
  <footer><p>&copy; 2026 <a href="https://uk.linkedin.com/in/accessiblesean">Sean Randall</a>. Retroreader is an open-source project.</p></footer>
</body>
</html>
"""
    output.write_text(page, encoding="utf-8", newline="\n")


def main() -> None:
    for source_name, (title, output_name) in PAGES.items():
        build_page(DOCS / source_name, title, DOCS / output_name)


if __name__ == "__main__":
    main()
