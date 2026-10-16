import { ESPLoader, Transport } from "https://unpkg.com/esptool-js@0.6.0/bundle.js";
const statusNode = document.querySelector("#status");
const progressNode = document.querySelector("#progress");
const installButton = document.querySelector("#install");
const deviceNode = document.querySelector("#device");
const optionsNode = document.querySelector("#firmware-options");
const downloadNode = document.querySelector("#firmware-download");

let manifest;
let variants = new Map();

const filesFor = variant => [
  [`firmware/${variant.image}`, 0x0000],
];

function updateDownload() {
  const selected = document.querySelector('input[name="firmware"]:checked');
  const variant = selected && variants.get(selected.value);
  if (!variant) {
    downloadNode.hidden = true;
    return;
  }
  downloadNode.href = `firmware/${variant.image}`;
  downloadNode.download = variant.image;
  downloadNode.textContent = `Download ${variant.label} complete firmware image`;
  downloadNode.hidden = false;
}

function renderVariants(data) {
  optionsNode.replaceChildren();
  const engines = new Map();
  for (const variant of data.variants) {
    variants.set(variant.id, variant);
    if (!engines.has(variant.engine)) engines.set(variant.engine, []);
    engines.get(variant.engine).push(variant);
  }
  for (const [engine, choices] of engines) {
    const details = document.createElement("details");
    if (choices.some(choice => choice.default)) details.open = true;
    const summary = document.createElement("summary");
    summary.textContent = engine;
    const fieldset = document.createElement("fieldset");
    const legend = document.createElement("legend");
    legend.textContent = `${engine} voice variant`;
    fieldset.append(legend);
    for (const choice of choices) {
      const label = document.createElement("label");
      const radio = document.createElement("input");
      radio.type = "radio";
      radio.name = "firmware";
      radio.value = choice.id;
      radio.checked = Boolean(choice.default);
      radio.addEventListener("change", updateDownload);
      label.append(radio, ` ${choice.label}`);
      fieldset.append(label);
    }
    details.append(summary, fieldset);
    optionsNode.append(details);
  }
  updateDownload();
}

installButton.disabled = true;
fetch("firmware-manifest.json", { cache: "no-cache" })
  .then(response => {
    if (!response.ok) throw new Error("Could not load the firmware list.");
    return response.json();
  })
  .then(data => {
    manifest = data;
    renderVariants(data);
    statusNode.textContent = `Ready to install ${data.version}.`;
    installButton.disabled = false;
  })
  .catch(error => {
    statusNode.textContent = `Installer unavailable: ${error.message || error}`;
  });

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
      ? `RR${octets.slice(-2).join("").toUpperCase()}`
      : "RR0000";
    deviceNode.innerHTML = `<strong>Device:</strong> ${deviceId} (ESP32-S3)`;
    statusNode.textContent = `Connected to ${deviceId}.`;
    const selectedFirmware = document.querySelector('input[name="firmware"]:checked');
    const variant = selectedFirmware && variants.get(selectedFirmware.value);
    if (!variant)
      throw new Error("Choose a speech engine and voice variant first.");
    statusNode.textContent = "Downloading firmware.";
    const fileArray = await Promise.all(filesFor(variant).map(loadFile));
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
