#!/usr/bin/env python3
import os
import sys
import time
import socket
import struct
import random
import string
import threading
import ctypes


HOST = os.environ.get("FS_HOST", "127.0.0.1")
PORT = int(os.environ.get("FS_PORT", "8000"))

# Detect native size_t for interop with C server
SIZE_T_BYTES = ctypes.sizeof(ctypes.c_size_t)
ENDIAN = "<" if sys.byteorder == "little" else ">"
SIZE_T_FMT = ("Q" if SIZE_T_BYTES == 8 else "I")


def send_all(sock: socket.socket, data: bytes) -> None:
    view = memoryview(data)
    total = 0
    while total < len(data):
        n = sock.send(view[total:])
        if n <= 0:
            raise RuntimeError("socket send failed")
        total += n


def recv_all(sock: socket.socket, n: int) -> bytes:
    chunks = []
    received = 0
    while received < n:
        chunk = sock.recv(n - received)
        if not chunk:
            raise RuntimeError("socket closed while receiving")
        chunks.append(chunk)
        received += len(chunk)
    return b"".join(chunks)


def send_cmd(sock: socket.socket, cmd: str) -> None:
    # Protocol: int length (includes NUL) + NUL-terminated string
    data = cmd.encode("utf-8") + b"\x00"
    length = len(data)
    send_all(sock, struct.pack(ENDIAN + "i", length))
    send_all(sock, data)


def recv_response(sock: socket.socket) -> bytes:
    # Protocol: size_t length + payload bytes
    raw_len = recv_all(sock, SIZE_T_BYTES)
    (length,) = struct.unpack(ENDIAN + SIZE_T_FMT, raw_len)
    if length == 0:
        return b""
    return recv_all(sock, length)


def upload(sock: socket.socket, path: str) -> None:
    size = os.path.getsize(path)
    send_all(sock, struct.pack(ENDIAN + SIZE_T_FMT, size))
    with open(path, "rb") as f:
        while True:
            chunk = f.read(64 * 1024)
            if not chunk:
                break
            send_all(sock, chunk)


def random_bytes(n: int) -> bytes:
    return os.urandom(n)


def make_temp_file(dir_path: str, prefix: str, size: int) -> str:
    os.makedirs(dir_path, exist_ok=True)
    name = f"{prefix}_{int(time.time()*1000)}_{random.randint(1000,9999)}.bin"
    p = os.path.join(dir_path, name)
    with open(p, "wb") as f:
        f.write(random_bytes(size))
    return p


def client_flow(username: str, password: str, file_size: int = 64 * 1024) -> None:
    def connect() -> socket.socket:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        return s

    # Try LOGIN first; if it fails, reconnect and try SIGNUP
    sock = connect()
    try:
        send_cmd(sock, f"LOGIN {username} {password}")
        resp = recv_response(sock).decode("utf-8", errors="ignore")
        if "FAILED" in resp:
            sock.close()
            sock = connect()
            send_cmd(sock, f"SIGNUP {username} {password}")
            resp = recv_response(sock).decode("utf-8", errors="ignore")
            if "FAILED" in resp:
                # Give up for this session
                return

        # Prepare file
        tmp_dir = os.path.join("tmp_py_uploads")
        fpath = make_temp_file(tmp_dir, username, file_size)
        fname = os.path.basename(fpath)

        # UPLOAD
        send_cmd(sock, f"UPLOAD {fname}")
        upload(sock, fpath)
        _ = recv_response(sock)

        # LIST
        send_cmd(sock, "LIST")
        _ = recv_response(sock)

        # DOWNLOAD
        send_cmd(sock, f"DOWNLOAD {fname}")
        data = recv_response(sock)
        # Save to downloads
        os.makedirs("downloads", exist_ok=True)
        with open(os.path.join("downloads", fname), "wb") as f:
            f.write(data)

        # DELETE
        send_cmd(sock, f"DELETE {fname}")
        _ = recv_response(sock)

        # LIST again
        send_cmd(sock, "LIST")
        _ = recv_response(sock)

    finally:
        try:
            send_cmd(sock, "quit")
        except Exception:
            pass
        try:
            sock.close()
        except Exception:
            pass


def main() -> None:
    users = [("naini", "nainiPass"), ("fazan", "fazanPass"), ("hassan", "hassanPass")]
    threads = []

    # One thread per user, with two sessions each
    for (u, p) in users:
        for _ in range(2):
            t = threading.Thread(target=client_flow, args=(u, p, random.randint(16*1024, 96*1024)))
            t.start()
            threads.append(t)

    for t in threads:
        t.join()

    print("Python load test completed.")


if __name__ == "__main__":
    main()


