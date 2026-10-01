#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace http_request {
constexpr size_t kMaxHeader = 1536;
constexpr size_t kMaxBody = 6144;
struct Request {
    char method[8] = {};
    char path[96] = {};
    size_t contentLength = 0;
};
inline bool equalCase(const char* a, const char* b) {
    while (*a && *b) {
        char x = *a++, y = *b++;
        if (x >= 'A' && x <= 'Z') x += 'a' - 'A';
        if (y >= 'A' && y <= 'Z') y += 'a' - 'A';
        if (x != y) return false;
    }
    return *a == *b;
}
// A complete CRLF header, including its terminating empty line. Mutates header
// only; caller keeps the body after len. Fail closed on ambiguous framing.
inline unsigned parse(char* header, size_t len, Request& out) {
    out = Request{};
    if (len > kMaxHeader) return 431;
    if (len < 4 || memcmp(header + len - 4, "\r\n\r\n", 4)) return 400;
    for (size_t i = 0; i < len; ++i) {
        const unsigned c = static_cast<unsigned char>(header[i]);
        if (!c || c > 126 || (c < 32 && c != '\r' && c != '\n' && c != '\t')) return 400;
    }
    char* end = strstr(header, "\r\n");
    if (!end) return 400;
    *end = 0;
    char* space = strchr(header, ' ');
    if (!space || size_t(space - header) >= sizeof(out.method)) return 400;
    *space = 0;
    strcpy(out.method, header);
    char* target = space + 1;
    space = strchr(target, ' ');
    if (!space || size_t(space - target) >= sizeof(out.path)) return 414;
    *space = 0;
    if (target[0] != '/' || strchr(target, '\t')) return 400;
    strcpy(out.path, target);
    const bool http11 = strcmp(space + 1, "HTTP/1.1") == 0;
    if (!http11 && strcmp(space + 1, "HTTP/1.0")) return 400;
    bool haveLength = false, json = false, haveType = false, haveHost = false;
    char* cursor = end + 2;
    while (cursor < header + len - 2) {
        end = strstr(cursor, "\r\n");
        if (!end || cursor == end || *cursor == ' ' || *cursor == '\t') return 400;
        *end = 0;
        char* colon = strchr(cursor, ':');
        if (!colon || colon == cursor) return 400;
        *colon = 0;
        for (const char* p = cursor; *p; ++p)
            if (!( (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                   (*p >= '0' && *p <= '9') || *p == '-')) return 400;
        char* value = colon + 1;
        while (*value == ' ' || *value == '\t') ++value;
        char* tail = end;
        while (tail > value && (tail[-1] == ' ' || tail[-1] == '\t')) *--tail = 0;
        if (equalCase(cursor, "Content-Length")) {
            if (haveLength || !*value) return 400;
            haveLength = true;
            size_t number = 0;
            for (const char* p = value; *p; ++p) {
                if (*p < '0' || *p > '9') return 400;
                number = number * 10 + unsigned(*p - '0');
                if (number > kMaxBody) return 413;
            }
            out.contentLength = number;
        } else if (equalCase(cursor, "Content-Type")) {
            if (haveType) return 400;
            haveType = true;
            char* semicolon = strchr(value, ';');
            if (semicolon) *semicolon = 0;
            json = equalCase(value, "application/json");
        } else if (equalCase(cursor, "Transfer-Encoding") || equalCase(cursor, "Content-Encoding")) {
            return 400;
        } else if (equalCase(cursor, "Expect")) {
            return 417;
        } else if (equalCase(cursor, "Host")) {
            if (haveHost || !*value) return 400;
            haveHost = true;
        }
        cursor = end + 2;
    }
    if (http11 && !haveHost) return 400;
    if (!strcmp(out.method, "POST")) {
        if (!haveLength) return 411;
        if (!json) return 415;
        if (!out.contentLength) return 400;
    } else if (!strcmp(out.method, "GET")) {
        if (out.contentLength) return 400;
    } else return 405;
    return 0;
}
}
