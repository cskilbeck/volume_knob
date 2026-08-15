# Adding the TRS MIDI adapter to the unified configurator

The `midi_adapter` project (`E:\dev\midi_adapter`) is a TRS-to-USB MIDI device that speaks
the same SysEx protocol as the Tiny MIDI Knob. It should appear in this configurator
alongside the knob and the HID keyboards rather than getting an app of its own.

Its firmware plan and the protocol details are in `midi_adapter/firmware/FIRMWARE-PLAN.md`;
the wire format is deliberately identical to the knob's.

## What the adapter is

TRS jack in, USB MIDI out. Three explicit modes — MIDI passthrough, expression pedal,
footswitch — chosen by the user, with **no auto-detection** (the reasoning is in the
firmware plan: an open footswitch is electrically identical to an empty jack, so detection
cannot be complete, and both non-MIDI modes need a calibration step from the user anyway).

Differences from the knob that matter here:

| | Knob | Adapter |
|---|---|---|
| model number | `0x3322` | `0x3323` |
| config device-type nibble | `0x0` | `0x1` |
| config length | 26 bytes | 32 bytes |
| SysEx `0x05` (bootloader) | supported | **not implemented** — the CH32V203 has no software path to its bootloader. Do not offer a firmware-update button for this device. |
| SysEx `0x06` (probe) | — | returns four raw ADC readings, for a future calibration UI |

## What is in the way

Three things in `Midi.js` are hardcoded to the knob:

1. `MIDI_MODEL_NUMBER_LOW/HIGH` — `on_device_id_response()` compares against them and
   returns early otherwise, so the adapter is currently invisible.
2. `CONFIG_LEN`, `config_map`, `default_config`, `flags`, `is_supported_config_version` —
   module-level, closed over by `config_from_bytes()` and `bytes_from_config()`.
3. `DeviceList.vue` renders `<MidiDevice>` for every MIDI device with no dispatch.

Plus: `on_midi_message()` routes only `0xB0` control change to devices. The adapter passes
through arbitrary MIDI, so the monitor needs a general hook.

## Plan

### 1. Device-type registry in `Midi.js`

A table keyed by model number holding what differs — model, config length, config map,
defaults, flags, and which component renders it. `on_device_id_response()` keeps checking
manufacturer and family, then *looks the model up* instead of comparing it, and stores the
entry as `device.type`. Unknown models are ignored and logged, as now.

`config_from_bytes()` and `bytes_from_config()` take the type as a parameter defaulting to
the knob, so existing call sites are untouched.

**The knob's tables deliberately stay in `Midi.js` for now** rather than moving to
per-device modules. This step is the only one that can break shipped hardware, so it is
kept to the smallest diff that works; `midi.default_config` and `midi.flags` continue to
export the knob's values so `MidiDevice.vue` needs no changes at all. That shim is
temporary — new code should read `device.type.*`.

**Verify the knob still enumerates and configures before going further.**

### 2. Adapter entry

Model `0x3323`, 32-byte config map. The map is already written and proven against real
hardware in `midi_adapter/web_test/index.html` — copy it from there rather than
re-deriving it from the C struct.

The struct is `__attribute__((packed))` and carries a `_Static_assert` on its size, because
without packing GCC puts a hole at offset 5 and the map silently decodes one byte out from
there on. A size assert cannot catch a *reordering*, so if `config.h` changes, this map
changes with it.

### 3. Dispatch in `DeviceList.vue`

Same shape as the existing HID dispatch: a name-to-component map, `:is` on the resolved
component, skip if unknown.

### 4. `MidiAdapterDevice.vue` — first cut, deliberately incomplete

- header: name, firmware version, connect toggle (patterns lifted from `MidiDevice.vue`)
- **mode selector** — passthrough / expression / switch — and save
- **MIDI monitor** — decoder already written in `midi_adapter/web_test/index.html`

Deferred: CC number and channel, ranges, direction, flags, thresholds, and the calibration
wizard built on SysEx `0x06`. The full 32-byte config still round-trips intact, because it
is read, one field is edited, and the whole thing is written back.

### 5. General MIDI hook

Route non-SysEx messages to `device.on_midi_in?.(data)` alongside the existing
`on_control_change` path. Additive; the knob is unaffected.

### 6. Dummy adapter

`make_dummy_midi_ports()` is already dependency-injected, so it takes a device type rather
than the knob's constants. Gives a "Demo TRS MIDI Adapter" for UI work with no hardware
attached, exactly as the demo knob does today.

## Later

- **Calibration wizard** on SysEx `0x06`: poll at ~20Hz, show live TIP and RING bars, ask
  the user to sweep the pedal. Whichever bar moves sets the wiper-on-ring flag and its
  extremes capture the input range. This is what replaces auto-detection, and it is better
  than firmware guessing precisely because the browser can ask a question and show a
  result.
- **Port naming.** The adapter's USB-MIDI jack descriptors carry no strings, so the host
  names its ports numerically. Either add string descriptors in firmware or lean on the
  existing localStorage rename in the device header.

## Two devices of the same name (done, 2026-08-15)

Two adapters both report their MIDI port as `midi_adapter`, and four things keyed on that
name: `device_from_input_port()` (which routes the monitor and every CC), the `'connected'`
branch of `on_state_change`, the reuse map in `init_devices()`, and `DeviceList.vue`'s
`:key`. The failure is quiet and confusing — one adapter's traffic draws on the other's
panel, and the second panel looks dead. Two knobs had the same fault; it was never
adapter-specific.

**Run-time identity is now `device.key`, the output port's id.** `device.input_id` is kept
separately because an input's id differs from its output's, and it is deliberately *not*
cleared on disconnect: it is the only way to recognise the port when it returns. The
reconnect path falls back to a name match restricted to devices with no port of that type
bound, which stays unambiguous among duplicates and preserves the old behaviour on a host
that renumbers ids across a replug.

**Persistent identity is the serial number**, which the adapter now appends to its identity
reply (`midi_adapter/firmware/FIRMWARE-PLAN.md` §4). Port ids are unique but not durable,
so per-unit labels in localStorage hang off the serial. Devices without one — the knob —
keep the old name-keyed label, so existing stored labels survive.

Demo port ids are qualified by name for the same reason: they were both hardcoded
`demo-midi-in` / `demo-midi-out` and would have collided under id-keying.
