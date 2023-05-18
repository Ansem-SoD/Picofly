import struct, zlib

blocks = []
last_off = 0x10000000

def add_block(data, offset):
    global last_off
    if last_off != offset:
        for i in range((offset - last_off) // 256):
            blocks.append((b"\xFF" * 256, last_off + i * 256))
    assert len(data) == 256
    blocks.append((data, offset))
    last_off = offset + 256

def get_uf2():
    total_blocks = len(blocks)
    result = b""
    for n, b in enumerate(blocks):
        uf2 = struct.pack("<IIIIIIII", 0x0A324655, 0x9E5D5157, 0x00002000, b[1], 256, n, total_blocks, 0xe48bff56) + b[0] + b"\x00" * (476 - 256) + struct.pack("<I", 0x0AB16F30)
        assert len(uf2) == 512
        result += uf2
    return result

def add_blocks(data, offset):
    if len(data) % 256:
        data += b"\xFF" * (256 - len(data) % 256)
    for i in range(0, len(data), 256):
        add_block(data[i:i+256], offset + i)

add_blocks(open("../busk/busk.bin", "rb").read(), 0x10000000)

add_blocks(b"\xFF" * 256, 0x10000000 + 0xFF00)

fw = open("usk.bin", "rb").read()
fw = struct.pack("<II", len(fw), zlib.crc32(fw)) + fw
open("update.bin", "wb").write(fw)

fw = fw + b"\xFF" * (0x3)

add_blocks(fw, 0x10000000 + 0x10000)

open("firmware.uf2", "wb").write(get_uf2())