# Third-party software

`vendor/openevv-perf` is derived from OpenEVV by Stanislaw Przedzinkowski and
is distributed under the MIT License included in that directory. The local
tree contains ESP32 portability and performance modifications used by this
firmware.

ESP-IDF and its managed components are downloaded separately during a source
build and retain their respective licences. They are not vendored here.

`vendor/dectalk43` references the DECtalk 4.3 source maintained by the DECtalk
project. Retroreader's ESP32 compatibility overrides are stored in
`firmware/main/dectalk43_overrides`. The upstream licence and notices remain in
the referenced source tree.

The experimental DoubleTalk Hybrid engine uses the board and 80C188 emulation
from David Sexton's `doubletalk-pc` project, preserved in
`vendor/doubletalk-pc-port`, under its BSD 3-Clause licence.
DoubleTalk is a product and trademark of RC Systems. The native acoustic
renderer in `vendor/doubletalk-native` is part of Retroreader.
