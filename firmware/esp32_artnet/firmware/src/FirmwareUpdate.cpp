#ifdef ARDUINO
#include "FirmwareUpdate.h"
#include <Arduino.h>
#include <Update.h>
#include <WebServer.h>
#include <esp_system.h>

namespace led {
namespace {
String token;
String uploadError;
size_t expectedBytes = 0;
bool started = false;
bool complete = false;

const char kUpdatePage[] PROGMEM = R"OTA(<!doctype html>
<html lang="de"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ArtNet LED Controller · Firmware-Update</title>
<style>body{font:15px system-ui;background:#11121b;color:#eee;max-width:620px;margin:7vh auto;padding:24px}button,input{font:inherit;margin:12px 0;padding:10px}button{background:#b198ff;border:0;border-radius:7px}a{color:#b198ff}progress{width:100%}</style>
<h1>Firmware-Update</h1><p>Eine passende <b>firmware.bin</b> auswählen. Gespeicherte LED-Einstellungen bleiben erhalten. Während des Updates die Stromversorgung eingeschaltet lassen.</p>
<form id="form"><input id="file" type="file" accept=".bin" required><br><button id="start">Firmware installieren</button></form>
<progress id="progress" max="100" value="0"></progress><p id="status" role="status">Bereit. Der Controller startet nach erfolgreicher Prüfung neu.</p><a href="/">Zurück zum Controller</a>
<script>
const form=document.getElementById('form'), status=document.getElementById('status');
form.onsubmit=async event=>{event.preventDefault();const file=document.getElementById('file').files[0];if(!file)return;
if(!file.name.toLowerCase().endsWith('.bin')||file.size<24||new Uint8Array(await file.slice(0,1).arrayBuffer())[0]!==0xe9){status.textContent='Bitte eine gültige ESP32 firmware.bin auswählen.';return;}
if(!confirm('Firmware installieren und Controller anschließend neu starten?'))return;
const button=document.getElementById('start');button.disabled=true;status.textContent='Firmware wird übertragen …';
const request=new XMLHttpRequest();request.open('POST','/update');request.timeout=180000;
request.setRequestHeader('X-OTA-Token','%TOKEN%');request.setRequestHeader('X-Firmware-Size',String(file.size));
request.upload.onprogress=e=>{if(e.lengthComputable)document.getElementById('progress').value=100*e.loaded/e.total;};
request.onload=()=>{button.disabled=false;if(request.status===200){status.textContent='Update geprüft und installiert. Neustart …';button.disabled=true;setTimeout(()=>location.href='/',12000);}else status.textContent='Update fehlgeschlagen: '+request.responseText;};
request.onerror=request.ontimeout=()=>{button.disabled=false;status.textContent='Verbindung unterbrochen. Controllerstatus prüfen, bevor du erneut startest.';};
const data=new FormData();data.append('firmware',file);request.send(data);};
</script></html>)OTA";

void failUpload(const char* message) {
  uploadError = message;
  if (Update.isRunning()) Update.abort();
}
}

void registerFirmwareUpdate(WebServer& server) {
  char randomToken[33];
  snprintf(randomToken, sizeof(randomToken), "%08lx%08lx%08lx%08lx",
           static_cast<unsigned long>(esp_random()), static_cast<unsigned long>(esp_random()),
           static_cast<unsigned long>(esp_random()), static_cast<unsigned long>(esp_random()));
  token = randomToken;
  const char* headers[] = {"X-OTA-Token", "X-Firmware-Size"};
  server.collectHeaders(headers, 2);
  server.on("/update", HTTP_GET, [&server]() {
    String page = FPSTR(kUpdatePage);
    page.replace("%TOKEN%", token);
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("X-Frame-Options", "DENY");
    server.send(200, "text/html; charset=utf-8", page);
  });
  server.on("/update", HTTP_POST, [&server]() {
    const bool authorized = server.header("X-OTA-Token") == token;
    const bool valid = authorized && started && complete && uploadError.isEmpty();
    bool success = false;
    if (valid) {
      success = Update.end();
      if (!success) uploadError = Update.errorString();
    }
    if (!success && Update.isRunning()) Update.abort();
    server.sendHeader("Cache-Control", "no-store");
    server.send(success ? 200 : (authorized ? 400 : 403), "text/plain; charset=utf-8",
                success ? String("Firmware installiert. Neustart.") :
                (uploadError.length() ? uploadError : String("Upload fehlt, ist unvollständig oder nicht freigegeben.")));
    started = complete = false;
    expectedBytes = 0;
    uploadError = "";
    if (success) {
      Serial.println("[update] firmware verified; rebooting");
      delay(500);
      ESP.restart();
    }
  }, [&server]() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      if (started) { failUpload("Nur eine Firmwaredatei pro Upload erlaubt."); return; }
      uploadError = "";
      complete = false;
      started = true;
      if (server.header("X-OTA-Token") != token) { failUpload("Update-Seite neu laden."); return; }
      const String sizeHeader = server.header("X-Firmware-Size");
      char* end = nullptr;
      expectedBytes = strtoul(sizeHeader.c_str(), &end, 10);
      if (sizeHeader.isEmpty() || *end != '\0' || expectedBytes < 24 ||
          expectedBytes > ESP.getFreeSketchSpace() || upload.name != "firmware" ||
          !upload.filename.endsWith(".bin")) {
        failUpload("Ungültige Firmwaredatei oder Datei zu groß."); return;
      }
      if (!Update.begin(expectedBytes, U_FLASH)) failUpload(Update.errorString());
    } else if (upload.status == UPLOAD_FILE_WRITE && uploadError.isEmpty() && started) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        failUpload(Update.errorString());
    } else if (upload.status == UPLOAD_FILE_END && uploadError.isEmpty() && started) {
      complete = upload.totalSize == expectedBytes;
      if (!complete) failUpload("Firmwaredatei nicht vollständig übertragen.");
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      failUpload("Upload abgebrochen.");
      started = complete = false;
    }
  });
}
}
#endif
