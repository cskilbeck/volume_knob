<script setup>

//////////////////////////////////////////////////////////////////////
// TRS MIDI adapter — first cut.
//
// Deliberately incomplete: mode selection and a live MIDI monitor, nothing
// else. The remaining config (CC number, channel, ranges, direction, switch
// thresholds) and the calibration wizard are listed in ADAPTER-INTEGRATION.md.
//
// The whole 32-byte config still round-trips intact, because it is read from
// the device, one field is changed, and the whole thing is written back.
//
// Layout deliberately mirrors MidiDevice.vue — same container, same collapse
// arrow, same button group, same bordered section panels — so the two devices
// read as members of one application rather than two.

import { ref, computed, onMounted, onUnmounted, nextTick } from 'vue';

import midi, { ADAPTER_MODE, ADAPTER_FORMAT_DEADZONE, ADAPTER_FORMAT_SCHMITT,
               ADAPTER_FORMAT_CURVE } from '../midi/Midi.js'
import CC from '../midi/CC.js'
import CCDropDown from './CCDropDown.vue'
import Modal from './Modal.vue'
import RangePair from './RangePair.vue'
import ValueSlider from './ValueSlider.vue'

//////////////////////////////////////////////////////////////////////

const props = defineProps({
    device: {
        type: Object,
        required: true
    }
});

//////////////////////////////////////////////////////////////////////
// A friendlier name than the raw port. The adapter's USB-MIDI jack descriptors
// carry no strings, so the host names its ports itself and the result is not
// always presentable. Same localStorage trick the knob's panel uses.

// Stored against the serial number when the device reports one, because the
// port name does not distinguish two adapters — they would share one label,
// and renaming either would rename both. The port id is unique but is not
// stable enough to persist against, so the serial is the only durable answer.
// Devices with no serial keep using the name, exactly as before.

function cookie_name() {
    const d = props.device;
    return d.serial ? `midi_adapter_${d.serial}_label` : `${d.name}_label`;
}

// No serial badge in the heading: the port name already ends in a per-board
// hash of the same UID (firmware, usb_descriptors.c: midi_port_name), so a
// badge beside it would be a second identifier for the same board. The full 24
// characters remain on the heading's tooltip, and still key the label below.

let device_label = ref(localStorage.getItem(cookie_name()) || "Unnamed");

function save_name() {
    if (device_label.value.length == 0) {
        device_label.value = "Unnamed";
    }
    localStorage.setItem(cookie_name(), device_label.value);
}

//////////////////////////////////////////////////////////////////////
// config

const MODES = [
    { value: ADAPTER_MODE.midi, label: 'MIDI passthrough',
      hint: 'TRS MIDI in, straight out over USB. Nothing to configure.' },
    { value: ADAPTER_MODE.expression, label: 'Expression pedal',
      hint: 'A passive pot becomes CC. Needs calibrating for its travel.' },
    { value: ADAPTER_MODE.switch, label: 'Footswitch',
      hint: 'A switch becomes CC, on or off.' },
];

const loaded = ref(false);
const saving = ref(false);
const mode = ref(ADAPTER_MODE.midi);

// Calibration writes into these, and "Store to device" sends them.
const in_min = ref(0);
const in_max = ref(4095);
const wiper_on_ring = ref(false);

// Output values, held in the DISPLAY domain (0..127 or 0..16383 depending on
// Extended CC) and converted at the pull/store boundary. See out_scale below.
const out_min = ref(0);
const out_max = ref(127);
const sw_value_off = ref(0);
const sw_value_on = ref(127);

// Bends the pedal's response so a linear pot feels like an audio-taper pot.
//
// Held as a boolean because that is all the device stores, but presented as a
// "Response" choice with a drawn curve rather than as a checkbox: the word
// "log" is actively misleading here — a log-taper pot's transfer curve bends
// UPWARDS and looks exponential — so the graph is doing most of the explaining.
const log_curve = ref(false);

// Latch rather than follow: each press flips between the two values, instead of
// the output tracking the contact. The firmware acts on the press edge only, so
// a momentary switch behaves like a latching one.
const sw_toggle = ref(false);

// Which two conductors the footswitch bridges. Decided by calibration rather
// than asked, because the two wirings need different MEASUREMENTS and the user
// has no way to know which they have — see sw_closedness() below.
const sw_on_ring = ref(false);

// Schmitt trigger edges for the footswitch, in raw counts of "closedness".
// Independent because some footswitches have a limited output range that is not
// centred anywhere convenient.
const sw_low = ref(1792);
const sw_high = ref(2304);

// Reverses the output in both modes: heel and toe swap for a pedal, and the
// two values swap for a footswitch. It exists as a flag rather than as a
// crossed output range because the pedal's range is ordered, and because a
// checkbox says what it means.
const invert = ref(false);

// Percent of the calibrated sweep ignored at each end, independently.
const dz_heel = ref(0);
const dz_toe = ref(0);

const cc_msb = ref(11);
const cc_lsb = ref(43);
const channel = ref(0);
const extended = ref(false);

const AF_WIPER_ON_RING = 0x0001;
const AF_INVERT = 0x0002;
const AF_SW_ON_RING = 0x0008;
const AF_SW_TOGGLE = 0x0020;
const AF_LOG_CURVE = 0x0080;
const AF_14BIT = 0x0004;

// 14-bit pairs CC n with CC n+32 by convention, so turning Extended on fills
// the LSB in rather than making the user look it up.
//
// It hangs off the checkbox, and ONLY the checkbox. It used to hang off the CC
// selector, where it also set `extended` itself — which meant nudging the CC
// number field through 0..31 silently switched 14-bit on and off, because every
// controller in that range is an MSB type. `extended` is now only ever changed
// by the user clicking here, or by a config arriving from the device.
//
// Reads the event rather than `extended`: v-model and this handler both listen
// for 'change', and relying on which runs first is a trap.
function extended_changed(event) {
    // Was the OPPOSITE of what it is now, since v-model has already run.
    rescale_outputs(!event.target.checked);

    if (!event.target.checked) return;
    const cc = CC.CCs[cc_msb.value];
    if (CC.is_MSB(cc)) {
        cc_lsb.value = cc.alt;
    }
}

//////////////////////////////////////////////////////////////////////
// 7-bit vs 14-bit output values.
//
// The config stores everything in the 14-bit domain and the firmware derives
// 7-bit output with >> 7. The UI shows whichever domain the Extended CC
// checkbox selects, so the numbers on screen are the numbers a DAW will see.
//
// The scale factor is 129, NOT 128 (i.e. not a shift), because 16383 = 127*129
// exactly. That gives three things a shift does not:
//
//   - full scale maps to full scale: 127 <-> 16383, so toggling Extended does
//     not silently rewrite 16383 as 16256 and light up the unsaved-changes nag
//     for no visible reason;
//   - the round trip is exact for every value, since N*129/129 == N;
//   - it still agrees with the firmware, because (N*129) >> 7 == N for all
//     N <= 127 — the extra 129th is exactly what compensates for >> 7 losing
//     the remainder.
const OUT_SCALE = 129;

const out_domain_max = computed(() => extended.value ? 16383 : 127);

function to_display(v14) {
    return extended.value ? v14 : Math.round(v14 / OUT_SCALE);
}

function to_config(vdisp) {
    return extended.value ? vdisp : vdisp * OUT_SCALE;
}

// Extended CC changes what the on-screen numbers MEAN, so they are re-derived
// through the device's stored values rather than reinterpreted in place —
// otherwise a 100 entered as 7-bit would silently become 100 out of 16383.
// The pedal's output range must be ordered with a span of at least 1: a crossed
// pair reads as a mistake, and a zero span pins the output to one value, which
// looks identical to a broken pedal. RangePair's clamping keeps it that way once
// it is true, but cannot repair a pair that arrives crossed — from a device
// storing one, or from rounding when the 7/14-bit domain changes — so it is
// enforced here at both of those entry points.
//
// Deliberately NOT applied to the footswitch's two values: "off" above "on" is a
// legitimate setup there, not an error.
function normalise_out_range() {
    const m = out_domain_max.value;
    const lo = Math.max(0, Math.min(m - 1, Math.round(out_min.value) || 0));
    const hi = Math.max(lo + 1, Math.min(m, Math.round(out_max.value) || 0));
    out_min.value = lo;
    out_max.value = hi;
}

function rescale_outputs(was_extended) {
    const conv = (v) => was_extended ? Math.round(v / OUT_SCALE) : v * OUT_SCALE;
    out_min.value = conv(out_min.value);
    out_max.value = conv(out_max.value);
    sw_value_off.value = conv(sw_value_off.value);
    sw_value_on.value = conv(sw_value_on.value);

    // 14-bit -> 7-bit divides, so two values 129 apart can round to the same
    // number and collapse the span to zero.
    normalise_out_range();
}

//////////////////////////////////////////////////////////////////////
// Deadzones arrived in config format 0x1, so a device on older firmware has no
// such fields — the bytes it would write them into are padding it ignores.
// Offering the control there would let the user set a value, store it, and see
// no effect whatsoever, so it is hidden instead.
const device_format = computed(
    () => (props.device.config?.version ?? 0) & 0xF);

const has_deadzone = computed(
    () => device_format.value >= ADAPTER_FORMAT_DEADZONE);

// Below format 0x2 these two bytes mean a centre and a half-width, so offering
// edge controls would write values the device reads as something else entirely.
const has_schmitt = computed(
    () => device_format.value >= ADAPTER_FORMAT_SCHMITT);

const has_curve = computed(
    () => device_format.value >= ADAPTER_FORMAT_CURVE);

//////////////////////////////////////////////////////////////////////
// The response curve, drawn. Mirrors map_value(): the live travel is
// normalised, cubed when the flag is set, then scaled into the output range —
// so this is the shape of the actual thing, not an illustration of it.

const CURVE_W = 68;
const CURVE_H = 38;

const curve_path = computed(() => {
    const N = 24;
    const pts = [];
    for (let i = 0; i <= N; i++) {
        const x = i / N;
        const y = log_curve.value ? x * x * x : x;
        pts.push(`${(x * CURVE_W).toFixed(1)},${(CURVE_H - y * CURVE_H).toFixed(1)}`);
    }
    return 'M' + pts.join(' L');
});

const curve_hint = computed(() => log_curve.value
    ? 'Log response: the output moves slowly near the heel and quickly near the toe, so a linear pot feels like an audio-taper pot. Halfway along the travel sends about an eighth of the range. Usually what you want for volume.'
    : 'Linear response: the output tracks pedal position directly, in equal steps. Halfway along the travel sends half the range.');

// Mirrors DZ_MAX_PERCENT in the firmware, which clamps these anyway. The pair
// is also held apart so some live travel always remains: a pedal with no live
// travel is indistinguishable from a broken one.
const DZ_MAX = 45;
const DZ_LIVE_MIN = 10;

const dz_heel_max = computed(
    () => Math.min(DZ_MAX, 100 - DZ_LIVE_MIN - dz_toe.value));
const dz_toe_max = computed(
    () => Math.min(DZ_MAX, 100 - DZ_LIVE_MIN - dz_heel.value));

const dz_live = computed(() => 100 - dz_heel.value - dz_toe.value);

//////////////////////////////////////////////////////////////////////
// Firmware update help.
//
// The canonical instructions are midi_adapter/FLASHING.md, published as a page.
// What lives here is deliberately only the SHORT version: the button timing and
// the handful of steps, which are the parts that almost never change. Anything
// long-tailed — installing the tools, troubleshooting, unlocking a chip that has
// been read-protected — is linked rather than copied, because a stale copy of a
// troubleshooting table is worse than no copy.
//
// Kept here rather than fetched: someone reading this is part-way through a
// firmware update, which is the worst possible moment to depend on the network.

const FLASHING_URL = 'https://skilbeck.com/midi-adapter-firmware-flashing/';

// Deliberately the unversioned "latest" copy, not a versioned filename: a direct
// link is one click fewer than sending people to a page to hunt for the file,
// and pointing at _latest means a firmware release does not also require this
// app to be edited and redeployed. Versioned copies are published alongside it
// for anyone who needs a specific one.
const FIRMWARE_URL = 'https://skilbeck.com/content/files/midi_adapter_latest.bin';

const firmwareModal = ref(false);

// Defaults to the platform in use, because half the instructions are irrelevant
// to any given reader and this is not an audience to make hunt. Still a choice,
// not a verdict — people do help each other on the wrong machine.
function detect_mac() {
    const p = navigator.userAgentData?.platform || navigator.platform || navigator.userAgent || '';
    return /mac|iphone|ipad/i.test(p);
}

const show_mac = ref(detect_mac());

const mode_hint = computed(
    () => MODES.find(m => m.value === mode.value)?.hint ?? '');

//////////////////////////////////////////////////////////////////////
// Tooltips for the two checkboxes whose effect is easy to get backwards.
//
// Each states what the device WILL DO as things currently stand, rather than
// what ticking the box would change. The earlier wording described the effect
// ("the open and closed values swap"), which read as a promise when the box was
// clear and as a description when it was set — so it was wrong half the time.

const toggle_hint = computed(() => sw_toggle.value
    ? 'Toggle mode is on: each press flips between the two values, and releasing does nothing.'
    : 'Toggle mode is off: the output follows the switch, sending one value while it is held and the other while it is released.');

const invert_hint = computed(() => {
    if (mode.value === ADAPTER_MODE.expression) {
        return invert.value
            ? 'Inverted: pressing the pedal down moves the output from the Toe value towards the Heel value.'
            : 'Not inverted: pressing the pedal down moves the output from the Heel value towards the Toe value.';
    }
    if (sw_toggle.value) {
        return invert.value
            ? 'Inverted: the first press sends the Off value and the next sends On.'
            : 'Not inverted: the first press sends the On value and the next sends Off.';
    }
    return invert.value
        ? 'Inverted: pressing sends the Off value and releasing sends On.'
        : 'Not inverted: pressing sends the On value and releasing sends Off.';
});

const firmware = computed(() => props.device.firmware_version_str ?? '?');

// Mirrors the knob's affordance: "Store to device" goes red once the UI has
// drifted from what the device is actually running.
const config_changed = computed(() => {
    if (!loaded.value) return false;
    const c = props.device.config;
    return mode.value !== c.mode ||
           in_min.value !== c.in_min ||
           in_max.value !== c.in_max ||
           cc_msb.value !== c.cc_msb ||
           cc_lsb.value !== c.cc_lsb ||
           channel.value !== c.channel ||
           wiper_on_ring.value !== !!(c.flags & AF_WIPER_ON_RING) ||
           extended.value !== !!(c.flags & AF_14BIT) ||
           to_config(out_min.value) !== c.out_min ||
           to_config(out_max.value) !== c.out_max ||
           to_config(sw_value_off.value) !== c.sw_value_off ||
           to_config(sw_value_on.value) !== c.sw_value_on ||
           dz_heel.value !== (c.dz_heel ?? 0) ||
           dz_toe.value !== (c.dz_toe ?? 0) ||
           invert.value !== !!(c.flags & AF_INVERT) ||
           log_curve.value !== !!(c.flags & AF_LOG_CURVE) ||
           sw_low.value !== c.sw_low ||
           sw_high.value !== c.sw_high ||
           sw_on_ring.value !== !!(c.flags & AF_SW_ON_RING) ||
           sw_toggle.value !== !!(c.flags & AF_SW_TOGGLE);
});

function pull_from_device() {
    const c = props.device.config;
    mode.value = c.mode ?? ADAPTER_MODE.midi;
    in_min.value = c.in_min ?? 0;
    in_max.value = c.in_max ?? 4095;
    cc_msb.value = c.cc_msb ?? 11;
    cc_lsb.value = c.cc_lsb ?? 43;
    channel.value = c.channel ?? 0;
    wiper_on_ring.value = !!(c.flags & AF_WIPER_ON_RING);
    extended.value = !!(c.flags & AF_14BIT);
    invert.value = !!(c.flags & AF_INVERT);
    log_curve.value = !!(c.flags & AF_LOG_CURVE);

    // After `extended`, which is what to_display() keys off.
    out_min.value = to_display(c.out_min ?? 0);
    out_max.value = to_display(c.out_max ?? 16383);
    sw_value_off.value = to_display(c.sw_value_off ?? 0);
    sw_value_on.value = to_display(c.sw_value_on ?? 16383);

    dz_heel.value = Math.min(DZ_MAX, c.dz_heel ?? 0);
    dz_toe.value = Math.min(DZ_MAX, c.dz_toe ?? 0);

    sw_low.value = c.sw_low ?? 1792;
    sw_high.value = c.sw_high ?? 2304;
    sw_on_ring.value = !!(c.flags & AF_SW_ON_RING);
    sw_toggle.value = !!(c.flags & AF_SW_TOGGLE);

    // A device holding a crossed range gets corrected here, which shows up as
    // unsaved changes — correctly, because the fix is not on the device yet.
    normalise_out_range();

    loaded.value = true;
}

function store_config() {
    const c = props.device.config;
    c.mode = mode.value;
    c.in_min = in_min.value;
    c.in_max = in_max.value;
    c.cc_msb = cc_msb.value;
    c.cc_lsb = cc_lsb.value;
    c.channel = Math.max(0, Math.min(15, channel.value | 0));
    c.out_min = to_config(out_min.value);
    c.out_max = to_config(out_max.value);
    c.sw_value_off = to_config(sw_value_off.value);
    c.sw_value_on = to_config(sw_value_on.value);
    c.dz_heel = dz_heel.value;
    c.dz_toe = dz_toe.value;
    c.sw_low = sw_low.value;
    c.sw_high = sw_high.value;

    // c.version is deliberately left as the device reported it. Writing this
    // app's newest format to a device on older firmware makes that firmware
    // reject the whole config and fall back to defaults — see the note on
    // adapter_config_map in Midi.js.

    let f = c.flags;
    f = wiper_on_ring.value ? (f | AF_WIPER_ON_RING) : (f & ~AF_WIPER_ON_RING);
    f = extended.value ? (f | AF_14BIT) : (f & ~AF_14BIT);
    f = invert.value ? (f | AF_INVERT) : (f & ~AF_INVERT);
    f = log_curve.value ? (f | AF_LOG_CURVE) : (f & ~AF_LOG_CURVE);
    f = sw_on_ring.value ? (f | AF_SW_ON_RING) : (f & ~AF_SW_ON_RING);
    f = sw_toggle.value ? (f | AF_SW_TOGGLE) : (f & ~AF_SW_TOGGLE);
    c.flags = f;
    saving.value = true;
    midi.write_flash(props.device.device_index);
}

//////////////////////////////////////////////////////////////////////
// Calibration.
//
// The pot divides against R8's 1k, so a pedal never reaches full scale and its
// travel differs from every other pedal — the endpoints cannot be assumed and
// have to be measured. Sweeping also settles the wiring question for free:
// whichever conductor moves IS the wiper, which is the thing auto-detection
// could never have decided reliably and a human can see at a glance.

const PROBE_MS = 50;              // 20Hz: live enough to follow a sweep
const MIN_TRAVEL = 200;           // counts; below this nothing really moved

const calibrating = ref(false);
const cal = ref(null);
let probe_timer = null;

function blank_cal() {
    return {
        tip: 0, ring: 0,
        tip_min: 65535, tip_max: 0,
        ring_min: 65535, ring_max: 0,
    };
}

const cal_travel = computed(() => {
    if (!cal.value) return { tip: 0, ring: 0 };
    const c = cal.value;
    return {
        tip: Math.max(0, c.tip_max - c.tip_min),
        ring: Math.max(0, c.ring_max - c.ring_min),
    };
});

const cal_enough = computed(
    () => Math.max(cal_travel.value.tip, cal_travel.value.ring) >= MIN_TRAVEL);

function on_probe(raw) {
    if (!calibrating.value || !cal.value) return;

    // Four uint16 LE: TIP with RING high, TIP with RING low, RING with TIP
    // high, RING with TIP low. A pot's wiper follows the driven conductor, so
    // the two "driven high" readings are the ones that move.
    const tip = raw[0] | (raw[1] << 8);
    const ring = raw[4] | (raw[5] << 8);

    const c = cal.value;
    c.tip = tip;
    c.ring = ring;
    if (tip < c.tip_min) c.tip_min = tip;
    if (tip > c.tip_max) c.tip_max = tip;
    if (ring < c.ring_min) c.ring_min = ring;
    if (ring > c.ring_max) c.ring_max = ring;
}

//////////////////////////////////////////////////////////////////////
// Footswitch calibration.
//
// Same shape as the pedal's, and for the same reason: the numbers cannot be
// assumed. Reports of switches with a limited output range are what prompted
// the independent sw_low/sw_high edges, and thresholds in raw ADC counts are
// unusable unless the user can see what their own switch actually reads.
//
// It also settles the WIRING by observation, exactly as the pedal sweep settles
// which conductor carries the wiper. The two switch wirings need different
// measurements and neither can see the other:
//
//   tip-ring     the switch joins the two conductors, so TIP follows RING when
//                RING is driven. Visible as a large differential.
//   tip-sleeve   the switch shorts TIP to ground. The conductors are never
//                joined, so driving RING reveals nothing at all — only the
//                internal pull-up read sees it.
//
// So whichever measurement MOVES tells us which wiring is present, and that is
// something no amount of firmware cleverness could have decided on its own.

const SW_MIN_TRAVEL = 200;        // counts; below this the switch did not move
const SW_ON_FRACTION = 0.7;       // where to put the closing edge in the span
const SW_OFF_FRACTION = 0.3;      // and the opening edge

const sw_calibrating = ref(false);
const sw_cal = ref(null);

function blank_sw_cal() {
    return {
        diff: 0, diff_min: 65535, diff_max: 0,
        pull: 0, pull_min: 65535, pull_max: 0,
        seen_pullup: false,
    };
}

const sw_cal_travel = computed(() => {
    if (!sw_cal.value) return { diff: 0, pull: 0 };
    const c = sw_cal.value;
    return {
        diff: Math.max(0, c.diff_max - c.diff_min),
        pull: Math.max(0, c.pull_max - c.pull_min),
    };
});

// The wiring that shows the most movement is the wiring that is present.
const sw_cal_on_ring = computed(
    () => sw_cal_travel.value.diff > sw_cal_travel.value.pull);

const sw_cal_enough = computed(
    () => Math.max(sw_cal_travel.value.diff, sw_cal_travel.value.pull) >= SW_MIN_TRAVEL);

// Mirrors switch_closedness() in the firmware: both wirings normalised so that
// bigger means more closed, which is the domain sw_low/sw_high live in.
function sw_closedness(raw) {
    const tip_hi = raw[0] | (raw[1] << 8);
    const tip_lo = raw[2] | (raw[3] << 8);
    const diff = Math.max(0, tip_hi - tip_lo);

    // Absent on firmware that predates the fifth probe reading.
    const has_pullup = raw.length >= 10;
    const pull = has_pullup ? 4095 - (raw[8] | (raw[9] << 8)) : 0;

    return { diff, pull, has_pullup };
}

function on_sw_probe(raw) {
    if (!sw_calibrating.value || !sw_cal.value) return;

    const { diff, pull, has_pullup } = sw_closedness(raw);
    const c = sw_cal.value;

    c.diff = diff;
    if (diff < c.diff_min) c.diff_min = diff;
    if (diff > c.diff_max) c.diff_max = diff;

    c.seen_pullup = has_pullup;
    if (has_pullup) {
        c.pull = pull;
        if (pull < c.pull_min) c.pull_min = pull;
        if (pull > c.pull_max) c.pull_max = pull;
    }
}

function start_sw_calibration() {
    sw_cal.value = blank_sw_cal();
    sw_calibrating.value = true;
    probe_timer = setInterval(() => {
        if (!props.device.active) {
            cancel_sw_calibration();
            return;
        }
        midi.probe_device(props.device.device_index);
    }, PROBE_MS);
}

function accept_sw_calibration() {
    stop_polling();
    sw_calibrating.value = false;

    const c = sw_cal.value;
    const on_ring = sw_cal_on_ring.value;
    const lo = on_ring ? c.diff_min : c.pull_min;
    const hi = on_ring ? c.diff_max : c.pull_max;
    const span = Math.max(0, hi - lo);

    sw_on_ring.value = on_ring;

    // Edges placed inside the observed span rather than at its ends: a switch
    // that only just reached its extremes during calibration would otherwise
    // sit exactly on an edge in use and chatter.
    sw_low.value = Math.round(lo + span * SW_OFF_FRACTION);
    sw_high.value = Math.round(lo + span * SW_ON_FRACTION);

    sw_cal.value = null;
}

function cancel_sw_calibration() {
    stop_polling();
    sw_calibrating.value = false;
    sw_cal.value = null;
}

function start_calibration() {
    cal.value = blank_cal();
    calibrating.value = true;
    probe_timer = setInterval(() => {
        // Unplugging mid-calibration would otherwise throw on every tick.
        if (!props.device.active) {
            cancel_calibration();
            return;
        }
        midi.probe_device(props.device.device_index);
    }, PROBE_MS);
}

function stop_polling() {
    if (probe_timer) { clearInterval(probe_timer); probe_timer = null; }
}

function accept_calibration() {
    stop_polling();
    calibrating.value = false;

    const t = cal_travel.value;
    const c = cal.value;

    // Whichever conductor moved is the wiper.
    if (t.ring > t.tip) {
        wiper_on_ring.value = true;
        in_min.value = c.ring_min;
        in_max.value = c.ring_max;
    } else {
        wiper_on_ring.value = false;
        in_min.value = c.tip_min;
        in_max.value = c.tip_max;
    }
    cal.value = null;
}

function cancel_calibration() {
    stop_polling();
    calibrating.value = false;
    cal.value = null;
}

function read_from_device() {
    midi.read_flash(props.device.device_index);
}

//////////////////////////////////////////////////////////////////////
// collapse — same arrow and transition as the knob and HID panels

let collapsed = ref(false);

function toggle_expand() {
    collapsed.value = !collapsed.value;
}

function rotation_matrix(cx, cy, angle) {
    const rad = angle * Math.PI / 180;
    const cos = Math.cos(rad), sin = Math.sin(rad);
    return `matrix(${cos} ${sin} ${-sin} ${cos} ` +
           `${cx - cx * cos + cy * sin} ${cy - cx * sin - cy * cos})`;
}

//////////////////////////////////////////////////////////////////////
// MIDI monitor — what the host reconstructed from the device's USB-MIDI
// packets, which for passthrough mode is the entire product working.

const MONITOR_MAX = 1000;

const monitor = ref([]);
const monitor_el = ref(null);

// Rows land here first and are flushed to the reactive list on a timer.
//
// In expression mode the device can emit 200 CC/second, and pushing each one
// straight into a reactive array re-renders a keyed v-for of up to MONITOR_MAX
// rows every time. That is enough to lock the page up hard — which also stops
// a scan completing, so it looks like devices have stopped appearing rather
// than like a slow list.
let pending_rows = [];
let flush_timer = null;

const FLUSH_MS = 100;
const hide_realtime = ref(true);
const counts = ref({ total: 0, note: 0, cc: 0, rt: 0, other: 0 });

function describe(d) {
    const s = d[0], ch = (s & 0x0f) + 1;

    switch (s & 0xf0) {
        // Velocity 0 is a note off by convention, and is what running status
        // emits, so showing it as such keeps the monitor honest.
        case 0x90: return d[2] === 0
            ? `note off   ch${ch}  note ${d[1]}  (vel 0)`
            : `note on    ch${ch}  note ${d[1]}  vel ${d[2]}`;
        case 0x80: return `note off   ch${ch}  note ${d[1]}  vel ${d[2]}`;
        case 0xa0: return `aftertouch ch${ch}  note ${d[1]}  ${d[2]}`;
        case 0xb0: return `CC         ch${ch}  cc ${d[1]}  val ${d[2]}`;
        case 0xc0: return `program    ch${ch}  ${d[1]}`;
        case 0xd0: return `pressure   ch${ch}  ${d[1]}`;
        case 0xe0: return `pitchbend  ch${ch}  ${((d[2] << 7) | d[1]) - 8192}`;
    }

    switch (s) {
        case 0xf1: return `mtc quarter frame ${d[1]}`;
        case 0xf2: return `song position ${(d[2] << 7) | d[1]}`;
        case 0xf3: return `song select ${d[1]}`;
        case 0xf6: return 'tune request';
        case 0xf8: return 'clock';
        case 0xfa: return 'start';
        case 0xfb: return 'continue';
        case 0xfc: return 'stop';
        case 0xfe: return 'active sensing';
        case 0xff: return 'reset';
    }
    return 'unrecognised';
}

// Scroll after Vue has rendered the new row, and only once per tick however
// many messages arrived — a burst of 64 back-to-back CCs would otherwise
// schedule 64 scrolls of a list that is about to change again anyway.
let scroll_pending = false;

function scroll_to_bottom() {
    if (scroll_pending) return;
    scroll_pending = true;

    nextTick(() => {
        scroll_pending = false;
        const el = monitor_el.value;
        if (el) el.scrollTop = el.scrollHeight;
    });
}

function on_midi_in(data) {
    const d = Array.from(data);
    const s = d[0];
    const realtime = s >= 0xf8;

    const c = counts.value;
    c.total++;
    if (realtime) c.rt++;
    else if ((s & 0xf0) === 0x80 || (s & 0xf0) === 0x90) c.note++;
    else if ((s & 0xf0) === 0xb0) c.cc++;
    else c.other++;

    if (realtime && hide_realtime.value) return;

    pending_rows.push({
        seq: c.total,
        hex: d.map(b => b.toString(16).padStart(2, '0')).join(' '),
        text: describe(d),
    });

    // Bound the queue too: a flood between flushes must not grow without limit.
    if (pending_rows.length > MONITOR_MAX) {
        pending_rows.splice(0, pending_rows.length - MONITOR_MAX);
    }

    if (flush_timer === null) {
        flush_timer = setTimeout(flush_rows, FLUSH_MS);
    }
}

function flush_rows() {
    flush_timer = null;

    if (pending_rows.length === 0) return;

    // Loop rather than push(...spread): the spread form passes one argument per
    // row and would blow the stack on a large batch.
    for (const row of pending_rows) {
        monitor.value.push(row);
    }
    pending_rows = [];

    if (monitor.value.length > MONITOR_MAX) {
        monitor.value.splice(0, monitor.value.length - MONITOR_MAX);
    }

    scroll_to_bottom();
}

function clear_monitor() {
    pending_rows = [];
    monitor.value = [];
    counts.value = { total: 0, note: 0, cc: 0, rt: 0, other: 0 };
}

//////////////////////////////////////////////////////////////////////

onUnmounted(() => {
    stop_polling();
    if (flush_timer !== null) {
        clearTimeout(flush_timer);
        flush_timer = null;
    }
});

onMounted(() => {
    props.device.on_config_loaded = pull_from_device;
    // One probe reply, two possible consumers. Routed rather than merged: the
    // two calibrations read different values out of the same payload and only
    // one can be running, since both drive the single probe_timer.
    props.device.on_probe = (raw) => {
        on_probe(raw);
        on_sw_probe(raw);
    };
    props.device.on_config_saved = () => { saving.value = false; };
    props.device.on_midi_in = on_midi_in;

    // The config may already have arrived before this component mounted.
    if (props.device.config && props.device.config.version !== undefined) {
        pull_from_device();
    }
});

</script>

<template>

    <svg class='d-none'>
        <symbol id='little-arrow' xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="currentColor" viewBox="0 0 16 16">
            <path d="m12.14 8.753-5.482 4.796c-.646.566-1.658.106-1.658-.753V3.204a1 1 0 0 1 1.659-.753l5.48 4.796a1 1 0 0 1 0 1.506z" />
        </symbol>
    </svg>

    <div class="container border rounded-0 bg-device border-secondary bg-secondary-subtle pt-2 mb-4"
         :class="collapsed ? 'pb-2' : ' pb-4'" style="min-width: 1000px;">

        <div class='row py-1'>
            <div class='col text-left ms-2' :class="!collapsed ? 'mb-1' : ''">
                <div class="row">

                    <div class="col-5 ps-0">
                        <div class="row">
                            <div class="col pe-0 me-0">
                                <button class="btn" @click="toggle_expand()"
                                        style="--bs-btn-padding-y: .25rem; --bs-btn-padding-x: .5rem; --bs-btn-font-size: .75rem;">
                                    <svg width="20" height="20" style="transition:0.1s"
                                         :transform="rotation_matrix(0, 0, collapsed ? 0 : 90)">
                                        <use href="#little-arrow"></use>
                                    </svg>
                                </button>
                                <strong :title="device.serial ? `Serial ${device.serial}` : null">{{ device.name }}</strong>
                                <span v-if="device.demo" class="badge bg-warning text-dark ms-2 rounded-0">DEMO</span>
                                <span class="d-inline-block" style="width:1em"></span>
                                <input class="bg-secondary-subtle text-secondary rounded-0 focus-ring ps-2 bright-focus-input"
                                       type="text" @blur="save_name()" v-model="device_label"
                                       @keypress='(e) => { e.key === "Enter" && e.currentTarget.blur(); }'>
                            </div>
                        </div>
                    </div>

                    <div class="col-5">
                        <div class='btn-group rounded-0' role="group" v-if='device.active'>
                            <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle'
                                    @click='midi.flash_device_led(device.device_index)'>
                                Flash LED
                            </button>
                            <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle'
                                    @click='read_from_device()'>
                                Read from device
                            </button>
                            <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle'
                                    :class="{ 'store-nag': config_changed }"
                                    :disabled="!loaded || saving" @click='store_config()'>
                                {{ saving ? 'Storing...' : 'Store to device' }}
                            </button>
                            <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle'
                                    @click='firmwareModal = true'>
                                Firmware
                            </button>
                        </div>
                    </div>

                    <div class="col-2 small pt-1 pe-3" v-if="device.active">
                        <div class="row">
                            <div class="col text-end">
                                Firmware version
                                <span class="text-body-secondary font-monospace me-2">{{ firmware }}</span>
                            </div>
                        </div>
                    </div>

                </div>
            </div>
        </div>

        <div class='row p-1' v-if="!collapsed">

            <!-- Mode -->

            <div class='col-lg-3 mx-3 bg-body border border-secondary rounded'>
                <div class="row pt-2">
                    <div class='col mb-1'>
                        <strong>Mode</strong>
                    </div>
                </div>
                <div class="row">
                    <div class='col-lg pb-3 ps-4'>
                        <div class="form-check" v-for="m in MODES" :key="m.value">
                            <input class="form-check-input" type="radio" :value="m.value"
                                   :id="`mode-${props.device.device_index}-${m.value}`" v-model="mode">
                            <label class="form-check-label user-select-none"
                                   :for="`mode-${props.device.device_index}-${m.value}`">
                                {{ m.label }}
                            </label>
                        </div>
                        <div class="text-body-secondary small mt-3">{{ mode_hint }}</div>
                        <div class="text-body-secondary small mt-2" v-if="!loaded">
                            Waiting for the device to send its config...
                        </div>

                        <div v-if="loaded && mode === 2" class="mt-3">
                            <div class="small text-body-secondary">
                                Thresholds
                                <span class="font-monospace ms-2">{{ sw_low }} .. {{ sw_high }}</span>
                                <span class="ms-2">on {{ sw_on_ring ? 'ring' : 'sleeve' }}</span>
                            </div>
                            <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle mt-2'
                                    v-if="!sw_calibrating" :disabled="!device.active || !has_schmitt"
                                    @click='start_sw_calibration()'>
                                Calibrate
                            </button>
                        </div>

                        <div v-if="loaded && mode === 1" class="mt-3">
                            <div class="small text-body-secondary">
                                Range
                                <span class="font-monospace ms-2">{{ in_min }} .. {{ in_max }}</span>
                                <span class="ms-2">on {{ wiper_on_ring ? 'ring' : 'tip' }}</span>
                            </div>
                            <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle mt-2'
                                    v-if="!calibrating" :disabled="!device.active"
                                    @click='start_calibration()'>
                                Calibrate
                            </button>
                        </div>
                    </div>
                </div>
            </div>

            <!-- MIDI -->

            <div class='col-lg-2 me-3 bg-body border border-secondary rounded'
                 v-if="loaded && mode !== 0">
                <div class="row pt-2">
                    <div class='col mb-1'>
                        <strong>MIDI</strong>
                    </div>
                </div>
                <div class="row">
                    <div class='col-lg pb-2'>

                        <div class="row ps-1">
                            <div class="col">
                                <div class="form-check">
                                    <label class="form-check-label user-select-none"
                                           :for="`ext-${props.device.device_index}`">
                                        Extended CC
                                    </label>
                                    <input class="form-check-input pull-left" type="checkbox"
                                           :id="`ext-${props.device.device_index}`"
                                           v-model="extended"
                                           @change="extended_changed">
                                </div>
                            </div>
                        </div>

                        <div class="row">
                            <div class='col'>
                                <CCDropDown v-model="cc_msb">
                                    {{ extended ? 'MSB' : 'CC' }}
                                </CCDropDown>
                            </div>
                        </div>

                        <div class="row">
                            <div class='col' :class="{ hide: !extended }">
                                <CCDropDown v-model="cc_lsb">
                                    LSB
                                </CCDropDown>
                            </div>
                        </div>

                        <div class="row">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" min="0" max="15"
                                           v-model.number="channel">
                                    <span class="input-group-text user-select-none">Chan</span>
                                </div>
                            </div>
                        </div>

                    </div>
                </div>
            </div>

            <!-- Output range -->

            <div class='col-lg-3 me-3 bg-body border border-secondary rounded'
                 v-if="loaded && mode !== 0">
                <div class="row pt-2">
                    <div class='col mb-1'>
                        <strong>Output</strong>
                        <span class="text-body-secondary small ms-2">
                            0 &ndash; {{ out_domain_max }}
                        </span>
                    </div>
                </div>
                <div class="row">
                    <div class='col-lg pb-3'>

                        <RangePair v-if="mode === 1"
                                   v-model:low="out_min" v-model:high="out_max"
                                   :max="out_domain_max" ordered
                                   low-label="Heel" high-label="Toe" />

                        <RangePair v-if="mode === 2"
                                   v-model:low="sw_value_off" v-model:high="sw_value_on"
                                   :max="out_domain_max"
                                   low-label="Off" high-label="On" />

                        <div class="text-body-secondary small mt-2" v-if="mode === 1">
                            The ends of the pedal's travel map to these values.
                        </div>

                        <template v-if="mode === 1 && has_deadzone">
                            <div class="mt-3 mb-1">
                                <strong class="small">Deadzone</strong>
                                <span class="text-body-secondary small ms-2">
                                    {{ dz_live }}% live travel
                                </span>
                            </div>
                            <ValueSlider v-model="dz_heel" :min="0" :max="dz_heel_max"
                                         label="Heel" suffix="%" />
                            <div class="mt-1">
                                <ValueSlider v-model="dz_toe" :min="0" :max="dz_toe_max"
                                             label="Toe" suffix="%" />
                            </div>
                            <div class="text-body-secondary small mt-2">
                                Travel inside a deadzone holds the output at that end's
                                value. The rest of the sweep still covers the whole
                                output range.
                            </div>
                        </template>
                        <div class="text-body-secondary small mt-2" v-if="mode === 2">
                            The value sent when the switch is open and closed.
                        </div>

                        <template v-if="mode === 2 && has_schmitt">
                            <div class="mt-3 mb-1">
                                <strong class="small">Switching thresholds</strong>
                                <span class="text-body-secondary small ms-2">
                                    raw, 0 &ndash; 4095
                                </span>
                            </div>
                            <RangePair v-model:low="sw_low" v-model:high="sw_high"
                                       :max="4095" ordered
                                       low-label="Open" high-label="Close" />
                            <div class="text-body-secondary small mt-2">
                                Reads above <span class="font-monospace">{{ sw_high }}</span>
                                count as pressed; it stays pressed until the reading falls
                                below <span class="font-monospace">{{ sw_low }}</span>.
                                The gap between them is what stops a switch resting near
                                one edge from chattering.
                            </div>
                        </template>

                        <div class="form-check mt-3" v-if="mode === 2" :title="toggle_hint">
                            <label class="form-check-label user-select-none"
                                   :for="`tog-${props.device.device_index}`">
                                Toggle mode
                            </label>
                            <input class="form-check-input pull-left" type="checkbox"
                                   :id="`tog-${props.device.device_index}`"
                                   v-model="sw_toggle">
                        </div>

                        <div class="mt-3" v-if="mode === 1 && has_curve" :title="curve_hint">
                            <strong class="small">Response</strong>
                            <div class="d-flex align-items-center gap-2 mt-1">
                                <select class="form-select form-select-sm" v-model="log_curve">
                                    <option :value="false">Linear (even)</option>
                                    <option :value="true">Log (audio taper)</option>
                                </select>
                                <svg :width="CURVE_W" :height="CURVE_H" class="flex-shrink-0 border border-secondary-subtle"
                                     :viewBox="`0 0 ${CURVE_W} ${CURVE_H}`" role="img"
                                     aria-label="response curve">
                                    <!-- linear reference, so the bend is visible as a
                                         departure from something rather than in isolation -->
                                    <path :d="`M0,${CURVE_H} L${CURVE_W},0`" fill="none"
                                          stroke="currentColor" stroke-opacity="0.25"
                                          stroke-width="1" stroke-dasharray="2 2" />
                                    <path :d="curve_path" fill="none" stroke="currentColor"
                                          stroke-width="1.5" />
                                </svg>
                            </div>
                        </div>

                        <div class="form-check mt-3" :title="invert_hint">
                            <label class="form-check-label user-select-none"
                                   :for="`inv-${props.device.device_index}`">
                                Invert
                            </label>
                            <input class="form-check-input pull-left" type="checkbox"
                                   :id="`inv-${props.device.device_index}`"
                                   v-model="invert">
                        </div>

                    </div>
                </div>
            </div>

            <!-- Calibration -->

            <div class='col-lg bg-body border border-secondary rounded me-3' v-if="calibrating">
                <div class="row pt-2">
                    <div class='col mb-1'>
                        <strong>Calibrating</strong>
                        <span class="text-body-secondary small ms-3">
                            sweep the pedal all the way heel to toe, then back
                        </span>
                    </div>
                </div>
                <div class="row">
                    <div class="col pb-3">

                        <div v-for="leg in [
                                { key: 'tip',  label: 'TIP ' },
                                { key: 'ring', label: 'RING' }]" :key="leg.key" class="mb-3">
                            <div class="d-flex small font-monospace">
                                <span>{{ leg.label }}</span>
                                <span class="ms-3">{{ cal[leg.key] }}</span>
                                <span class="ms-auto me-2">
                                    travel {{ cal_travel[leg.key] }}
                                    <span v-if="cal_travel[leg.key] >= 200"
                                          class="ms-2 badge bg-success rounded-0">wiper</span>
                                </span>
                            </div>
                            <div class="progress border bg-body border-secondary rounded-0" style="height:14px">
                                <div class="progress-bar value-bar"
                                     :style="`width:${cal[leg.key] * 100 / 4095}%`"></div>
                            </div>
                            <div class="small text-body-secondary font-monospace">
                                min {{ cal[leg.key + '_max'] >= cal[leg.key + '_min'] ? cal[leg.key + '_min'] : 0 }}
                                &nbsp; max {{ cal[leg.key + '_max'] }}
                            </div>
                        </div>

                        <div class="small text-body-secondary mb-2" v-if="!cal_enough">
                            Not enough movement yet. Whichever conductor moves is the
                            wiper &mdash; that is how the wiring gets decided.
                        </div>

                        <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle'
                                :disabled="!cal_enough" @click='accept_calibration()'>
                            Use this range
                        </button>
                        <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle ms-2'
                                @click='cancel_calibration()'>
                            Cancel
                        </button>
                    </div>
                </div>
            </div>

            <!-- Footswitch calibration -->

            <div class='col-lg bg-body border border-secondary rounded me-3' v-if="sw_calibrating">
                <div class="row pt-2">
                    <div class='col mb-1'>
                        <strong>Calibrating</strong>
                        <span class="text-body-secondary small ms-3">
                            press and release the footswitch a few times
                        </span>
                    </div>
                </div>
                <div class="row">
                    <div class="col pb-3">

                        <div v-for="leg in [
                                { key: 'diff', label: 'TIP-RING   ' },
                                { key: 'pull', label: 'TIP-SLEEVE ' }]" :key="leg.key" class="mb-3">
                            <div class="d-flex small font-monospace">
                                <span>{{ leg.label }}</span>
                                <span class="ms-3">{{ sw_cal[leg.key] }}</span>
                                <span class="ms-auto me-2">
                                    travel {{ sw_cal_travel[leg.key] }}
                                    <span v-if="sw_cal_enough &&
                                                (leg.key === 'diff') === sw_cal_on_ring"
                                          class="ms-2 badge bg-success rounded-0">wiring</span>
                                </span>
                            </div>
                            <div class="progress border bg-body border-secondary rounded-0" style="height:14px">
                                <div class="progress-bar value-bar"
                                     :style="`width:${sw_cal[leg.key] * 100 / 4095}%`"></div>
                            </div>
                            <div class="small text-body-secondary font-monospace">
                                min {{ sw_cal[leg.key + '_max'] >= sw_cal[leg.key + '_min']
                                       ? sw_cal[leg.key + '_min'] : 0 }}
                                &nbsp; max {{ sw_cal[leg.key + '_max'] }}
                            </div>
                        </div>

                        <div class="small text-body-secondary mb-2" v-if="!sw_cal.seen_pullup">
                            This firmware does not report the tip-sleeve reading, so only a
                            tip-ring switch can be calibrated. Update the firmware for the
                            other wiring.
                        </div>
                        <div class="small text-body-secondary mb-2" v-else-if="!sw_cal_enough">
                            Not enough movement yet. Whichever row moves is the wiring you
                            have &mdash; that is how it gets decided.
                        </div>

                        <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle'
                                :disabled="!sw_cal_enough" @click='accept_sw_calibration()'>
                            Use these thresholds
                        </button>
                        <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle ms-2'
                                @click='cancel_sw_calibration()'>
                            Cancel
                        </button>
                    </div>
                </div>
            </div>

            <!-- MIDI in -->

            <div class='col-lg bg-body border border-secondary rounded me-3'
                 style="min-width: 0"
                 v-if="!calibrating && !sw_calibrating">
                <div class="row pt-2">
                    <div class='col mb-1 d-flex align-items-center gap-3'>
                        <strong class="text-nowrap">MIDI in</strong>
                        <div class="form-check mb-0 text-nowrap">
                            <input class="form-check-input" type="checkbox"
                                   :id="`rt-${props.device.device_index}`" v-model="hide_realtime">
                            <label class="form-check-label user-select-none small"
                                   :for="`rt-${props.device.device_index}`">
                                hide realtime<span v-if="hide_realtime && counts.rt"> ({{ counts.rt }})</span>
                            </label>
                        </div>
                        <button class='btn btn-sm rounded-0 tertiary-bg border border-secondary-subtle ms-auto me-2'
                                @click='clear_monitor()'>
                            Clear
                        </button>
                    </div>
                </div>

                <!-- Counts on their own line: they grow as they count, and
                     sharing a row with the title made the whole header wrap. -->
                <div class="row">
                    <div class="col">
                        <div class="text-body-secondary small font-monospace mb-1">
                            {{ counts.total }} messages &mdash;
                            {{ counts.note }} note, {{ counts.cc }} cc,
                            {{ counts.rt }} realtime, {{ counts.other }} other
                        </div>
                    </div>
                </div>
                <div class="row pb-3">
                    <div class="col">
                        <div ref="monitor_el"
                             class="border border-secondary bg-body-tertiary rounded-0 p-2 font-monospace small"
                             style="height: 240px; overflow-y: auto; overflow-x: auto; white-space: pre;">
                            <div v-for="m in monitor" :key="m.seq">{{ m.hex.padEnd(11) }}  {{ m.text }}</div>
                            <div v-if="monitor.length === 0" class="text-body-secondary">
                                nothing yet &mdash; play something into the TRS jack
                            </div>
                        </div>
                    </div>
                </div>
            </div>

        </div>
    </div>

    <!-- Firmware update help -->

    <Modal v-model="firmwareModal" maxwidth="42rem" closeable header="Firmware update">
        <div class="p-3">

            <p class="small">
                This adapter is running firmware
                <strong class="font-monospace">{{ firmware }}</strong>.
            </p>

            <p class="small">
                <a :href="FIRMWARE_URL" target="_blank" rel="noreferrer noopener">
                    Download the latest firmware</a>
                &mdash; a file ending in <span class="font-monospace">.bin</span>. Save it
                somewhere you can find again, such as your Downloads folder.
            </p>

            <!-- Comes before the flashing-mode box because it is a hard
                 prerequisite for it: the button is on the PCB, so a boxed unit
                 cannot be put into flashing mode at all. -->
            <div class="border border-secondary rounded p-2 mb-3">
                <strong class="small">Opening the case</strong>
                <p class="small mb-1 mt-1">
                    The button is inside the enclosure, so the adapter has to come apart
                    first. Unplug it, then:
                </p>
                <ol class="small mb-1">
                    <li>Unscrew and remove the silver 6.35&nbsp;mm jack insert.</li>
                    <li>Prise off the back cover (the USB end) with a spudger or thin blade.</li>
                    <li>Carefully remove the PCB.</li>
                </ol>
                <p class="small mb-0 text-body-secondary">
                    Reassemble in reverse. Work gently round the edge of the cover rather
                    than forcing one spot.
                </p>
            </div>

            <!-- The one step people get wrong, so it is out on its own and above
                 the platform split rather than repeated inside each one. -->
            <div class="border border-secondary rounded p-2 mb-3">
                <strong class="small">Getting into flashing mode</strong>
                <p class="small mb-1 mt-1">
                    <strong>Hold the button down, plug in the USB cable, then let go.</strong>
                    The button has to be held before the cable goes in &mdash; pressing it
                    afterwards does nothing.
                </p>
                <p class="small mb-0 text-body-secondary">
                    In flashing mode the adapter will not appear as a MIDI device and this
                    page will not see it. That is normal, and it is how you know it worked.
                </p>
            </div>

            <div class="btn-group btn-group-sm mb-3" role="group">
                <button class="btn rounded-0 tertiary-bg border border-secondary-subtle"
                        :class="{ active: !show_mac }" @click="show_mac = false">Windows</button>
                <button class="btn rounded-0 tertiary-bg border border-secondary-subtle"
                        :class="{ active: show_mac }" @click="show_mac = true">Mac</button>
            </div>

            <!-- Windows -->
            <div v-if="!show_mac" class="small">
                <p>
                    You need <strong>WCHISPTool</strong>, free from the chip maker
                    (<a href="https://www.wch-ic.com/downloads/WCHISPTool_Setup_exe.html"
                        target="_blank" rel="noreferrer noopener">download</a>). Installing it
                    also sets up the USB driver it needs. This is a one-off.
                </p>
                <ol class="mb-2">
                    <li>In WCHISPTool set <strong>Chip Series</strong> to
                        <span class="font-monospace">CH32V20x</span>,
                        <strong>Chip Model</strong> to
                        <span class="font-monospace">CH32V203G6U6</span>, and
                        <strong>Download Method</strong> to <span class="font-monospace">USB</span>.</li>
                    <li>Get the adapter into flashing mode, as above.</li>
                    <li>It should appear in the tool&rsquo;s USB device list.</li>
                    <li>Browse to your <span class="font-monospace">.bin</span> file and click
                        <strong>Download</strong>.</li>
                    <li>Unplug and plug back in, without the button.</li>
                </ol>
                <p class="text-body-secondary">
                    Leave every other setting alone &mdash; especially anything about read
                    protection.
                </p>
            </div>

            <!-- Mac -->
            <div v-else class="small">
                <p>
                    You need <strong>wchisp</strong>, free and open source
                    (<a href="https://github.com/ch32-rs/wchisp/releases"
                        target="_blank" rel="noreferrer noopener">download</a>). Setting it up
                    the first time takes a few minutes and is covered in the full
                    instructions.
                </p>
                <ol class="mb-2">
                    <li>Get the adapter into flashing mode, as above.</li>
                    <li>In Terminal, run these three commands from the folder holding
                        <span class="font-monospace">wchisp</span> and your
                        <span class="font-monospace">.bin</span> file:</li>
                </ol>
<pre class="border border-secondary rounded p-2 mb-2 font-monospace small">./wchisp erase
./wchisp flash -E -R midi_adapter_0.3.0.0.bin
./wchisp reset</pre>
                <p class="text-danger-emphasis">
                    <strong>Do not use <span class="font-monospace">wchisp flash</span> on its
                    own.</strong> wchisp&rsquo;s own documentation suggests it, but on this chip
                    it works once and then locks itself out. Always erase first and pass
                    <span class="font-monospace">-E</span>.
                </p>
            </div>

            <hr>

            <p class="small mb-1">
                <a :href="FLASHING_URL" target="_blank" rel="noreferrer noopener">
                    Full instructions, including troubleshooting</a>
                &mdash; start here if anything does not go as described.
            </p>
            <p class="small text-body-secondary mb-0">
                Your settings are kept across a firmware update.
            </p>
        </div>
    </Modal>

</template>
