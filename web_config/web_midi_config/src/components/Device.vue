<script setup>

import { toRaw, watch, ref, computed } from 'vue';

import Toggle from './Toggle.vue'
import Modal from './Modal.vue'
import CCDropDown from './CCDropDown.vue';

import midi from '../Midi.js'

import fileDownload from 'js-file-download';

const props = defineProps({
    device: {
        type: Object,
        required: true
    }
});

// 3 modal dialogs
const flashModal = ref(false);
const importModal = ref(false);
const disconnectModal = ref(false);

// is the current config different from what's on the device?
let config_changed = ref(false);

// this is the config on the device to compare against
let stored_config = null;

// noddy compare for config objects

function shallowEqual(object1, object2) {

    const keys1 = Object.keys(object1);
    const keys2 = Object.keys(object2);

    if (keys1.length !== keys2.length) {
        return false;
    }

    for (let key of keys1) {
        if (key != 'value' && object1[key] != object2[key]) {
            return false;
        }
    }
    return true;
}

function make_flag(flag) {

    return computed({
        get() {
            return (props.device.config.flags & flag) != 0;
        },
        set(val) {
            props.device.config.flags = (props.device.config.flags & ~flag) | (val ? flag : 0);
        }
    });

}

const rotate_extended = make_flag(midi.flags.cf_rotate_extended);
const rotate_relative = make_flag(midi.flags.cf_rotate_relative);
const led_invert = make_flag(midi.flags.cf_led_invert);
const led_flash_on_rot = make_flag(midi.flags.cf_led_flash_on_rot);
const led_flash_on_limit = make_flag(midi.flags.cf_led_flash_on_limit);
const btn_momentary = make_flag(midi.flags.cf_btn_momentary);
const btn_extended = make_flag(midi.flags.cf_btn_extended);
const led_flash_on_click = make_flag(midi.flags.cf_led_flash_on_click);
const led_flash_on_release = make_flag(midi.flags.cf_led_flash_on_release);
const led_track_button_toggle = make_flag(midi.flags.cf_led_track_button_toggle);

function make_channel(bit_offset) {
    return computed({
        get() {
            return (props.device.config.channels >> bit_offset) & 0xf;
        },
        set(val) {
            let mask = ~(0xf << bit_offset);
            props.device.config.channels = (props.device.config.channels & mask) | ((val & 0xf) << bit_offset);
        }
    });
}

const rot_channel = make_channel(0);
const btn_channel = make_channel(4);

const acceleration = computed({
    get() {
        const lsb = (props.device.config.flags & midi.flags.cf_acceleration_lsb) != 0 ? 1 : 0;
        const msb = (props.device.config.flags & midi.flags.cf_acceleration_msb) != 0 ? 2 : 0;
        return lsb | msb;
    },
    set(val) {
        const lsb = (val & 1) != 0 ? midi.flags.cf_acceleration_lsb : 0;
        const msb = (val & 2) != 0 ? midi.flags.cf_acceleration_msb : 0;
        const mask = midi.flags.cf_acceleration_lsb | midi.flags.cf_acceleration_msb;
        props.device.config.flags = (props.device.config.flags & ~mask) | lsb | msb;
    }
});

// sanitize the values and track if anything changed

watch(props.device.config, (o, n) => {

    let c = o.value;

    let max_zero_point = (rotate_extended.value != 0) ? (1 << 14) : (1 << 7);
    let max_rot_value = max_zero_point - 1;

    c.rot_zero_point = Math.max(16, Math.min(c.rot_zero_point, max_zero_point));

    c.rot_delta_14 = Math.max(1, Math.min(c.rot_delta_14, 0x3fff));
    c.btn_value_a_14 = Math.max(0, Math.min(c.btn_value_a_14, 0x3fff));
    c.btn_value_b_14 = Math.max(0, Math.min(c.btn_value_b_14, 0x3fff));

    c.rot_delta_7 = Math.max(1, Math.min(c.rot_delta_7, 0x7f));
    c.btn_value_a_7 = Math.max(0, Math.min(c.btn_value_a_7, 0x7f));
    c.btn_value_b_7 = Math.max(0, Math.min(c.btn_value_b_7, 0x7f));

    n.value = c;

    config_changed.value = stored_config != null && !shallowEqual(toRaw(c), stored_config);
});

// a config was loaded from the device

midi.on_config_loaded((device) => {
    config_changed.value = false;
    stored_config = Object.assign({}, toRaw(device.config));
    props.device.config = device.config;
});

// a config was saved to the device 

midi.on_config_saved((device) => {
    config_changed.value = false;
    stored_config = Object.assign({}, toRaw(device.config));
});

// this doesn't work - port.close() does nothing

function toggleConnection(device) {
    if (!device.active) {
        midi.toggle_device_connection(device.device_index)
        config_changed.value = false;
    } else {
        disconnectModal.value = true;
    }
}

</script>

<template>
    <div class="container border rounded-3 py-3 bg-device border-secondary bg-secondary-subtle">
        <div class='row p-1'>

            <!-- Name, Serial, Buttons -->

            <div class='col-lg-3'>
                <div class='row'>
                    <div class='col-6'>
                        <div class="container">
                            <h5>{{ device.name }}</h5>
                        </div>
                    </div>
                </div>
                <div class='row mt-1'>
                    <div class="col-6 text-center">
                        <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                            @click='toggleConnection(device)'>
                            {{ !device.active ? 'Connect' : 'Disconnect' }}
                        </button>
                        <div class="small mt-3" v-if="device.active">
                            Firmware
                            <span class="text-primary-emphasis font-monospace">
                                v{{ device.firmware_version >> 8 }}.{{ (device.firmware_version &
                                0x7f).toString(10).padStart(2, '0') }}
                            </span>
                        </div>
                        <div class="small" v-if="device.active">
                            Serial #
                            <span class="text-body-secondary font-monospace">{{ device.serial_str }}</span>
                        </div>
                    </div>
                    <div class="col-6">
                        <div class='btn-group-vertical' role="group" v-if='device.active'>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                                @click='midi.flash_device_led(device.device_index)'>
                                <span class="mx-2">Flash LED</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                                @click='midi.read_flash(device.device_index)'>
                                <span class="mx-2">Read from device</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                                :class="{ 'red-text': config_changed }"
                                @click='midi.write_flash(device.device_index); config_changed = false'>
                                <span class="mx-2">Store to device</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                                v-if='device.active || 1'
                                @click="fileDownload(midi.get_config_json(device.device_index), 'midi_knob_settings.json');">
                                <span class="mx-2">Export config</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                                @click="importModal = true">
                                <span class="mx-2">Import config</span>
                            </button>

                            <button class='btn btn-sm tertiary-bg border border-secondary-subtle'
                                @click='flashModal = true'>
                                <span class="mx-2">Advanced</span>
                            </button>

                        </div>
                    </div>
                </div>
            </div>

            <!-- Rotation -->

            <div v-if='device.active' class='col-lg-3 mx-3 bg-body border border-secondary rounded'>
                <div class="row pt-2">
                    <div class='col'>
                        <h5>Rotation</h5>
                    </div>
                </div>
                <div class="row">
                    <div class='col-lg'>
                        <div class="row px-2">
                            <div class="col">
                                <div class="form-check">
                                    <label class="form-check-label user-select-none" for="extended_check_rot">Extended
                                        CC</label>
                                    <input class="form-check-input pull-left" type="checkbox" id="extended_check_rot"
                                        v-model="rotate_extended">
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class='col'>
                                <CCDropDown v-model="device.config.rot_control_msb" :label="rotate_extended ?
                                'MSB' : 'CC'
                                " />
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class='col'>
                                <CCDropDown v-model="device.config.rot_control_lsb" label="LSB"
                                    :hidden="!rotate_extended" />
                            </div>
                        </div>
                    </div>
                    <div class="col-lg">
                        <div class="row">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="rot_channel">
                                    <span class="input-group-text user-select-none">Chan</span>
                                </div>
                            </div>
                        </div>
                        <div class="row">
                            <div class="col" v-if="rotate_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.rot_delta_14">
                                    <span class="input-group-text user-select-none">Delta</span>
                                </div>
                            </div>
                            <div class="col" v-if="!rotate_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.rot_delta_7">
                                    <span class="input-group-text user-select-none">Delta</span>
                                </div>
                            </div>
                        </div>
                        <div class="row">
                            <div class="col">
                                <div class="input-group">
                                    <select class="form-select smaller-text" id="inputGroupSelect01"
                                        v-model="acceleration">
                                        <option selected value="0">Off</option>
                                        <option value="1">Low</option>
                                        <option value="2">Medium</option>
                                        <option value="3">High</option>
                                    </select>
                                    <label class="input-group-text" for="inputGroupSelect01">Accel</label>
                                </div>
                            </div>
                        </div>
                        <div class="row mx-4 mt-1 mb-0">
                            <div class="col" style="height:1.5rem;margin: 4px;padding: 0px;">
                                <Toggle v-model="rotate_relative" :checked-text-color='"var(--bs-btn-color)"'
                                    :unchecked-text-color='"var(--bs-btn-color)"'
                                    :checked-background-color='"var(--bs-tertiary-bg)"'
                                    :unchecked-background-color='"var(--bs-tertiary-bg)"'
                                    :border-color='"var(--bs-border-color)"'
                                    :unchecked-pill-color='"var(--bs-border-color)"'
                                    :checked-pill-color='"var(--bs-border-color)"' :checked-text='"Relative"'
                                    :unchecked-text='"Absolute"' />
                            </div>
                        </div>
                        <div class="row" :class="{ 'hide': !rotate_relative }">
                            <div class='col'>
                                <div class="input-group mb-2">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.rot_zero_point">
                                    <span class="input-group-text user-select-none">Zero</span>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Button -->

            <div v-if='device.active' class='col-lg-3 mx-3 bg-body border border-secondary rounded'>
                <div class="row pt-2">
                    <div class="col">
                        <h5>Button</h5>
                    </div>
                </div>
                <div class="row">
                    <div class="col-lg">
                        <div class="row p-1">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="btn_channel">
                                    <span class="input-group-text user-select-none">Chan</span>
                                </div>
                            </div>
                        </div>
                        <div class="row mx-4 mt-2 mb-1">
                            <div class="col" style="height:1.5rem;margin: 0px;padding: 0px;">
                                <Toggle v-model="btn_momentary" :checked-text-color='"var(--bs-btn-color)"'
                                    :unchecked-text-color='"var(--bs-btn-color)"'
                                    :checked-background-color='"var(--bs-tertiary-bg)"'
                                    :unchecked-background-color='"var(--bs-tertiary-bg)"'
                                    :border-color='"var(--bs-border-color)"'
                                    :unchecked-pill-color='"var(--bs-border-color)"'
                                    :checked-pill-color='"var(--bs-border-color)"' :checked-text='"Momentary"'
                                    :unchecked-text='"Toggle"' />
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col" v-if="btn_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.btn_value_a_14">
                                    <span class="input-group-text user-select-none">
                                        {{ btn_momentary ? "OFF?" : "A?" }}
                                    </span>
                                </div>
                            </div>
                            <div class="col" v-if="!btn_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.btn_value_a_7">
                                    <span class="input-group-text user-select-none">
                                        {{ btn_momentary ? "OFF" : "A" }}
                                    </span>
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col" v-if="btn_extended">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.btn_value_b_14">
                                    <span class="input-group-text user-select-none">
                                        {{ btn_momentary ? "OFF!" : "B!" }}
                                    </span>
                                </div>
                            </div>
                            <div class="col" v-if="!btn_extended">
                                <div class="input-group mb-2">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.btn_value_b_7">
                                    <span class="input-group-text user-select-none">
                                        {{ btn_momentary ? "OFF" : "B" }}
                                    </span>
                                </div>
                            </div>
                        </div>
                    </div>
                    <div class="col-lg">
                        <div class="row px-2">
                            <div class="col">
                                <div class="form-check">
                                    <label class="form-check-label user-select-none" for="extended_check_btn">Extended
                                        CC</label>
                                    <input class="form-check-input pull-left" type="checkbox" id="extended_check_btn"
                                        v-model="btn_extended">
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                <CCDropDown v-model="device.config.btn_control_msb"
                                    :label="btn_extended ? 'MSB' : 'CC'" />
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col mb-2">
                                <CCDropDown v-model="device.config.btn_control_lsb" label="LSB"
                                    :hidden="!btn_extended" />
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- LED -->

            <div v-if='device.active' class='col-lg-2 mx-3 bg-body border border-secondary rounded'>
                <div class="row pt-2">
                    <div class="col">
                        <h5>LED</h5>
                    </div>
                </div>
                <div class="row">
                    <div class="col">
                        <div class="row">
                            <div class="col">
                                Flash LED when:
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_rot">Knob is
                                    rotated</label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_rot"
                                    v-model="led_flash_on_rot">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_limit">Rotation value
                                    hits
                                    limit</label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_limit"
                                    v-model="led_flash_on_limit">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_click">Button is
                                    pressed</label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_click"
                                    v-model="led_flash_on_click">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_release">Button is
                                    released</label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_release"
                                    v-model="led_flash_on_release">
                            </div>
                        </div>
                    </div>
                    <hr class="my-1 mt-1" />
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="track_button_value">LED tracks
                                    button
                                    value</label>
                                <input class="form-check-input pull-left" type="checkbox" id="track_button_value"
                                    v-model="led_track_button_toggle">
                            </div>
                        </div>
                    </div>
                    <div class="row mb-2">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="led_invert">Invert
                                    LED</label>
                                <input class="form-check-input pull-left" type="checkbox" id="led_invert"
                                    v-model="led_invert">
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <!-- Modals -->

    <Modal v-model="flashModal" maxwidth="25%" closeable header="Advanced Functions">
        <div class="row mx-2 my-1">
            <div class="col">
                <p class="text-center red-text">Warning! Only click this if you
                    know what you're doing</p>
                <p class="text-center">
                    Instructions for firmware updating are
                    <a href='https://skilbeck.com/tiny-usb-midi-knob' target="_blank" rel="noreferrer noopener">
                        available here</a>.
                </p>
                <p class="text-center">
                    To get back to normal mode, unplug and plug it back in.
                </p>
            </div>
        </div>
        <div class="row mx-2 my-2">
            <div class="col text-center">
                <button class='btn btn-sm btn-danger' @click='midi.flash_mode(device.device_index); flashModal = false'>
                    Put device in Firmware Update Mode
                </button>
            </div>
        </div>
    </Modal>

    <Modal v-model="importModal" maxwidth="20%" closeable header="Import Settings">
        <div class="row mx-2 my-2">
            <div class="col mb-2 text-center">
                <input class="btn btn-sm border" type="file" id="import_settings" />
            </div>
        </div>
        <div class="row mx-2 my-3">
            <div class="col text-center">
                <button class="btn btn-sm btn-primary">
                    Import
                </button>
            </div>
        </div>
        <div class="row mx-2 my-3">
            <div class="col text-center">
                This is not yet implemented, sorry
            </div>
        </div>
    </Modal>

    <Modal v-model="disconnectModal" maxwidth="25%" closeable header="Disconnect">
        <div class="row mx-2">
            <div class="col text-center">
                Sorry, I can't seem to make disconnect work. You have to close the tab to disconnect all devices.
            </div>
        </div>
        <div class="row mt-4 mb-2">
            <div class="col text-center">
                <button class="btn btn-primary btn-sm" @click="disconnectModal = false">
                    Bummer
                </button>
            </div>
        </div>
    </Modal>
</template>

<style>
.hide {
    visibility: hidden;
}

.smaller-text {
    font-size: smaller;
}


[data-bs-theme='light'] .red-text {
    color: #C06000;
    font-weight: 600;
}

[data-bs-theme='dark'] .red-text {
    color: orange;
    font-weight: 600;
}

.slim-button {
    padding-top: 0px !important;
    padding-bottom: 2px !important;
}

.bg-device {
    background-color: var(--bs-body-bg);
}
</style>
