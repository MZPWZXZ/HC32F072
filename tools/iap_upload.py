#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
HC32F072KA IAP 上位机:通过自定义串口协议向 Bootloader 升级 App。

用法:
    python tools/iap_upload.py --port COM5 dist/hc32f072ka_app.bin
    python tools/iap_upload.py -p COM5 -b 115200 --no-boot dist/hc32f072ka_app.bin

流程:
    1. (可选)向运行中的 App 发送 "boot\\r",使芯片软复位进入 Bootloader;
    2. SYNC 握手(Bootloader 回复 0x06 即就绪);
    3. 按"从尾到头"逐 512B 扇区发送 WRITE 帧,每帧等待 ACK/NAK;
    4. 发送 DONE 帧(镜像长度 + CRC32),Bootloader 回读校验后自动跳转 App。

协议与 bootloader/iap_proto.h 保持一致;需要 pyserial:
    pip install pyserial
"""
import argparse
import binascii
import struct
import sys
import time

import serial

# ------------------------- 协议常量(与 iap_proto.h 同步) -------------------------
FRAME_MAGIC = 0xAA
CMD_SYNC = 0x01
CMD_WRITE = 0x03
CMD_DONE = 0x04
CMD_REBOOT = 0x05
ACK = 0x06
NAK = 0x15

FLASH_SECTOR = 512          # 扇区大小
APP_MAX_SIZE = 0x18000      # App 最大 96KB
BOOT_CMD = b"boot\r"


# ---------------------------------- 工具函数 ----------------------------------
def crc16(data):
    """CRC16-CCITT-FALSE(与 Bootloader iap_crc16 一致)。"""
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def build_frame(cmd, payload=b""):
    """组帧:MAGIC + CMD + LEN(u16 LE) + PAYLOAD + CRC16。"""
    head = bytes([FRAME_MAGIC, cmd]) + struct.pack("<H", len(payload))
    body = head[1:] + payload
    return head + payload + struct.pack("<H", crc16(body))


def read_status(port, timeout=2.0):
    """读取单字节状态,跳过引导程序打印的 ASCII 文本。"""
    deadline = time.time() + timeout
    while time.time() < deadline:
        b = port.read(1)
        if not b:
            continue
        if b[0] in (ACK, NAK):
            return b[0]
    return None


def sync_with_bootloader(port, tries=40):
    """反复发 SYNC 直到收到 ACK。"""
    frame = build_frame(CMD_SYNC)
    for i in range(tries):
        port.write(frame)
        status = read_status(port, timeout=0.6)
        if status == ACK:
            print("[iap] bootloader synced (try %d)" % (i + 1))
            return True
    return False


# ------------------------------------ 主流程 -----------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="HC32F072KA IAP 串口升级工具")
    parser.add_argument("image", help="App 镜像(bin),如 dist/hc32f072ka_app.bin")
    parser.add_argument("-p", "--port", required=True, help="串口名,如 COM5")
    parser.add_argument("-b", "--baud", type=int, default=115200,
                        help="波特率,默认 115200")
    parser.add_argument("--no-boot", action="store_true",
                        help="不向 App 发送 boot 命令(需已处于 Bootloader)")
    args = parser.parse_args()

    try:
        with open(args.image, "rb") as f:
            raw = f.read()
    except OSError as exc:
        sys.exit("[iap] cannot open image: %s" % exc)

    if not raw or len(raw) > APP_MAX_SIZE:
        sys.exit("[iap] image empty or too large(>96KB)")

    # 补齐到扇区对齐,空缺填 0xFF(擦除态)
    pad = (-len(raw)) % FLASH_SECTOR
    blob = raw + b"\xFF" * pad
    total = len(blob)
    crc_val = binascii.crc32(blob) & 0xFFFFFFFF
    print("[iap] image: %d bytes -> %d bytes(sector aligned), crc32=0x%08X"
          % (len(raw), total, crc_val))

    try:
        port = serial.Serial(args.port, args.baud, timeout=0.4)
    except serial.SerialException as exc:
        sys.exit("[iap] open %s failed: %s" % (args.port, exc))

    try:
        if not args.no_boot:
            print("[iap] ask app to enter bootloader ...")
            port.reset_input_buffer()
            port.write(BOOT_CMD)
            time.sleep(0.3)   # 等待 App 处理并软复位

        if not sync_with_bootloader(port):
            sys.exit("[iap] no bootloader response. 请确认已进入引导"
                     "(App 输入 boot,或 App 非法自动停留)后重试")

        # 逐扇区写入:从尾到头(向量表扇区最后写,中断可保留旧 App)
        total_frames = total // FLASH_SECTOR
        sent = 0
        for offset in range(total - FLASH_SECTOR, -1, -FLASH_SECTOR):
            sector = blob[offset:offset + FLASH_SECTOR]
            frame = build_frame(CMD_WRITE,
                                struct.pack("<I", offset) + sector)

            ok = False
            for attempt in range(3):
                port.write(frame)
                status = read_status(port, timeout=3.0)
                if status == ACK:
                    ok = True
                    break
                if status == NAK:
                    print("[iap] sector@0x%X NAK, retry %d/3"
                          % (offset, attempt + 1))
                # 超时也重发
            if not ok:
                sys.exit("[iap] write sector 0x%X failed(offset 0x%X)"
                         % (offset // FLASH_SECTOR, offset))

            sent += 1
            if sent % 8 == 0 or sent == total_frames:
                print("[iap] write %d/%d sectors (%.0f%%)"
                      % (sent, total_frames, 100.0 * sent / total_frames))

        # 完成帧:Bootloader 回读整镜像校验 CRC32 后自动跳转
        done = build_frame(CMD_DONE, struct.pack("<II", total, crc_val))
        status = None
        for _ in range(3):
            port.write(done)
            status = read_status(port, timeout=5.0)
            if status is not None:
                break
        if status == ACK:
            print("[iap] upgrade OK, bootloader jumps to app")
        else:
            sys.exit("[iap] final verify failed(NAK/timeout), 请重跑升级")

    finally:
        port.close()


if __name__ == "__main__":
    main()
