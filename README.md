# Brightness control without changing on/off state

This fork adds three optional brightness-only button modes to the original
firmware. They allow a remote to control the brightness of several lamp groups
at once while leaving switched-off lamps off. The modes can be assigned
independently to any button; existing modes remain available.

The original project's English documentation is in
[README_ORIG_EN.md](README_ORIG_EN.md).

## New modes

| Switch type | Single click | Double click | Hold |
| --- | --- | --- | --- |
| `brightness_level_no_onoff` | No lighting command | No lighting command | Alternates between increasing and decreasing brightness on successive holds |
| `brightness_level_up_no_onoff` | No lighting command | Increases brightness by 25 units | Increases brightness toward `maxLevel` |
| `brightness_level_down_no_onoff` | No lighting command | Decreases brightness by 25 units | Decreases brightness toward `minLevel` |

Releasing a held button sends `Stop`. None of these modes sends `On`, `Off`, or
`Toggle`. They use Level Control commands without On/Off and explicitly set the
command's `ExecuteIfOff` override to `false`. Lamps implementing that behavior
ignore the command while off; lit lamps change brightness without switching off.

The new mode IDs are `10`, `11`, and `12`. Existing IDs and the settings layout
are preserved. The fork also corrects outdated mode mappings in the bundled
external converters and ensures step-command retries retain the original
command's On/Off variant and payload.

## Install the update

For a compatible TS0046 already running the upstream custom firmware:

1. Download the [TS0046 v1.0.08 OTA image](bin/no_onoff/6565-0226-10083001-tuya_battery_switch_ts004x_zed_model_6.zigbee)
   and install it through Zigbee2MQTT's **Custom firmware** update.
2. Download the [TS0046 external converter](zigbee2mqtt/TS0046-z-SlD.js).
   It can be saved as `TS0046-z-SlD.mjs`. Add it through **Settings → Dev console
   → External converters**, or place it in `external_converters` alongside the
   Zigbee2MQTT `configuration.yaml` and restart Zigbee2MQTT. Keep only one copy
   for this model. In Z2M 2.11 or newer, external converters must be enabled with
   `advanced.enable_external_js: true` if they are disabled.
3. Verify firmware version `v1.0.08`, then select the desired new `switch_type`
   for each button. Wake the remote with four quick button presses before
   changing settings or bindings.

The built-in Z2M converter does not expose these new modes. Matching external
converters are also included for TS0042, TS0043, and TS0044; the included OTA
binary is for TS0046 only.

## Example: independent switching, shared dimming

Keep buttons 3 and 4 switching groups A and B independently. Create another
Zigbee group containing all the individual lamps from A and B, then:

- Set button 5 to `brightness_level_up_no_onoff`.
- Set button 6 to `brightness_level_down_no_onoff`.
- Bind both endpoints to the combined group using `genLevelCtrl`.

If only A is on, only A should change brightness. If both groups are on, both
should change. If both are off, both should remain off. The remote sends the
same command to all lamps; each lamp handles it according to its own on/off
state. After binding, control does not require HA, MQTT, or Z2M to be running,
provided the required Zigbee connectivity remains available.

## Validation

The TS0046 v1.0.08 firmware compiled successfully. Host tests cover original and
new modes on all six endpoints, click/hold/release gestures, group and unicast
bindings, and command retries. All four external converters loaded successfully
with `zigbee-herdsman-converters` 26.135.0. The OTA header, version, lengths, CRC,
and binary checksums were checked.

Physical remote and lamp verification is still pending. See the
[detailed installation, hardware test checklist, and build instructions](doc/brightness-no-onoff.md).

Run the host tests with:

```sh
python3 tests/test_brightness.py
```
