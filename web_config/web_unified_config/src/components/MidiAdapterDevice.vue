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

import midi, { ADAPTER_MODE } from '../midi/Midi.js'
import CC from '../midi/CC.js'
import CCDropDown from './CCDropDown.vue'

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

const cc_msb = ref(11);
const cc_lsb = ref(43);
const channel = ref(0);
const extended = ref(false);

const AF_WIPER_ON_RING = 0x0001;
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
    if (!event.target.checked) return;
    const cc = CC.CCs[cc_msb.value];
    if (CC.is_MSB(cc)) {
        cc_lsb.value = cc.alt;
    }
}

const mode_hint = computed(
    () => MODES.find(m => m.value === mode.value)?.hint ?? '');

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
           extended.value !== !!(c.flags & AF_14BIT);
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

    let f = c.flags;
    f = wiper_on_ring.value ? (f | AF_WIPER_ON_RING) : (f & ~AF_WIPER_ON_RING);
    f = extended.value ? (f | AF_14BIT) : (f & ~AF_14BIT);
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
    props.device.on_probe = on_probe;
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
                                    :class="{ 'red-text': config_changed }"
                                    :disabled="!loaded || saving" @click='store_config()'>
                                {{ saving ? 'Storing...' : 'Store to device' }}
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

            <div class='col-lg-3 me-3 bg-body border border-secondary rounded'
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

            <!-- MIDI in -->

            <div class='col-lg bg-body border border-secondary rounded me-3' v-if="!calibrating">
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
                             style="height: 240px; overflow-y: auto; white-space: pre;">
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

</template>
