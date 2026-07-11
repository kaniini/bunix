#!/usr/bin/env python3
import json
import socket
import sys
import time


def qmp_recv_line(sock):
    data = b""
    while b"\r\n" not in data and b"\n" not in data:
        chunk = sock.recv(1)
        if not chunk:
            raise RuntimeError("qmp closed")
        data += chunk
    return data


def qmp_send(sock, obj):
    payload = json.dumps(obj, separators=(",", ":")).encode("ascii") + b"\r\n"
    sock.sendall(payload)
    while True:
        response = qmp_recv_line(sock)
        parsed = json.loads(response.decode("ascii"))
        if "return" in parsed or "error" in parsed:
            return response


def wait_for_marker(path, marker):
    deadline = time.time() + 20
    while True:
        try:
            with open(path, "rb") as f:
                if marker in f.read():
                    print("qmp-send-key: guest transfer armed", flush=True)
                    return
        except OSError:
            pass
        if time.time() > deadline:
            raise RuntimeError("timed out waiting for serial marker")
        time.sleep(0.1)


def main():
    if len(sys.argv) not in (2, 3):
        print("usage: qmp-send-key.py SOCKET [SERIAL_LOG]", file=sys.stderr)
        return 2

    path = sys.argv[1]
    deadline = time.time() + 20
    while True:
        try:
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            sock.connect(path)
            break
        except OSError:
            if time.time() > deadline:
                raise
            time.sleep(0.1)

    with sock:
        greeting = qmp_recv_line(sock)
        print("qmp-send-key: connected", greeting.decode("ascii", "replace").strip(), flush=True)
        caps = qmp_send(sock, {"execute": "qmp_capabilities"})
        print("qmp-send-key: capabilities", caps.decode("ascii", "replace").strip(), flush=True)
        if len(sys.argv) == 3:
            wait_for_marker(sys.argv[2], b"usb-bus: transfer polled")
            time.sleep(0.2)
        else:
            time.sleep(4)
        response = qmp_send(
            sock,
            {
                "execute": "send-key",
                "arguments": {
                    "keys": [{"type": "qcode", "data": "a"}],
                    "hold-time": 100,
                },
            },
        )
        print("qmp-send-key: send-key", response.decode("ascii", "replace").strip(), flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
