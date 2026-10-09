document.querySelector("#contact-form").addEventListener("submit", event => {
  event.preventDefault();
  const data = new FormData(event.currentTarget);
  const subject = String(data.get("subject") || "Retroreader enquiry").trim();
  const name = String(data.get("name") || "").trim();
  const message = String(data.get("message") || "").trim();
  const url = new URL("https://github.com/SeanRandall/freenoveKit_Epub/issues/new");
  url.searchParams.set("title", subject);
  url.searchParams.set("body", `From: ${name}\n\n${message}`);
  document.querySelector("#contact-status").textContent = "Opening GitHub to review your message.";
  window.location.href = url.toString();
});
