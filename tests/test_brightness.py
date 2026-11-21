#!/usr/bin/env python3
"""Run real firmware level/button code with a host C compiler and mocked radio/GPIO.

No device or Telink compiler is required. Tests check emitted commands rather
than simulate the lamp firmware. Real lamps still require an integration test.
"""
import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    # Use production command payloads, IDs, and command macros from the SDK.
    level_header = (ROOT / "tl_zigbee_sdk/zigbee/zcl/general/zcl_level.h").read_text()
    constants = "\n".join(line for line in level_header.splitlines()
                          if line.startswith("#define ZCL_CMD_LEVEL_") or
                          line.startswith("#define LEVEL_MOVE_") or
                          line.startswith("#define ZCL_LEVEL_OPTIONS_"))
    payloads = level_header[level_header.index("typedef struct {"):level_header.index("#if 0")]
    macros = "\n".join(line for line in level_header.splitlines()
                       if line.startswith("#define zcl_level_") and "Cmd(" in line)
    stub = (ROOT / "tests/radio_mock.h").read_text()
    (tmp / "app_main.h").write_text(stub + "\n" + constants + "\n" + payloads + "\n" + macros + "\n" +
                                   (ROOT / "tests/radio_mock_api.h").read_text())
    buttons = (ROOT / "src/app_button.c").read_text()
    button_type = buttons[buttons.index("typedef struct {"):buttons.index("static ev_timer_event_t *timerClearSleepEvt")]
    start = buttons.index("static void read_button_level(")
    end = buttons.index("\nstatic void ", start + 1)
    harness = (ROOT / "tests/brightness_harness.c").read_text()
    harness = harness.replace("/* PRODUCTION_BUTTON_TYPE */", button_type)
    harness = harness.replace("/* PRODUCTION_BUTTON_READER */", buttons[start:end])
    (tmp / "test.c").write_text(harness)
    subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Wno-unused-parameter",
                    "-Wno-unused-variable", "-Wno-unused-but-set-variable", "-I", str(tmp), "-I", str(ROOT / "src/include"),
                    "-I", str(ROOT / "src/zigbee/zcl"), str(ROOT / "src/app_level.c"),
                    str(tmp / "test.c"), "-o", str(tmp / "test")], check=True)
    subprocess.run([str(tmp / "test")], check=True)

# Every converter must use the firmware's actual enum IDs on every endpoint.
expected = {"brightness_level": 3, "brightness_level_up": 4, "brightness_level_down": 5,
            "scene": 9, "brightness_level_no_onoff": 10,
            "brightness_level_up_no_onoff": 11, "brightness_level_down_no_onoff": 12}
for path in sorted((ROOT / "zigbee2mqtt").glob("*.js")):
    source = path.read_text()
    for name, value in expected.items():
        found = re.findall(r"\b" + name + r":\s*(\d+)", source)
        assert len(found) == 1, (path, name, found)
        assert all(int(item) == value for item in found), (path, name, found)
    assert source.count("lookup: switchTypes,") == int(path.stem[5]), path
    subprocess.run(["node", "--check", str(path)], check=True)
print("PASS: converter mode IDs and JavaScript syntax for all models")
