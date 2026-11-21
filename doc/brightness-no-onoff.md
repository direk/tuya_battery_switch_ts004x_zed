# Brightness control without changing on/off state

## Installation

This is custom firmware for the same hardware variants supported upstream. Use
only the image matching your remote's button count and hardware. The six-button
remote supported upstream is `TS0046`, manufacturer `_TZ3000_iszegwpd`.

1. Back up your Zigbee2MQTT configuration and note the remote's current bindings
   and per-button settings.
2. For the TS0046, upload the native custom-firmware OTA image
   [`6565-0226-10083001-tuya_battery_switch_ts004x_zed_model_6.zigbee`](../bin/no_onoff/6565-0226-10083001-tuya_battery_switch_ts004x_zed_model_6.zigbee)
   through Zigbee2MQTT's **Custom firmware** update. This image targets a remote
   already running the upstream custom firmware, whose OTA identity is `6565/0226`.
   Do not use the old Tuya migration image from the main `bin` directory for this update.
3. Install the matching converter from this fork's `zigbee2mqtt` directory as an
   external converter. For TS0046 use
   [`TS0046-z-SlD.js`](../zigbee2mqtt/TS0046-z-SlD.js).
   Place it in your Zigbee2MQTT data directory's `external_converters` folder
   and restart Zigbee2MQTT. Use only one external converter for this model.
   The built-in converter does not know the three new mode values.
4. Check that the remote reports `v1.0.08` and the new `switch_type` options are
   available. Normal OTA should retain pairing, settings, and bindings; do not
   factory-reset the remote unless troubleshooting requires it.
5. Before changing a setting or binding, press any button four times to keep
   the remote awake for 20 seconds.

The converters retain the upstream Zigbee model IDs, so they override the
built-in definitions. They correct old lookup values in the original external
converter files: brightness modes are `3`, `4`, `5`, color-temperature modes are
`6`, `7`, `8`, and scene mode is `9`. The new modes are appended as `10`, `11`,
and `12`. Do not use these converters' new modes with older firmware.

## Example: separate switching, shared dimming

Keep button 3 bound to group A and button 4 bound to group B for `genOnOff`.
Create a third Zigbee group, A+B, and add all the individual lamps from A and B.
Groups are not nested; a lamp can belong to more than one group.

- Set button 5 (`switch_type_5`) to `brightness_level_up_no_onoff`.
- Set button 6 (`switch_type_6`) to `brightness_level_down_no_onoff`.
- Bind each of endpoints 5 and 6 to group A+B for `genLevelCtrl`.
- Use a double click for a brightness step, or hold and release for a continuous
  transition. A single click deliberately sends no lighting command.

Only `genLevelCtrl` is needed for these modes. Removing a `genOnOff` binding
alone does not make an original brightness mode safe for this purpose: the
Level Control cluster also includes commands with On/Off semantics.

The remote does not query which group is on. It sends the command to all lamps;
each lamp decides whether to execute it based on its own on/off state. Therefore
this also works if only some lamps within a group are on. No HA, MQTT, or
Zigbee2MQTT processing is required after configuration; Zigbee connectivity is
still required.

## Hardware acceptance test

This firmware has not yet been tested on a physical remote or Hue lamp.
Confirm these cases after installing it:

| Initial state | Double click / hold button 5 or 6 | Expected state |
| --- | --- | --- |
| A on, B off | Adjust brightness | A changes brightness; B stays off |
| A off, B on | Adjust brightness | B changes brightness; A stays off |
| A on, B on | Adjust brightness | Both change brightness and stay on |
| A off, B off | Adjust brightness | Both stay off |

Also verify single clicks do not switch lamps, releasing a hold stops the
transition, and dimming to the minimum does not switch a lit lamp off. Repeat
with Zigbee2MQTT stopped while leaving the Zigbee radio powered. Lamps must
implement the standard Level Control command/option behavior; hardware testing
is needed to confirm this for each lamp firmware.

## Build and tests

The bundled Telink SDK and the TC32 compiler are required for an OTA build.
The included TS0046 binary was built with TC32 `4.5.1.tc32-elf-1.5`, using
`flyskywhy/tc32` commit `fdb1e7eec0a63f88156d99178b0dc64ad7d040e4`
(`macos` branch). The external converters were loaded successfully against
`zigbee-herdsman-converters` version `26.135.0`.
For macOS, a compatible compiler is available in the `macos` branch of
[flyskywhy/tc32](https://github.com/flyskywhy/tc32). The normal makefile also
supports the Linux compiler installed under `/opt/tc32`.

```sh
make PROJECT_MODEL=_model_6 COMPILE_PREFIX=/path/to/tc32/bin/tc32 -j1
```

Use `_model_1`, `_model_2`, `_model_3`, `_model_4`, or `_model_6` for the
appropriate hardware. Build serially: the upstream makefile shares output
paths and generates a generic native OTA filename. The native OTA file is
`bin/6565-0226-10083001-tuya_battery_switch_ts004x_zed.zigbee`; give it a
model-specific name before building another variant. The second image emitted
by the makefile uses the old Tuya migration header and is not the update image
for an already-converted remote.

Host tests need Python 3, a C compiler (`cc`), and Node.js:

```sh
python3 tests/test_brightness.py
```

They execute the production level-command code and button reader with mocked
GPIO, time, network services, and radio. They check all six endpoints, original
and new modes, single/double clicks, alternating holds, release, group and
unicast bindings, and retransmissions. SDK payload definitions and command IDs
are used directly. Converter lookup values and JavaScript syntax are also
checked. These tests do not emulate the lamps and do not replace the hardware
acceptance test above.
