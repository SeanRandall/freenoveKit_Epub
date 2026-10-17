# Retroreader changelog

Retroreader remains alpha software. Releases may change settings, storage
layouts and behaviour without preserving compatibility with earlier builds.

## 2026.10.17 — pre-release

- Add the British Dolphin Apollo II speech synthesizer, reconstructed from the
  original hardware firmware and rendered natively on the ESP32-S3.
- Provide Apollo controls for rate, voice volume, pitch, its male and non-male
  speaker tables, and expression. The original Apollo deliberately reduces
  expression at faster rates.
- Stream Apollo speech as it is synthesized for responsive, interruptible
  menus and reading, while yielding regularly to the reader interface.
- Fix a crash when moving through engine-specific voice settings.
- Add Apollo to the web installer, downloadable firmware, voice samples and
  source-provenance information.

## 2026.10.16 — pre-release

This is the first date-numbered Retroreader release and a substantial change
to how speech engines and firmware images are packaged.

### Highlights

- Use the chosen speech engine consistently across the reader, menus, file
  transfer and NVDA Remote, rather than falling back to OpenEVV in network
  modes.
- Provide one complete firmware image for each engine and voice. The web
  installer and downloadable files now use the exact same images.
- Add OpenEVV pause modes: always pause, pause at sentence endings, or disable
  automatic pauses. Eloquence fans will want to set this to always for a faster reading experience, which sounds counterintuitive.
- Improve long-sentence splitting so speech is not unnecessarily broken in
  the middle of a sentence. Examples of this were rare, but it broke my flow.
- Add an accessible browser file manager alongside WebDAV file transfer, with
  browsing, upload, download, rename, folder creation and deletion. Opening the
  reader's address in a web browser forwards to the file manager automatically.
- Expand the website documentation, installation guidance, speech samples and
  measured speaking-rate information.

### Identity and storage changes

- Rename product-specific identifiers from EVV to Retroreader or RR. OpenEVV
  remains the name of one available speech engine.
- Change device hostnames from `EVVxxxx` to `RRxxxx`.
- Change the hidden SD-card settings folder from `.evv` to `.rr`.
- Supply a new SD-card template for the revised layout. Earlier alpha layouts
  are not supported by this release.

### Speech engines

Complete images are prepared for:

- OpenEVV (Eloquence), US and British English;
- SVOX Pico, US and British English;
- DECtalk DTC-01;
- DECtalk 4.3;
- OpenBST (Keynote Gold), 1998 English;
- Monologue XEN11K8; and
- TruVoice US English.

An additional experimental image adds DoubleTalk Hybrid, based on the
early-1990s RC Systems DoubleTalk PC. It retains the original firmware front
end for dictionary processing, pronunciation and prosody while replacing the
sample-by-sample acoustic work with a native ESP32 renderer. Its speech menu
includes rate, pitch, voice, tone, articulation, expression and a working
faster-than-stock rate boost.

### Packaging

- Introduce a dated firmware manifest as the single catalogue used by release
  builds, direct downloads and the browser installer.
- Replace separate bootloader, reader and network-service downloads with one
  complete `.bin` file per engine and voice.
- Rebuild the standalone Windows installer for the complete images.
- Add SHA-256 checksums for downloadable firmware.

## 0.5 alpha — 13 October 2026

- Added TruVoice US English to the public installer and downloads.
- Added accessible browser-based SD-card file management.
- Expanded the voice-sample page with approximate speaking rates.
- Expanded website and SD-card documentation.

## 0.4 alpha — 9 October 2026

- Renamed the project from EVV Reader to Retroreader.
- Added a modular speech-engine interface and public engine selection.
- Added Monologue XEN11K8 with rate, pitch, voice-volume, dictionary and pause
  controls.
- Added DECtalk DTC-01, OpenBST 1998 English and SVOX Pico builds alongside
  OpenEVV.
- Added British English OpenEVV and Pico builds.
- Launched the Retroreader website, cross-platform web installer and speech
  sample pages.

## 0.3 alpha — 6 October 2026

- Added folder-aware library browsing, recent files and configurable sorting.
- Added plain-text, MP3, M4A and M4B support.
- Added improved EPUB indexing, SD-card caching and support for much larger
  books.
- Expanded navigation, status and reader settings.
- Improved recording reliability and keypad responsiveness.
- Added conditional File Transfer, NVDA Remote and RemSound modes, shown only
  when their configuration files are present.
- Added the browser-based Web Serial installer.
- Included clean Wi-Fi, NVDA Remote and RemSound configuration examples in the
  SD-card template.

## 0.2 alpha — 1 October 2026

- Added the full SD-card template to the downloadable package.
- Added example configuration files and the reader guide.
- Added my tactile physical description of the Freenove kit to the
  guide.

## 0.1 alpha — 1 October 2026

- First public alpha for the Freenove Media Kit for ESP32-S3, 1.14-inch model.
- Included a standalone Windows installer and complete US English, British
  English and French-voice OpenEVV firmware images.
- The French voice retained the English user interface.

