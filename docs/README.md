# Retroreader website and web installer

This directory is ready to publish with GitHub Pages. Select the repository's
`main` branch and `/docs` folder under **Settings, Pages**.

The installer uses Espressif's `esptool-js` over Web Serial. It writes only the
five required flash regions and does not erase the whole chip, preserving NVS
preferences during upgrades. Its firmware assets are generated during the
release build and stored under `docs/firmware`.

The page requires HTTPS and a Web Serial browser such as Chrome or Edge. The
page is deliberately ordinary HTML.
