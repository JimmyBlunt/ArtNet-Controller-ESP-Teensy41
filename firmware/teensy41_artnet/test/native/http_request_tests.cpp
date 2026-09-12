#include "../../include/http_request.h"
#include <assert.h>
#include <stdio.h>
#include <string>

unsigned parse(std::string text, http_request::Request* result = nullptr) {
    http_request::Request request;
    const unsigned code = http_request::parse(&text[0], text.size(), request);
    if (result) *result = request;
    return code;
}
int main() {
    using http_request::Request;
    Request r;
    assert(parse("GET /api/status HTTP/1.1\r\nHost: 10.0.0.253\r\n\r\n", &r) == 0);
    assert(std::string(r.path) == "/api/status" && r.contentLength == 0);
    const std::string post = "POST /api/config HTTP/1.1\r\nHost: x\r\nContent-Type: application/json; charset=utf-8\r\n";
    assert(parse(post + "Content-Length: 6144\r\n\r\n", &r) == 0 && r.contentLength == 6144);
    assert(parse(post + "Content-Length: 6145\r\n\r\n") == 413);
    assert(parse(post + "Content-Length: 999999999999999999999999\r\n\r\n") == 413);
    assert(parse(post + "Content-Length: -1\r\n\r\n") == 400);
    assert(parse(post + "Content-Length: 2x\r\n\r\n") == 400);
    assert(parse(post + "Content-Length: 2\r\nContent-Length: 2\r\n\r\n") == 400);
    assert(parse(post + "Content-Length: 2\r\nTransfer-Encoding: chunked\r\n\r\n") == 400);
    assert(parse(post + "Content-Length: 2\r\nExpect: 100-continue\r\n\r\n") == 417);
    assert(parse(post + "Content-Length: 2\r\nContent-Encoding: gzip\r\n\r\n") == 400);
    assert(parse(post + "\r\n") == 411);
    assert(parse("POST /api/stop HTTP/1.1\r\nHost: x\r\nContent-Length: 2\r\nContent-Type: text/plain\r\n\r\n") == 415);
    assert(parse("GET / HTTP/1.1\r\n\r\n") == 400);
    assert(parse("GET / HTTP/1.0\r\n\r\n") == 0);
    assert(parse("GET / HTTP/1.1\r\nHost: x\r\nHost: y\r\n\r\n") == 400);
    assert(parse("GET / HTTP/1.1\r\nHost: x\r\n folded: bad\r\n\r\n") == 400);
    assert(parse("GET / HTTP/1.1\r\nHost: x\r\nContent-Length: 1\r\n\r\n") == 400);
    assert(parse("DELETE / HTTP/1.1\r\nHost: x\r\n\r\n") == 405);
    assert(parse("GET /" + std::string(100, 'x') + " HTTP/1.1\r\nHost: x\r\n\r\n") == 414);
    assert(parse("GET / HTTP/1.1\r\nHost: x\r\nX: " + std::string(1536, 'x') + "\r\n\r\n") == 431);
    std::string nul = "GET / HTTP/1.1\r\nHost: x\r\n\r\n"; nul[6] = 0;
    assert(parse(nul) == 400);
    puts("HTTP framing tests passed: bounds, length overflow, duplicate headers, unsupported framing/methods, embedded NUL.");
}
