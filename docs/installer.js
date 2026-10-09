import { ESPLoader, Transport } from "https://unpkg.com/esptool-js@0.6.0/bundle.js";
const statusNode = document.querySelector("#status");
const progressNode = document.querySelector("#progress");
const installButton = document.querySelector("#install");
const deviceNode = document.querySelector("#device");

const readerImages = {
  "openevv:en-US": "reader-en-US.bin",
  "openevv:en-GB": "reader-en-GB.bin",
  "openevv:fr-FR": "reader-fr-FR.bin",
  "pico:en-US": "reader-pico-en-US.bin",
  "pico:en-GB": "reader-pico-en-GB.bin",
  "pico:de-DE": "reader-pico-de-DE.bin",
  "pico:es-ES": "reader-pico-es-ES.bin",
  "pico:fr-FR": "reader-pico-fr-FR.bin",
  "pico:it-IT": "reader-pico-it-IT.bin",
  "dectalk:dtc01-en-US": "reader-dectalk-dtc01-en-US.bin",
  "openbst:1998ENG": "reader-openbst-1998ENG.bin",
  "monologue:xen11k8": "reader-monologue-xen11k8.bin",
};

const filesFor = selection => [
  ["firmware/bootloader.bin", 0x0000],
  ["firmware/partition-table.bin", 0x8000],
  ["firmware/ota_data_initial.bin", 0xd000],
  [`firmware/${readerImages[selection]}`, 0x10000],
  ["firmware/network-services.bin", 0x610000],
];

async function loadFile([url, address]) {
  const response = await fetch(url, { cache: "no-cache" });
  if (!response.ok) throw new Error(`Could not download ${url}`);
  return { data: new Uint8Array(await response.arrayBuffer()), address };
}

installButton.addEventListener("click", async () => {
  let transport;
  try {
    if (!("serial" in navigator)) throw new Error("Use a current Chrome or Edge browser with Web Serial support.");
    installButton.disabled = true;
    progressNode.value = 0;
    statusNode.textContent = "Choose the Freenove serial port.";
    const port = await navigator.serial.requestPort();
    transport = new Transport(port, true);
    const terminal = { clean() {}, writeLine(line) { statusNode.textContent = line; }, write() {} };
    const loader = new ESPLoader({ transport, baudrate: 460800, terminal });
    statusNode.textContent = "Connecting.";
    const chip = await loader.main();
    if (!String(chip).toLowerCase().includes("esp32-s3")) throw new Error(`Expected ESP32-S3, found ${chip}`);
    const mac = await loader.chip.readMac(loader);
    const octets = mac.split(/[:-]/);
    const deviceId = octets.length >= 2
      ? `EVV${octets.slice(-2).join("").toUpperCase()}`
      : "EVV0000";
    deviceNode.innerHTML = `<strong>Device:</strong> ${deviceId} (ESP32-S3)`;
    statusNode.textContent = `Connected to ${deviceId}.`;
    const selectedFirmware = document.querySelector('input[name="firmware"]:checked');
    if (!selectedFirmware || !readerImages[selectedFirmware.value])
      throw new Error("Choose a speech engine and voice variant first.");
    statusNode.textContent = "Downloading firmware.";
    const fileArray = await Promise.all(filesFor(selectedFirmware.value).map(loadFile));
    await loader.writeFlash({
      fileArray,
      flashSize: "16MB",
      flashMode: "dio",
      flashFreq: "80m",
      eraseAll: false,
      compress: true,
      reportProgress(_index, written, total) {
        progressNode.value = total ? Math.round(written * 100 / total) : 0;
      },
    });
    await loader.after("hard_reset");
    statusNode.textContent = `Installation complete on ${deviceId}. The reader is restarting.`;
    progressNode.value = 100;
  } catch (error) {
    statusNode.textContent = `Installation failed: ${error.message || error}`;
  } finally {
    installButton.disabled = false;
    if (transport) try { await transport.disconnect(); } catch (_) {}
  }
});
