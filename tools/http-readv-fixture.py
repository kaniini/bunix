#!/usr/bin/env python3
import argparse
import http.server
import os
import socketserver
import time


DEFAULT_PAYLOAD_SIZE = 262144
PREFIX = b"BUNIX_READV_HTTP_BEGIN\n"
SUFFIX = b"\nBUNIX_READV_HTTP_END\n"
FILL = b"0123456789abcdef"
PAYLOAD = b""
CHUNK_SIZE = 4096


def build_payload(payload_size):
    if payload_size < len(PREFIX) + len(SUFFIX):
        raise ValueError("payload size is too small for markers")
    fill_len = payload_size - len(PREFIX) - len(SUFFIX)
    return PREFIX + (FILL * ((fill_len + len(FILL) - 1) // len(FILL)))[:fill_len] + SUFFIX


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/health":
            body = b"ok\n"
        elif self.path == "/readv.bin" or self.path == "/large.bin":
            body = PAYLOAD
        else:
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        for offset in range(0, len(body), CHUNK_SIZE):
            self.wfile.write(body[offset:offset + CHUNK_SIZE])

    def log_message(self, fmt, *args):
        print("http-readv-fixture: " + fmt % args, flush=True)


class ReusableTCPServer(socketserver.TCPServer):
    allow_reuse_address = True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=18080)
    parser.add_argument("--ready-file")
    parser.add_argument("--duration", type=float, default=180.0)
    parser.add_argument("--payload-size", type=int, default=DEFAULT_PAYLOAD_SIZE)
    parser.add_argument("--chunk-size", type=int, default=4096)
    args = parser.parse_args()
    global PAYLOAD
    global CHUNK_SIZE
    PAYLOAD = build_payload(args.payload_size)
    CHUNK_SIZE = args.chunk_size if args.chunk_size > 0 else 4096

    with ReusableTCPServer((args.host, args.port), Handler) as server:
        server.timeout = 0.5
        if args.ready_file:
            with open(args.ready_file, "w", encoding="utf-8") as f:
                f.write("ready\n")
        deadline = time.monotonic() + args.duration
        while time.monotonic() < deadline:
            server.handle_request()
    if args.ready_file:
        try:
            os.unlink(args.ready_file)
        except FileNotFoundError:
            pass


if __name__ == "__main__":
    main()
