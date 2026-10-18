RetroReader configuration directory

Wi-Fi:
Rename WIFI.INI.example to WIFI.INI and replace both placeholder values.

NVDA Remote:
Rename nvdaremote.ini.example to nvdaremote.ini and replace the host, port
and key. The key is a secret and must not be committed or shared.

RemSound:
Rename remsound.ini.example to remsound.ini and replace the host, port and
password with the values from the RemSound sender profile.

Optional global pronunciation substitutions may be stored in the tab-separated
file substitutions.tsv. Engine-specific files such as
substitutions.openevv.tsv and substitutions.dectalk4.3.tsv add or override
entries only for that engine. Each line contains the text to find, a tab
character, and its replacement. The combined limit is 160 entries. See the
Pronunciation substitutions documentation for phoneme alphabets, modifiers,
and the readable escape notation used by control-character engines.


