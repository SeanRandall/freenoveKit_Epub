freenoveKit_Epub alpha firmware

The easiest installation uses freenoveKit_Epub-alpha.bin at address 0x0.
That merged image contains both the reader and the file-transfer/NVDA Remote
partition. The individual binary files are retained for developers using
ESP-IDF's normal multi-image flashing process; network-transfer.bin belongs
at flash address 0x610000.

Target: Freenove Media Kit for ESP32-S3, 1.14-inch model, 16 MB flash.

SHA-256 for freenoveKit_Epub-alpha.bin:
46b44b88e1fc71d4a7cb6e3473ae04615b5ef117234ad2c57644723a7fbdf699

