# <a id="Top">LoraTap Zigbee wireless switch with custom firmware</a>

<img src="doc/images/LoraTap_TS004x.jpg"/>

## [Store link](https://ali.click/09xe01e)

---

> [!WARNING]
> This guide uses the four-button remote as an example.
>
> **The original author accepts no responsibility if using this project damages or disables your remote (switch).**

| Custom Zigbee Model | Original Zigbee Model | Z2M Model | Original Zigbee Manufacturer| Update method | Photo    |
|:-------------------:|:---------------------:|:---------:|:---------------------------:|:-------------:|:--------:|
| TS0041-z-SlD | TS0041 | TS0041 | --- | OTA file | [:camera:](doc/images/TS0041-z-SlD.png) |
| TS0042-z-SlD | TS0042 | [TS0042](https://www.zigbee2mqtt.io/devices/TS0042.html) | _TZ3000_fkvaniuu | [OTA file](https://github.com/slacky1965/tuya_battery_switch_ts004x_zed/raw/refs/heads/main/bin/1141-d3a3-1111114b-tuya_battery_switch_ts004x_zed_model_2.zigbee) | [:camera:](doc/TS0042.md) |
| TS0043-z-SlD | TS0043 | [TS0043](https://www.zigbee2mqtt.io/devices/TS0043.html) | _TZ3000_famkxci2 | [OTA file](https://github.com/slacky1965/tuya_battery_switch_ts004x_zed/raw/refs/heads/main/bin/1141-d3a3-1111114b-tuya_battery_switch_ts004x_zed_model_3.zigbee) | [:camera:](doc/TS0043.md) |
| TS0044-z-SlD | TS0044 | [TS0043](https://www.zigbee2mqtt.io/devices/TS0043.html) | _TZ3000_ee8nrt2l | [OTA file](https://github.com/slacky1965/tuya_battery_switch_ts004x_zed/raw/refs/heads/main/bin/1141-d3a3-1111114b-tuya_battery_switch_ts004x_zed_model_4.zigbee) | [:camera:](doc/TS0044.md) |
| TS0046-z-SlD | TS0046 | [TS0046](https://www.zigbee2mqtt.io/devices/TS0046.html) | _TZ3000_iszegwpd | [OTA file](https://github.com/slacky1965/tuya_battery_switch_ts004x_zed/raw/refs/heads/main/bin/1141-d3a3-1111114b-tuya_battery_switch_ts004x_zed_model_6.zigbee) | [:camera:](doc/TS0046.md) |

Tested only with Zigbee2MQTT. Further comprehensive testing is needed.

## Why use this firmware?

The original firmware could only report single clicks, double clicks, and holds. It did not support binding or group control.

## Updating the firmware

<details>
<summary>Show instructions</summary>

Newer versions of `zigbee2mqtt` make updating much easier.

<img src="doc/images/z2m-version.jpg"/>

Switch to the `zigbee2mqtt-windfront` frontend.

> [!WARNING]
> When this firmware was first released, its converter had not yet been added to `zigbee-herdsman-converters`. For that version of `zigbee2mqtt`, place the [external converter](zigbee2mqtt/TS0044-z-SlD.js) in the `external_converters` directory and restart `zigbee2mqtt`. Later versions with built-in support do not require the external converter.

Download the appropriate update file from this repository; use the table above to select the file for your model. Open the device page and click the cloud icon next to `Firmware version` on the right.

<img src="doc/images/z2m-ota_device.jpg"/>

Select `Custom firmware` from the drop-down menu.

<img src="doc/images/z2m-ota_custom_firmware.jpg"/>

Select the downloaded file.

<img src="doc/images/z2m-ota_file_selection.jpg"/>

Press a button on the remote to wake it, then start the update.

<img src="doc/images/z2m-ota_update.jpg"/>

A rotating circle with arrows on the remote's image indicates that the update has started. `Recent activity` shows the estimated time remaining in seconds and the upload progress as a percentage.

<img src="doc/images/z2m-ota_process.jpg"/>

Progress is also recorded in the log.

<img src="doc/images/z2m-ota_log.jpg"/>

Once the update is complete, the remote is ready to pair. Enable joining in `zigbee2mqtt`, then pair the remote. Remove the old device entry.

</details>

> [!WARNING]
> To enter pairing mode, press any button five times in succession. The LED flashes with each press. After the fifth press, the LED stays on for three seconds. Within those three seconds, press and hold the button for 1–2 seconds. The LED then flashes approximately once per second to indicate pairing mode. Pairing mode lasts up to 90 seconds, after which the LED stops flashing and the remote returns to standby.

---

## Brightness-only modes in this fork

Version `1.0.08` adds three modes selectable independently on any button:

| Switch type | Single click | Double click | Hold |
| --- | --- | --- | --- |
| `brightness_level_no_onoff` | No lighting command | No lighting command | Alternates between increasing and decreasing brightness |
| `brightness_level_up_no_onoff` | No lighting command | Increases brightness by 25 units | Increases brightness toward `maxLevel` |
| `brightness_level_down_no_onoff` | No lighting command | Decreases brightness by 25 units | Decreases brightness toward `minLevel` |

Releasing a held button sends `Stop`. These modes use Level Control commands
without On/Off and explicitly override `ExecuteIfOff` to `false` in each command.
Compliant lamps ignore them while off, even if their stored `ExecuteIfOff` option
is enabled. They do not send `On`, `Off`, or `Toggle`. Existing modes keep their
original behavior, and their numeric IDs and saved settings are preserved.

See [installation, binding, and build instructions](doc/brightness-no-onoff.md).
The firmware compiles and host command tests are available; actual behavior on
Philips Hue and other lamp firmware still needs hardware verification.

## Features

<img src="doc/images/z2m-exposes.jpg"/>

- `Battery`: battery charge as a percentage.
- `Switch actions (Endpoint: X)`: selects the `On`, `Off`, or `Toggle` command when `Switch type` is set to `toggle` or `momentary`. This setting is ignored in other modes.
- `Switch type (Endpoint: X)`: selects what happens when a button is used.
  - `toggle`: sends an `OnOff` command when the button is pressed. `Switch actions (Endpoint: X)` determines the command: `On` always sends `On`, `Off` always sends `Off`, and `toggle` always sends `Toggle`.
  - `momentary`: sends `OnOff` commands on both press and release. With `Switch actions` set to `On`, pressing sends `On` and releasing sends `Off`. With it set to `Off`, pressing sends `Off` and releasing sends `On`. With it set to `toggle`, both pressing and releasing send `Toggle`.
  - `multifunction`: reports `actions` for single, double, and triple clicks, holding, and releasing the button.
  - `brightness_level`: alternates between increasing and decreasing brightness.
    - Single click: sends `Toggle` to change the target device's on/off state.
    - Hold: sends `Move to level`. The target gradually increases or decreases brightness toward the value in `minLevel` or `maxLevel`. The transition is controlled by `onOffTransitionTime`, which can be changed in the developer console; its default value is `25`. Increasing brightness turns on a device that is off. Reaching the minimum brightness does not turn the device off.
  - `brightness_level_up` (**not available on the one-button remote**): increases brightness.
    - Single click: sends `On`.
    - Double click: sends `Step`, increasing brightness by approximately 25 units. If the device is off, it turns on.
    - Hold: sends `Move to level`, gradually increasing brightness toward `maxLevel`. The increase continues while the button is held, until the maximum is reached. The transition is controlled by `onOffTransitionTime`, which can be changed in the developer console; its default value is `25`. If the device is off, it turns on.
  - `brightness_level_down` (**not available on the one-button remote**): decreases brightness.
    - Single click: sends `Off`.
    - Double click: sends `Step`, decreasing brightness by approximately 25 units. If an on device reaches its minimum level, it turns off.
    - Hold: sends `Move to level`, gradually decreasing brightness toward `minLevel`. The decrease continues while the button is held, until the minimum is reached. The transition is controlled by `onOffTransitionTime`, which can be changed in the developer console; its default value is `25`. Reaching the minimum level during a hold does not turn the device off.
  - `scene`: recalls scenes created on the target device or group using the `Scene` tab. Pressing the button sends `Recall scene` to that device or group.
- `Scene id (Endpoint: X)`: scene number, from `0` to `255`.
- `Group id (Endpoint: X)`: the group number if the scene was created in a group, from `1` to `0xfff7`. Leave this at `0` if the scene was not created in a group.
- `Min level`: the minimum permitted brightness level.
- `Max level`: the maximum permitted brightness level.
- `Action`: displays the names of received commands and attributes as text.

`(Endpoint: X)` identifies the endpoint number, from `1` to `6`, depending on the number of buttons. Accordingly, there are between one and six sets of `Switch actions`, `Switch type`, `Scene id`, `Group id`, `Min level`, and `Max level` settings.

> [!WARNING]
> Before confirming changes to these settings, reporting, binding, or other parameters in the web interface, wake the remote by pressing any button four times in succession. It stays awake for 20 seconds.

---

## Reporting

Only the battery supports conventional attribute reporting. Its reporting behavior has been adjusted to conserve battery power. Battery measurements and reports depend on `Max rep interval`.

<img src="doc/images/z2m-battery_reporting.jpg"/>

When no buttons are pressed, the remote sleeps for the interval specified by `Max rep interval`. In the screenshot, this is 14,400 seconds, or four hours. After that interval, the remote wakes, measures the battery, sends a report, and sleeps for the same interval again. **To force a battery report, press any button four times in succession.**

## Binding

Binding tells the remote where to send its commands. Bind the `genOnOff` cluster for on/off commands. To control both on/off state and brightness, bind both `genOnOff` and `genLevelCtrl`. Bulbs usually act as Zigbee routers, so a directly bound remote and bulb should continue working even if the coordinator is unavailable, provided the required Zigbee connectivity remains available.

You can bind to an individual device or an entire **group**.

You can also bind to a device that supports `Find and bind`. To put the remote into discovery mode, quickly press the desired button four times, then press and hold that same button within three seconds.

## Groups

Groups provide another way to control lights without configuring binding. For example, configure button 2 to turn on the light and increase brightness, and button 3 to turn off the light and decrease brightness.

<img src="doc/images/z2m-dev_settings.jpg"/>

Create a group with a name and an ID, for example `10`. This ID is also the group's Zigbee network address. Add the bulb's `Endpoint 1` and the remote's `Endpoint 2` and `Endpoint 3` to this group, using the endpoints configured for light control.

<img src="doc/images/z2m-group.jpg"/>

No further settings are required. Pressing button 2 or 3 now sends commands to the bulb.

---

Contact the original author on **[Telegram](https://t.me/slacky1965)**.

### To support the original author, you can donate via [YooMoney](https://yoomoney.ru/to/4100118300223495)

## Version history

- 1.0.08 (this fork)
  - Added the three per-button brightness-only modes described above.
  - Updated the external converters for all included remote models, correcting their outdated mode IDs to match the firmware.
  - Fixed step-command retries to preserve the original command's On/Off variant and payload.
  - Added host tests for button gestures, command selection, and retransmission.

- 1.0.01
  - Added the four-button remote `TS0044 _TZ3000_ee8nrt2l`.
- 1.0.02
  - Added the two-button remote `TS0042 _TZ3000_fkvaniuu`.
  - Fixed `momentary` mode: the remote previously went to sleep before the release command could be sent.
- 1.0.03
  - Added the three-button remote `TS0043 _TZ3000_famkxci2`.
  - Added the six-button remote `TS0046 _TZ3000_iszegwpd`.
  - Fixed a problem where any button could trigger group control.
- 1.0.04
  - Changed how commands and reports are sent, intended to fix commands occasionally failing to reach a bound device.
- 1.0.05
  - Added the ability to put the chip to sleep by holding a button for a long time.
  - Reduced the time spent awake after a button press.
- 1.0.06
  - Changed brightness-command handling.
  - Added binding through `Find and bind`.
  - Added remote restart: quickly press any button ten times in succession.
  - Added delivery checks using `aps ack`. A command is sent again if its first delivery returns an `aps ack` error.
  - Removed unused endpoints. Previously all six were shown; now, for example, a two-button remote shows only two endpoints.
- 1.0.07
  - Extended the time the remote stays awake during pairing.

[Back to top](#Top)
