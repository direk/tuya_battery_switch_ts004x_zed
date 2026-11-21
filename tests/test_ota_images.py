#!/usr/bin/env python3
"""Validate the packaged v1.0.08 native OTA images and their raw binaries."""
import binascii
import hashlib
import struct
from pathlib import Path

folder = Path(__file__).resolve().parents[1] / "bin/no_onoff"
header = struct.Struct("<I5HIH32sI")
expected_files = set()
for model in (1, 2, 3, 4, 6):
    ota = folder / f"6565-0226-10083001-tuya_battery_switch_ts004x_zed_model_{model}.zigbee"
    binary = folder / f"tuya_battery_switch_ts004x_zed_model_{model}_V1.0.08.bin"
    expected_files.update((ota.name, binary.name))
    data = ota.read_bytes()
    magic, revision, size, flags, manufacturer, image, version, stack, text, total = header.unpack_from(data)
    assert (magic, revision, size, flags) == (0x0BEEF11E, 0x100, header.size, 0), ota
    assert (manufacturer, image, version) == (0x6565, 0x0226, 0x10083001), ota
    assert total == len(data), ota
    tag, length = struct.unpack_from("<HI", data, size)
    payload = data[size + 6:]
    assert tag == 0 and length == len(payload), ota
    assert payload == binary.read_bytes(), binary
    assert payload[6:8] == b"\x5d\x02", binary
    assert int.from_bytes(payload[2:6], "little") == version, binary
    assert int.from_bytes(payload[0x18:0x1C], "little") == len(payload), binary
    assert int.from_bytes(payload[-4:], "little") == (binascii.crc32(payload[:-4]) ^ 0xFFFFFFFF), binary
    print(f"PASS: TS004{model} OTA identity/version, lengths, CRC, and binary payload")

checksums = {}
for line in (folder / "SHA256SUMS").read_text().splitlines():
    digest, name = line.split("  ", 1)
    assert name not in checksums, name
    checksums[name] = digest
assert set(checksums) == expected_files, "Checksum list must cover exactly all packaged images"
for name, digest in checksums.items():
    assert hashlib.sha256((folder / name).read_bytes()).hexdigest() == digest, name
assert len({checksums[name] for name in checksums if name.endswith(".zigbee")}) == 5, "Models must have distinct images"
print("PASS: SHA-256 checksums for all five OTA images and five raw binaries")
