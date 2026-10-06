from pathlib import Path
from html import escape
import re
from zipfile import ZIP_DEFLATED, ZIP_STORED, ZipFile

root = Path(__file__).resolve().parents[1]
output = root / "sd-card" / "EVV Reader Guide.epub"
physical_description = root / "Hardware physical description.md"
reader_guide = root / "EVV Reader Guide.md"
web_directory = root / "docs"


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


def web_document(title: str, body: str) -> str:
    """Wrap converted Markdown in the accessible installer-page layout."""
    return f"""<!doctype html>
<html lang="en-GB">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>{escape(title)}</title>
  <link rel="stylesheet" href="style.css">
</head>
<body>
  <main>
    <nav aria-label="Documentation">
      <a href="index.html">Firmware installer</a> |
      <a href="reader-guide.html">Reader guide</a> |
      <a href="physical-description.html">Physical description</a>
    </nav>
{body}
  </main>
</body>
</html>
"""

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
guide = f"""<?xml version="1.0" encoding="UTF-8"?>
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-GB">
<head><title>EVV Reader Guide</title></head>
<body>
{markdown_body(reader_guide.read_text(encoding="utf-8-sig"))}
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

web_directory.mkdir(parents=True, exist_ok=True)
web_directory.joinpath("reader-guide.html").write_text(
    web_document(
        "EVV Reader Guide",
        markdown_body(reader_guide.read_text(encoding="utf-8-sig")),
    ),
    encoding="utf-8",
    newline="\n",
)
web_directory.joinpath("physical-description.html").write_text(
    web_document(
        "Freenove kit physical description",
        markdown_body(physical_description.read_text(encoding="utf-8-sig")),
    ),
    encoding="utf-8",
    newline="\n",
)

print(output)
