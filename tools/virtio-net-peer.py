#!/usr/bin/env python3
import argparse
import ipaddress
import os
import select
import socket
import struct
import time


ETH_IPV4 = 0x0800
ETH_ARP = 0x0806
ETH_IPV6 = 0x86DD
IP_ICMP = 1
IP_ICMPV6 = 58
ICMPV6_ECHO_REQUEST = 128
ICMPV6_ECHO_REPLY = 129
ICMPV6_NEIGHBOR_SOLICIT = 135
ICMPV6_NEIGHBOR_ADVERT = 136


def parse_mac(text):
    parts = text.split(":")
    if len(parts) != 6:
        raise argparse.ArgumentTypeError("MAC must have 6 octets")
    try:
        data = bytes(int(part, 16) for part in parts)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("invalid MAC octet") from exc
    if len(data) != 6:
        raise argparse.ArgumentTypeError("invalid MAC")
    return data


def parse_ipv4(text):
    return ipaddress.IPv4Address(text).packed


def parse_ipv6(text):
    return ipaddress.IPv6Address(text).packed


def checksum(data):
    if len(data) & 1:
        data += b"\x00"
    total = 0
    for i in range(0, len(data), 2):
        total += (data[i] << 8) | data[i + 1]
        total = (total & 0xffff) + (total >> 16)
    while total >> 16:
        total = (total & 0xffff) + (total >> 16)
    return (~total) & 0xffff


def ipv4_header(src_ip, dst_ip, proto, payload_len, ident=0):
    header = bytearray(20)
    header[0] = 0x45
    struct.pack_into("!HHHBBH4s4s", header, 2, 20 + payload_len, ident, 0,
                     64, proto, 0, src_ip, dst_ip)
    struct.pack_into("!H", header, 10, checksum(header))
    return bytes(header)


def ipv6_header(src_ip, dst_ip, next_header, payload_len, hop_limit=64):
    header = bytearray(40)
    header[0] = 0x60
    struct.pack_into("!HBB16s16s", header, 4, payload_len, next_header,
                     hop_limit, src_ip, dst_ip)
    return bytes(header)


def ipv6_checksum(src_ip, dst_ip, next_header, payload):
    pseudo = src_ip + dst_ip + struct.pack("!I3xB", len(payload), next_header)
    return checksum(pseudo + payload)


def arp_packet(opcode, sender_mac, sender_ip, target_mac, target_ip):
    return struct.pack("!HHBBH6s4s6s4s", 1, ETH_IPV4, 6, 4, opcode,
                       sender_mac, sender_ip, target_mac, target_ip)


def ethernet(dst_mac, src_mac, ethertype, payload):
    return dst_mac + src_mac + struct.pack("!H", ethertype) + payload


def arp_request(peer_mac, peer_ip, guest_ip):
    return ethernet(b"\xff" * 6, peer_mac, ETH_ARP,
                    arp_packet(1, peer_mac, peer_ip, b"\x00" * 6,
                               guest_ip))


def arp_reply(peer_mac, peer_ip, guest_mac, guest_ip):
    return ethernet(guest_mac, peer_mac, ETH_ARP,
                    arp_packet(2, peer_mac, peer_ip, guest_mac, guest_ip))


def icmp_echo_reply(peer_mac, peer_ip, guest_mac, guest_ip, request_payload):
    icmp = bytearray(request_payload)
    if len(icmp) < 8:
        return None
    icmp[0] = 0
    icmp[1] = 0
    icmp[2] = 0
    icmp[3] = 0
    struct.pack_into("!H", icmp, 2, checksum(icmp))
    ip = ipv4_header(peer_ip, guest_ip, IP_ICMP, len(icmp))
    return ethernet(guest_mac, peer_mac, ETH_IPV4, ip + bytes(icmp))


def solicited_node_ipv6(target_ip):
    return ipaddress.IPv6Address("ff02::1:ff00:0").packed[:13] + target_ip[-3:]


def icmpv6_neighbor_advert(peer_mac, peer_ip6, guest_mac, guest_ip6,
                           request_payload):
    if len(request_payload) < 24:
        return None
    target_ip = request_payload[8:24]
    if target_ip != peer_ip6:
        return None
    icmp = bytearray(32)
    icmp[0] = ICMPV6_NEIGHBOR_ADVERT
    icmp[4] = 0x60
    icmp[8:24] = peer_ip6
    icmp[24] = 2
    icmp[25] = 1
    icmp[26:32] = peer_mac
    csum = ipv6_checksum(peer_ip6, guest_ip6, IP_ICMPV6, icmp)
    struct.pack_into("!H", icmp, 2, csum)
    ip = ipv6_header(peer_ip6, guest_ip6, IP_ICMPV6, len(icmp), 255)
    return ethernet(guest_mac, peer_mac, ETH_IPV6, ip + bytes(icmp))


def icmpv6_echo_reply(peer_mac, peer_ip6, guest_mac, guest_ip6,
                      request_payload):
    if len(request_payload) < 8:
        return None
    icmp = bytearray(request_payload)
    icmp[0] = ICMPV6_ECHO_REPLY
    icmp[1] = 0
    icmp[2] = 0
    icmp[3] = 0
    csum = ipv6_checksum(peer_ip6, guest_ip6, IP_ICMPV6, icmp)
    struct.pack_into("!H", icmp, 2, csum)
    ip = ipv6_header(peer_ip6, guest_ip6, IP_ICMPV6, len(icmp))
    return ethernet(guest_mac, peer_mac, ETH_IPV6, ip + bytes(icmp))


def parse_mcast(text):
    host, sep, port = text.rpartition(":")
    if not sep or not host:
        raise argparse.ArgumentTypeError("expected GROUP:PORT")
    return host, int(port, 10)


def open_mcast(group, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", port))
    membership = socket.inet_aton(group) + socket.inet_aton("0.0.0.0")
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, membership)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, 1)
    sock.setblocking(False)
    return sock


class Peer:
    def __init__(self, args):
        self.group, self.port = parse_mcast(args.mcast)
        self.guest_mac = parse_mac(args.guest_mac)
        self.peer_mac = parse_mac(args.peer_mac)
        self.guest_ip = parse_ipv4(args.guest_ip)
        self.peer_ip = parse_ipv4(args.peer_ip)
        self.guest_ip6 = parse_ipv6(args.guest_ip6)
        self.peer_ip6 = parse_ipv6(args.peer_ip6)
        self.sock = open_mcast(self.group, self.port)
        self.end_at = time.monotonic() + args.duration
        self.verbose = args.verbose
        self.rx = 0
        self.tx = 0

    def log(self, message):
        print(message, flush=True)

    def send(self, frame):
        self.sock.sendto(frame, (self.group, self.port))
        self.tx += 1
        if self.verbose:
            self.log(f"tx len={len(frame)}")

    def handle_arp(self, frame):
        if len(frame) < 42:
            return
        arp = frame[14:42]
        htype, ptype, hlen, plen, opcode = struct.unpack("!HHBBH", arp[:8])
        if htype != 1 or ptype != ETH_IPV4 or hlen != 6 or plen != 4:
            return
        sender_mac = arp[8:14]
        sender_ip = arp[14:18]
        target_ip = arp[24:28]
        if opcode == 1 and target_ip == self.peer_ip:
            self.log(f"arp request from {ipaddress.IPv4Address(sender_ip)}")
            self.send(arp_reply(self.peer_mac, self.peer_ip, sender_mac,
                                sender_ip))

    def handle_ipv4(self, frame):
        if len(frame) < 34:
            return
        ip = frame[14:]
        ihl = (ip[0] & 0x0f) * 4
        if (ip[0] >> 4) != 4 or ihl < 20 or len(ip) < ihl:
            return
        total_len = struct.unpack("!H", ip[2:4])[0]
        if total_len < ihl or len(ip) < total_len:
            return
        proto = ip[9]
        src_ip = ip[12:16]
        dst_ip = ip[16:20]
        if proto == IP_ICMP and dst_ip == self.peer_ip:
            payload = ip[ihl:total_len]
            if len(payload) >= 8 and payload[0] == 8:
                self.log(f"icmp echo from {ipaddress.IPv4Address(src_ip)}")
                reply = icmp_echo_reply(self.peer_mac, self.peer_ip,
                                        frame[6:12], src_ip, payload)
                if reply is not None:
                    self.send(reply)

    def handle_ipv6(self, frame):
        if len(frame) < 54:
            return
        ip = frame[14:]
        if (ip[0] >> 4) != 6:
            return
        payload_len = struct.unpack("!H", ip[4:6])[0]
        next_header = ip[6]
        src_ip = ip[8:24]
        dst_ip = ip[24:40]
        if len(ip) < 40 + payload_len or next_header != IP_ICMPV6:
            return
        payload = ip[40:40 + payload_len]
        if not payload:
            return
        if payload[0] == ICMPV6_NEIGHBOR_SOLICIT:
            reply = icmpv6_neighbor_advert(self.peer_mac, self.peer_ip6,
                                           frame[6:12], src_ip, payload)
            if reply is not None:
                self.log(f"ndp ns from {ipaddress.IPv6Address(src_ip)}")
                self.send(reply)
        elif payload[0] == ICMPV6_ECHO_REQUEST and dst_ip == self.peer_ip6:
            self.log(f"icmp6 echo from {ipaddress.IPv6Address(src_ip)}")
            reply = icmpv6_echo_reply(self.peer_mac, self.peer_ip6,
                                      frame[6:12], src_ip, payload)
            if reply is not None:
                self.send(reply)

    def handle(self, frame):
        if len(frame) < 14:
            return
        ethertype = struct.unpack("!H", frame[12:14])[0]
        self.rx += 1
        if self.verbose:
            self.log(f"rx len={len(frame)} ethertype=0x{ethertype:04x}")
        if ethertype == ETH_ARP:
            self.handle_arp(frame)
        elif ethertype == ETH_IPV4:
            self.handle_ipv4(frame)
        elif ethertype == ETH_IPV6:
            self.handle_ipv6(frame)

    def run(self, send_arp):
        self.log(f"virtio-net-peer mcast={self.group}:{self.port}")
        if send_arp:
            self.send(arp_request(self.peer_mac, self.peer_ip, self.guest_ip))
        while time.monotonic() < self.end_at:
            remaining = max(0.0, self.end_at - time.monotonic())
            readable, _, _ = select.select([self.sock], [], [],
                                           min(0.25, remaining))
            if not readable:
                continue
            frame, _ = self.sock.recvfrom(65535)
            self.handle(frame)
        self.log(f"virtio-net-peer done rx={self.rx} tx={self.tx}")


def self_test():
    guest_mac = parse_mac("52:54:00:18:00:01")
    peer_mac = parse_mac("52:54:00:18:00:fe")
    guest_ip = parse_ipv4("10.0.2.15")
    peer_ip = parse_ipv4("10.0.2.2")
    request = arp_request(peer_mac, peer_ip, guest_ip)
    assert request[:6] == b"\xff" * 6
    assert request[12:14] == b"\x08\x06"
    reply = arp_reply(peer_mac, peer_ip, guest_mac, guest_ip)
    assert reply[:6] == guest_mac
    echo = b"\x08\x00\x00\x00\x12\x34\x00\x01ping"
    frame = icmp_echo_reply(peer_mac, peer_ip, guest_mac, guest_ip, echo)
    assert frame[:6] == guest_mac
    assert frame[12:14] == b"\x08\x00"
    assert frame[23] == IP_ICMP
    assert frame[34] == 0
    guest_ip6 = parse_ipv6("2001:db8:18::15")
    peer_ip6 = parse_ipv6("2001:db8:18::2")
    ns = bytearray(32)
    ns[0] = ICMPV6_NEIGHBOR_SOLICIT
    ns[8:24] = peer_ip6
    ns[24] = 1
    ns[25] = 1
    ns[26:32] = guest_mac
    frame6 = icmpv6_neighbor_advert(peer_mac, peer_ip6, guest_mac,
                                    guest_ip6, ns)
    assert frame6[:6] == guest_mac
    assert frame6[12:14] == b"\x86\xdd"
    assert frame6[20] == IP_ICMPV6
    assert frame6[54] == ICMPV6_NEIGHBOR_ADVERT
    print("virtio-net-peer self-test ok")


def main():
    parser = argparse.ArgumentParser(
        description="Small raw Ethernet peer for QEMU socket mcast netdevs.")
    parser.add_argument("--mcast", default="230.18.0.1:18100",
                        help="QEMU socket multicast endpoint GROUP:PORT")
    parser.add_argument("--guest-mac", default="52:54:00:18:00:01")
    parser.add_argument("--peer-mac", default="52:54:00:18:00:fe")
    parser.add_argument("--guest-ip", default="10.0.2.15")
    parser.add_argument("--peer-ip", default="10.0.2.2")
    parser.add_argument("--guest-ip6", default="2001:db8:18::15")
    parser.add_argument("--peer-ip6", default="2001:db8:18::2")
    parser.add_argument("--duration", type=float, default=30.0)
    parser.add_argument("--ready-file")
    parser.add_argument("--send-arp", action="store_true")
    parser.add_argument("--verbose", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    peer = Peer(args)
    if args.ready_file:
        with open(args.ready_file, "w", encoding="ascii") as ready:
            ready.write(f"{os.getpid()}\n")
    peer.run(args.send_arp)


if __name__ == "__main__":
    main()
