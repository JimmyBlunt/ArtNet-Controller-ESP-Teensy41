#include <Arduino.h>
#include <ArduinoJson.h>
#include <QNEthernet.h>
#include "web_http.h"
#include "http_request.h"
#include "web_config_json.h"
#include "web_controller.h"
#include "web_assets.h"

namespace http {
namespace {
using qindesign::network::EthernetClient;
using qindesign::network::EthernetServer;
EthernetServer server(80);
constexpr size_t kBudget = 512;
constexpr uint32_t kIdleMs = 1500, kTotalMs = 5000;
struct Slot {
    EthernetClient client;
    bool used = false, headers = false, sending = false, finishing = false;
    uint32_t started = 0, progressed = 0;
    char request[http_request::kMaxHeader + http_request::kMaxBody + 1] = {};
    size_t received = 0, bodyOffset = 0;
    http_request::Request parsed;
    char response[8192] = {};
    char header[512] = {};
    size_t headerLength = 0, headerSent = 0, bodyLength = 0, bodySent = 0;
    const uint8_t* body = nullptr;
};
Slot slots[2];
StaticJsonDocument<12288> json;
uint32_t requests = 0, timeouts = 0, rejected = 0, pollMaxUs = 0;
uint32_t timeoutBytes = 0, timeoutTail = 0;
bool timeoutHeaders = false;
void close(Slot& slot) {
    slot.client.close();  // QNEthernet nonblocking close; never flush/writeFully.
    slot.client = EthernetClient();
    slot.used = false;
}
void response(Slot& slot, unsigned code, const char* type, const uint8_t* data,
              size_t size, bool gzip = false) {
    slot.body = data;
    slot.bodyLength = size;
    slot.bodySent = slot.headerSent = 0;
    const char* reason = code == 200 ? "OK" : "Request Rejected";
    int n = snprintf(slot.header, sizeof(slot.header),
        "HTTP/1.1 %u %s\r\nContent-Type: %s\r\nContent-Length: %u\r\n"
        "Connection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n"
        "Content-Security-Policy: default-src 'self'; style-src 'self' 'unsafe-inline'; frame-ancestors 'none'\r\n%s\r\n",
        code, reason, type, unsigned(size), gzip ? "Content-Encoding: gzip\r\nVary: Accept-Encoding\r\n" : "");
    if (n < 0 || size_t(n) >= sizeof(slot.header)) { close(slot); return; }
    slot.headerLength = size_t(n);
    slot.sending = true;
}
void error(Slot& slot, unsigned code, const char* message) {
    ++rejected;
    json.clear();
    json["ok"] = false;
    json["error"] = message;
    const size_t n = serializeJson(json, slot.response, sizeof(slot.response));
    response(slot, code, "application/json", reinterpret_cast<const uint8_t*>(slot.response), n);
}
void sendJson(Slot& slot) {
    if (json.overflowed() || measureJson(json) >= sizeof(slot.response)) {
        error(slot, 500, "RESPONSE_CAPACITY"); return;
    }
    const size_t n = serializeJson(json, slot.response, sizeof(slot.response));
    response(slot, 200, "application/json", reinterpret_cast<const uint8_t*>(slot.response), n);
}
// Read-only, length-bounded JSON reader also detects trailing non-whitespace.
struct Input {
    const char* data; size_t length, position = 0;
    int read() { return position < length ? static_cast<unsigned char>(data[position++]) : -1; }
    int peek() { return position < length ? static_cast<unsigned char>(data[position]) : -1; }
    size_t readBytes(char* target, size_t n) {
        const size_t remaining = length - position;
        if (n > remaining) n = remaining;
        memcpy(target, data + position, n); position += n; return n;
    }
};
void dispatch(Slot& slot) {
    ++requests;
    json.clear();
    const char* path = slot.parsed.path;
    if (!strcmp(slot.parsed.method, "GET")) {
        if (!strcmp(path, "/api/config")) {
            webcfg::toJson(controller::config(), json.to<JsonObject>());
        } else if (!strcmp(path, "/api/status")) {
            controller::status(json.to<JsonObject>());
            json["http_requests"] = requests;
            json["http_rejected"] = rejected;
            json["http_timeouts"] = timeouts;
            json["http_poll_us_max"] = pollMaxUs;
            json["http_last_timeout_bytes"] = timeoutBytes;
            json["http_last_timeout_headers"] = timeoutHeaders;
            json["http_last_timeout_tail"] = timeoutTail;
        } else {
            if (!strcmp(path, "/favicon.ico")) {
                response(slot, 204, "image/x-icon", nullptr, 0); return;
            }
            if (!strcmp(path, "/")) path = "/index.html";
            for (const auto& asset : web_assets::assets) {
                if (!strcmp(path, asset.path)) {
                    response(slot, 200, asset.mime, asset.data, asset.size, true); return;
                }
            }
            error(slot, 404, "NOT_FOUND"); return;
        }
        sendJson(slot); return;
    }
    Input input{slot.request + slot.bodyOffset, slot.parsed.contentLength};
    auto result = deserializeJson(json, input, DeserializationOption::NestingLimit(6));
    bool trailing = false;
    while (input.position < input.length) {
        const int c = input.read();
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') trailing = true;
    }
    if (result || trailing || !json.is<JsonObject>()) { error(slot, 400, "INVALID_JSON"); return; }
    char detail[192] = {};
    bool ok = false;
    if (!strcmp(path, "/api/config")) {
        webcfg::Config config = webcfg::defaults();
        if (!webcfg::fromJson(json.as<JsonObjectConst>(), config, detail, sizeof(detail))) {
            error(slot, 400, detail); return;
        }
        ok = controller::apply(config, detail, sizeof(detail));
    } else if (!strcmp(path, "/api/save")) {
        ok = controller::save(detail, sizeof(detail));
    } else if (!strcmp(path, "/api/start")) {
        ok = controller::start(detail, sizeof(detail));
    } else if (!strcmp(path, "/api/stop")) {
        controller::stop(); ok = true;
    } else if (!strcmp(path, "/api/test")) {
        if (!json["output"].is<unsigned>() || !json["seconds"].is<unsigned>() ||
            json["output"].as<unsigned>() > 8 || json["seconds"].as<unsigned>() < 1 ||
            json["seconds"].as<unsigned>() > 600) {
            error(slot, 400, "TEST_RANGE_OUTPUT_0_8_SECONDS_1_600"); return;
        }
        ok = controller::test(json["output"].as<unsigned>(), json["seconds"].as<unsigned>(), detail, sizeof(detail));
    } else if (!strcmp(path, "/api/test-pattern")) {
        if (!json["action"].is<const char*>()) { error(slot, 400, "TEST_ACTION_REQUIRED"); return; }
        const char* action = json["action"];
        if (!strcmp(action, "stop")) { controller::endPattern(); ok = true; }
        else if (!strcmp(action, "start") || !strcmp(action, "loop")) {
            if (!json["output"].is<unsigned>() || json["output"].as<unsigned>() > 8) {
                error(slot, 400, "OUTPUT_RANGE_0_8"); return;
            }
            ok = controller::pattern(json["output"].as<unsigned>(), !strcmp(action, "loop"), detail, sizeof(detail));
        } else { error(slot, 400, "UNKNOWN_TEST_ACTION"); return; }
    } else if (!strcmp(path, "/api/reboot")) {
        ok = controller::reboot(detail, sizeof(detail));
    } else { error(slot, 404, "NOT_FOUND"); return; }
    if (!ok) { error(slot, 409, detail); return; }
    json.clear(); json["ok"] = true;
    sendJson(slot);
}
void receive(Slot& slot, uint32_t now) {
    const int available = slot.client.available();
    if (available <= 0) return;
    const size_t room = sizeof(slot.request) - 1 - slot.received;
    if (!room) { error(slot, 413, "REQUEST_TOO_LARGE"); return; }
    size_t wanted = size_t(available);
    if (wanted > kBudget) wanted = kBudget;
    if (wanted > room) wanted = room;
    const size_t old = slot.received;
    const int n = slot.client.read(reinterpret_cast<uint8_t*>(slot.request + old), wanted);
    if (n <= 0) return;
    slot.received += size_t(n);
    slot.request[slot.received] = 0;
    slot.progressed = now;
    if (!slot.headers) {
        // Scan only new bytes plus the possible split delimiter.
        for (size_t i = old > 3 ? old - 3 : 0; i + 3 < slot.received; ++i) {
            if (memcmp(slot.request + i, "\r\n\r\n", 4)) continue;
            slot.bodyOffset = i + 4;
            const unsigned code = http_request::parse(slot.request, slot.bodyOffset, slot.parsed);
            if (code) { error(slot, code, "INVALID_HTTP_REQUEST"); return; }
            slot.headers = true;
            break;
        }
        if (!slot.headers && slot.received >= http_request::kMaxHeader) {
            error(slot, 431, "HEADERS_TOO_LARGE"); return;
        }
    }
    if (slot.headers && slot.received >= slot.bodyOffset + slot.parsed.contentLength) dispatch(slot);
}
void transmit(Slot& slot, uint32_t now) {
    int available = slot.client.availableForWrite();
    if (available <= 0) return;
    size_t budget = size_t(available) < kBudget ? size_t(available) : kBudget;
    if (slot.headerSent < slot.headerLength) {
        size_t n = slot.headerLength - slot.headerSent;
        if (n > budget) n = budget;
        size_t written = slot.client.write(reinterpret_cast<const uint8_t*>(slot.header + slot.headerSent), n);
        slot.headerSent += written; budget -= written;
        if (written) slot.progressed = now;
        if (slot.headerSent < slot.headerLength) return;
    }
    if (budget && slot.bodySent < slot.bodyLength) {
        size_t n = slot.bodyLength - slot.bodySent;
        if (n > budget) n = budget;
        size_t written = slot.client.write(slot.body + slot.bodySent, n);
        slot.bodySent += written;
        if (written) slot.progressed = now;
    }
    if (slot.bodySent == slot.bodyLength) {
        if (slot.headers && slot.received == slot.bodyOffset + slot.parsed.contentLength &&
            slot.client.available() == 0) {
            close(slot); return;  // Complete normal request; release scarce sockets promptly.
        }
        // Preserve queued responses even when a rejected request has unread TCP
        // data. Full close with unread data would make lwIP send RST and lose 413.
        slot.client.closeOutput();
        slot.finishing = true;
    }
}
}
void begin() { server.begin(); }
void poll() {
    const uint32_t started = micros(), now = millis();
    // Two clients allow a control request alongside one slow asset/header client.
    for (auto& slot : slots) {
        if (!slot.used) {
            EthernetClient next = server.accept();
            if (!next) continue;
            slot.client = next;
            slot.client.setConnectionTimeoutEnabled(false);
            slot.client.setNoDelay(true);
            slot.used = true; slot.headers = slot.sending = slot.finishing = false;
            slot.received = slot.bodyOffset = 0;
            slot.started = slot.progressed = now;
        }
        if (!slot.client.connected() && !slot.client.available()) { close(slot); continue; }
        if (uint32_t(now - slot.started) >= kTotalMs || uint32_t(now - slot.progressed) >= kIdleMs) {
            timeoutBytes = slot.received; timeoutHeaders = slot.headers; timeoutTail = 0;
            for (size_t i = slot.received > 4 ? slot.received - 4 : 0; i < slot.received; ++i)
                timeoutTail = (timeoutTail << 8) | uint8_t(slot.request[i]);
            ++timeouts; slot.client.abort(); close(slot); continue;
        }
        if (slot.finishing) {
            const int available = slot.client.available();
            if (available > 0) {
                const size_t n = size_t(available) < kBudget ? size_t(available) : kBudget;
                slot.client.read(nullptr, n);  // Bounded discard; FIN already queued.
                slot.progressed = now;
            }
        } else if (slot.sending) transmit(slot, now); else receive(slot, now);
    }
    const uint32_t elapsed = micros() - started;
    if (elapsed > pollMaxUs) pollMaxUs = elapsed;
}
}
