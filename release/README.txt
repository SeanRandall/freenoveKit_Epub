freenoveKit_Epub alpha firmware

The easiest installation uses freenoveKit_Epub-alpha.bin at address 0x0.
That merged image contains both the reader and the file-transfer/NVDA Remote
partition. The individual binary files are retained for developers using
ESP-IDF's normal multi-image flashing process; network-transfer.bin belongs
at flash address 0x610000.

Target: Freenove Media Kit for ESP32-S3, 1.14-inch model, 16 MB flash.

SHA-256 for freenoveKit_Epub-alpha.bin:
df6acba9475ca600feb3ca7b3e754cb99bb31cc7eaaf16ffe541567a79a109b0

