from pathlib import Path
from html import escape
import re
from zipfile import ZIP_DEFLATED, ZIP_STORED, ZipFile

root = Path(__file__).resolve().parents[1]
output = root / "sd-card" / "EVV Reader Guide.epub"
physical_description = root / "Hardware physical description.md"


def inline_markdown(text: str) -> str:
    """Convert the small inline Markdown subset used by the description."""
    text = text.replace("**", "")
    parts = []
    position = 0
    for match in re.finditer(r"\[([^]]+)\]\(([^)]+)\)", text):
        parts.append(escape(text[position:match.start()]))
        parts.append(
            f'<a href="{escape(match.group(2), quote=True)}">'
            f'{escape(match.group(1))}</a>'
        )
        position = match.end()
    parts.append(escape(text[position:]))
    return "".join(parts)


def markdown_body(markdown: str) -> str:
    """Make accessible XHTML from the deliberately simple source document."""
    output_lines = []
    paragraph = []
    in_list = False

    def close_paragraph() -> None:
        if paragraph:
            output_lines.append(f"<p>{inline_markdown(' '.join(paragraph))}</p>")
            paragraph.clear()

    def close_list() -> None:
        nonlocal in_list
        if in_list:
            output_lines.append("</ul>")
            in_list = False

    for raw_line in markdown.replace("\u00a0", " ").splitlines():
        line = raw_line.strip()
        if not line:
            close_paragraph()
            close_list()
            continue
        heading = re.match(r"^(#{1,6})\s+(.+)$", line)
        if heading:
            close_paragraph()
            close_list()
            level = min(len(heading.group(1)), 3)
            output_lines.append(
                f"<h{level}>{inline_markdown(heading.group(2))}</h{level}>"
            )
            continue
        bullet = re.match(r"^(?:\\?\*)\s+(.+)$", line)
        if bullet:
            close_paragraph()
            if not in_list:
                output_lines.append("<ul>")
                in_list = True
            output_lines.append(f"<li>{inline_markdown(bullet.group(1))}</li>")
            continue
        close_list()
        paragraph.append(line)

    close_paragraph()
    close_list()
    return "\n".join(output_lines)

mimetype = "application/epub+zip"
container = """<?xml version="1.0" encoding="UTF-8"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles><rootfile full-path="OEBPS/content.opf" media-type="application/oebps-package+xml"/></rootfiles>
</container>
"""
package = """<?xml version="1.0" encoding="UTF-8"?>
<package xmlns="http://www.idpf.org/2007/opf" version="3.0" unique-identifier="book-id">
  <metadata xmlns:dc="http://purl.org/dc/elements/1.1/">
    <dc:identifier id="book-id">urn:evv:alpha-guide</dc:identifier>
    <dc:title>EVV Reader Guide</dc:title>
    <dc:language>en-GB</dc:language>
    <dc:creator>EVVZero contributors</dc:creator>
    <meta property="dcterms:modified">2026-10-01T00:00:00Z</meta>
  </metadata>
  <manifest>
    <item id="guide" href="guide.xhtml" media-type="application/xhtml+xml"/>
    <item id="physical" href="physical.xhtml" media-type="application/xhtml+xml"/>
  </manifest>
  <spine><itemref idref="guide"/><itemref idref="physical"/></spine>
</package>
"""
guide = """<?xml version="1.0" encoding="UTF-8"?>
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-GB">
<head><title>EVV Reader Guide</title></head>
<body>
<h1>EVV Reader Guide</h1>
<p>This alpha firmware turns the Freenove Media Kit for ESP32-S3 into a speech-first EPUB reader. Hold the unit with the screen at the top and the USB-C connector at the bottom.</p>

<h2>The library</h2>
<p>Up and down move through books. Right repeats the current title. Centre opens the selected book and begins reading. Hold Up to open settings. Hold Left returns to the previously loaded reading without regenerating it.</p>

<h2>Reading</h2>
<p>Centre pauses or resumes. During playback, Up raises volume and Down lowers it. Left and Right move backward or forward by a sentence. Hold Centre locks or unlocks the keypad.</p>
<p>While paused, Left and Right select the previous or next book section. Down moves through the enabled status announcements. Hold Left opens the library. Hold Up opens settings. Hold Right begins a recording; the next button press stops and saves it.</p>

<h2>Settings</h2>
<p>Up and Down move through the menu. Left and Right change the focused setting. Centre activates an action or toggles a setting. Hold Up closes the menu. Playing-screen status items opens a submenu in which Centre enables or disables an item and Left or Right moves it in the announcement order.</p>
<p>Settings include speech rate, pronunciation substitutions, dictionary commands, interface sounds, status items, pauses between sections, startup behaviour, the sleep timer, file transfer and NVDA Remote.</p>

<h2>File transfer</h2>
<p>File transfer connects to the Wi-Fi details stored in dot evv slash WIFI dot INI and starts a local WebDAV server. Its name is EVV followed by four hexadecimal characters. Hold Centre deliberately to leave transfer mode and return to the library.</p>

<h2>Configuration and privacy</h2>
<p>Wi-Fi and NVDA Remote details belong in the hidden dot evv directory on the SD card. They are not built into this firmware. File transfer is unauthenticated and should only be used on a trusted private network.</p>

<h2>Alpha limitations</h2>
<p>This is test software. Keep backup copies of books and recordings. Speech generation can take several seconds when a book is first opened or restored after a cold boot. Please report the action being performed, the spoken message and whether the unit recovered after reset when describing a fault.</p>
</body></html>
"""

physical = f"""<?xml version="1.0" encoding="UTF-8"?>
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-GB">
<head><title>Freenove kit physical overview</title></head>
<body>
{markdown_body(physical_description.read_text(encoding="utf-8-sig"))}
</body></html>
"""

output.parent.mkdir(parents=True, exist_ok=True)
with ZipFile(output, "w") as book:
    book.writestr("mimetype", mimetype, compress_type=ZIP_STORED)
    book.writestr("META-INF/container.xml", container, compress_type=ZIP_DEFLATED)
    book.writestr("OEBPS/content.opf", package, compress_type=ZIP_DEFLATED)
    book.writestr("OEBPS/guide.xhtml", guide, compress_type=ZIP_DEFLATED)
    book.writestr("OEBPS/physical.xhtml", physical, compress_type=ZIP_DEFLATED)

print(output)
