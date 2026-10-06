# Translating EVV Reader

EVV Reader uses the widespread GNU gettext PO format for translation work.
The ESP firmware does not carry the gettext runtime: completed PO catalogues
will be converted to compact static C data during a release build.

`evvzero.pot` is the English message template. `fr.po` is the working French
catalogue. Translators can use Poedit, Weblate or any ordinary PO editor; they
should translate `msgstr` values and leave `msgid`, `%` format placeholders
and source-reference comments unchanged.

Developers can refresh the template from the spoken UI source with:

```powershell
python tools/extract_ui_strings.py
```

The script creates `fr.po` only when it does not already exist, so it cannot
erase a translator's work. After refreshing the template, merge new and
changed messages with GNU gettext or the equivalent command in a PO editor:

```powershell
msgmerge --update translations/fr.po translations/evvzero.pot
```

The extractor targets speech announcement construction, deliberately omitting
serial diagnostics, filesystem paths and network protocol text that users do
not hear.
