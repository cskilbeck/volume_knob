<script setup>

import { watch, getCurrentInstance, ref } from 'vue';
import Toggle from './Toggle.vue'
import midi from '../Midi.js'
import Modal from './Modal.vue'
import CCDropDown from './CCDropDown.vue';
import fileDownload from 'js-file-download';

const props = defineProps({
    device: {
        type: Object,
        required: true
    }
});

const flashModal = ref(false);
const importModal = ref(false);

// sanitize the values

watch(props.device.config, (o, n) => {

    let c = o;

    let max_zero_point = c.cf_rotate_extended ? (1 << 14) : (1 << 7);
    let max_high_value = max_zero_point - 1;
    let btn_limit = c.cf_btn_extended ? (1 << 14) : (1 << 7);

    c.rot_zero_point = Math.max(16, Math.min(c.rot_zero_point, max_zero_point));

    c.rot_delta = Math.max(1, Math.min(c.rot_delta, c.rot_zero_point));

    c.rot_limit_high = Math.max(1, Math.min(c.rot_limit_high, max_high_value));
    c.rot_limit_low = Math.max(0, Math.min(c.rot_limit_low, c.rot_limit_high - 1));

    c.btn_value_1 = Math.max(0, Math.min(c.btn_value_1, btn_limit - 1));
    c.btn_value_2 = Math.max(0, Math.min(c.btn_value_2, btn_limit - 1));

    c.rot_channel = Math.max(0, Math.min(c.rot_channel, 15));
    c.btn_channel = Math.max(0, Math.min(c.btn_channel, 15));

    n.value = c;
});

const instance = getCurrentInstance();

function force_update() {
    instance?.proxy?.$forceUpdate();
}

midi.on_config_changed((device) => {
    props.device.config = device.config;
    force_update();
});

function toggler_style() {
    let btn = document.getElementById("canonical-button")
    if (btn) {
        return window.getComputedStyle(btn);
    }
}

function import_settings() {
    // use fetch to send the file to some noddy thing on the server which just sends it back
    // do some prompt when it comes back in case it takes a while
}

</script>

<template>
    <button class="btn" id="canonical-button">foo</button>
    <div class="container border rounded-3 py-3">
        <div class='row p-2 pb-3'>

            <!-- Name, Serial, Buttons -->

            <div class='col-lg-3  text-center'>
                <div class='row'>
                    <div class='col'>
                        <div class="container">
                            <h5>{{ device.name }}</h5>
                        </div>
                    </div>
                </div>
                <div class='row'>
                    <div class='col'>
                        <div class="container small">
                            Firmware
                            <span class="text-primary-emphasis font-monospace">v{{ device.firmware_version >> 8 }}.{{
                                (device.firmware_version & 0x7f).toString(10).padStart(2, '0') }}</span>
                        </div>
                    </div>
                </div>
                <div class='row'>
                    <div class='col'>
                        <div class="container small">
                            Serial #
                            <span class="text-body-secondary font-monospace">{{ device.serial_str }}</span>
                        </div>
                    </div>
                </div>
                <div class='row mt-1 mx-5'>
                    <div class="col">
                        <div class='btn-group-vertical' role="group">
                            <button class='btn btn-sm btn-outline-secondary text-emphasis'
                                v-on:click='midi.flash_device_led(device.device_index)'>Flash
                                LED</button>
                            <button class='btn btn-sm btn-outline-secondary text-emphasis'
                                v-on:click='midi.read_flash(device.device_index)'>Load</button>
                            <button class='btn btn-sm btn-outline-secondary text-emphasis'
                                v-on:click='midi.write_flash(device.device_index)'>Save</button>
                            <button class='btn btn-sm btn-outline-secondary text-emphasis'
                                @click="fileDownload(midi.get_config_json(device.device_index), 'midi_knob_settings.json');">Export</button>
                            <button class='btn btn-sm btn-outline-secondary text-emphasis'
                                @click="importModal = true">Import</button>
                            <Modal v-model="importModal" maxwidth="20%" closeable header="Import Settings">
                                <div class="row mx-2 my-2">
                                    <div class="col mb-2">
                                        <input type="file" id="import_settings" />
                                    </div>
                                </div>
                                <div class="row mx-2 my-3">
                                    <div class="col text-center">
                                        <button class="btn btn-sm btn-primary">Import</button>
                                    </div>
                                </div>
                                <div class="row mx-2 my-3">
                                    <div class="col text-center">
                                        This is not yet implemented, sorry
                                    </div>
                                </div>
                            </Modal>
                            <button class='btn btn-sm btn-outline-secondary text-emphasis'
                                @click='flashModal = true'>Advanced</button>
                            <Modal v-model="flashModal" maxwidth="20%" closeable header="Advanced Functions">
                                <div class="row mx-2 my-1">
                                    <div class="col mb-1">
                                        <p class="text-center text-warning">Warning! Only mess with this if you're
                                            quite sure you know what you're doing...</p>
                                        <p>Instructions for performing the firmware update are available <a
                                                href='https://skilbeck.com' target="_blank"
                                                rel="noreferrer noopener">here.</a>
                                            To get back to normal mode you can unplug the device and plug it back in.
                                        </p>
                                        <p></p>
                                    </div>
                                </div>
                                <div class="row mx-2 my-4">
                                    <div class="col text-center">
                                        <button class='btn btn-sm btn-danger'
                                            v-on:click='midi.flash_mode(device.device_index)'>Put device in Firmware
                                            Update Mode</button>
                                    </div>
                                </div>
                            </Modal>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Rotation -->

            <div class='col-lg-3 mx-3'>
                <div class="row">
                    <div class='col'>
                        <h5>Rotation</h5>
                    </div>
                </div>
                <div class="row pt-2 border rounded-3">
                    <div class="col-lg">
                        <div class="row p-1">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.rot_channel">
                                    <span class="input-group-text user-select-none">Chan</span>
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="device.config.rot_delta">
                                    <span class="input-group-text user-select-none">Delta</span>
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                <div class="mt-1 mb-1">
                                    <Toggle v-model="device.config.cf_rotate_relative" :pixelsHigh="26" :dotSize="0.7"
                                        :rounded="1" :pixelsWide="110">
                                        <template #checked-text>Relative</template>

                                        <template #unchecked-text>Absolute</template>
                                    </Toggle>
                                </div>
                            </div>
                        </div>
                        <div class="row p-1" v-show="device.config.cf_rotate_relative">
                            <div class='col'>
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.rot_zero_point">
                                    <span class="input-group-text user-select-none">Zero</span>
                                </div>
                            </div>
                        </div>
                        <div class="row p-1" :class="{ hide: device.config.cf_rotate_relative }">
                            <div class='col'>
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.rot_limit_low">
                                    <span class="input-group-text user-select-none">Min</span>
                                </div>
                            </div>
                        </div>
                        <div class="row p-1" v-show="!device.config.cf_rotate_relative">
                            <div class='col'>
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.rot_limit_high">
                                    <span class="input-group-text user-select-none">Max</span>
                                </div>
                            </div>
                        </div>
                    </div>
                    <div class='col-lg'>
                        <div class="row p-1">
                            <div class="col">
                                <div class="form-check">
                                    <label class="form-check-label user-select-none" for="extended_check_rot">Extended
                                        CC</label>
                                    <input class="form-check-input pull-left" type="checkbox" id="extended_check_rot"
                                        v-model="device.config.cf_rotate_extended">
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class='col'>
                                <CCDropDown v-model="device.config.rot_control_high" :label="device.config.cf_rotate_extended ?
                                'MSB' : 'CC'" />
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class='col'>
                                <CCDropDown v-model="device.config.rot_control_low" label="LSB"
                                    :hidden="!device.config.cf_rotate_extended" />
                            </div>
                        </div>
                        <div class="row p-1 mb-2 mt-3">
                            <div class="col">
                                <div class="input-group">
                                    <label class="input-group-text" for="inputGroupSelect01">Accel</label>
                                    <select class="form-select smaller-text" id="inputGroupSelect01"
                                        v-model="device.config.cf_acceleration">
                                        <option selected value="0">Off</option>
                                        <option value="1">Low</option>
                                        <option value="2">Medium</option>
                                        <option value="3">High</option>
                                    </select>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Button -->

            <div class='col-lg-3 mx-3'>
                <div class="row">
                    <div class="col">
                        <h5>Button</h5>
                    </div>
                </div>
                <div class="row pt-2 border rounded-3">
                    <div class="col-lg">
                        <div class="row p-1">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.btn_channel">
                                    <span class="input-group-text user-select-none">Chan</span>
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                <div class="mt-1 mb-1">
                                    <Toggle v-model="device.config.cf_btn_momentary" :pixelsHigh="32" :pixelsWide="140"
                                        :dotSize="0.75" :marginPixels="8" :rounded="1">

                                        <template #checked-text>Momentary</template>

                                        <template #unchecked-text>Toggle</template>
                                    </Toggle>
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.btn_value_1">
                                    <span class="input-group-text user-select-none">{{ device.config.cf_btn_momentary
                                ? "OFF" : "A" }}</span>
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control"
                                        v-model.number="device.config.btn_value_2">
                                    <span class="input-group-text user-select-none">{{ device.config.cf_btn_momentary
                                ? "ON" : "B" }}</span>
                                </div>
                            </div>
                        </div>
                    </div>
                    <div class="col-lg">
                        <div class="row p-1">
                            <div class="col">
                                <div class="form-check">
                                    <label class="form-check-label user-select-none" for="extended_check_btn">Extended
                                        CC</label>
                                    <input class="form-check-input pull-left" type="checkbox" id="extended_check_btn"
                                        v-model="device.config.cf_btn_extended">
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                <CCDropDown v-model="device.config.btn_control_high" :label="device.config.cf_btn_extended ?
                                'MSB' : 'CC'" />
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                <CCDropDown v-model="device.config.btn_control_low" label="LSB"
                                    :hidden="!device.config.cf_btn_extended" />
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- LED -->

            <div class='col-lg-2 mx-3'>
                <div class="row">
                    <div class="col">
                        <h5>LED</h5>
                    </div>
                </div>
                <div class="row pt-2 border rounded-3 px-1">
                    <div class="col">
                        <div class="row my-1">
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
                                    v-model="device.config.cf_led_flash_on_rot">
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
                                    v-model="device.config.cf_led_flash_on_limit">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_click">Button is
                                    pressed</label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_click"
                                    v-model="device.config.cf_led_flash_on_click">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_release">Button is
                                    released</label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_release"
                                    v-model="device.config.cf_led_flash_on_release">
                            </div>
                        </div>
                    </div>
                    <hr class="mt-3" />
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="track_button_value">LED tracks
                                    button
                                    value</label>
                                <input class="form-check-input pull-left" type="checkbox" id="track_button_value"
                                    v-model="device.config.cf_led_track_button_toggle">
                            </div>
                        </div>
                    </div>
                    <div class="row mb-2">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="led_invertflash_on_release">Invert
                                    LED</label>
                                <input class="form-check-input pull-left" type="checkbox" id="led_invert"
                                    v-model="device.config.cf_led_invert">
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<style>
.hide {
    visibility: hidden;
}

.smaller-text {
    font-size: smaller;
}

.slim-button {
    padding-top: 0px !important;
    padding-bottom: 2px !important;
}

#canonical-button {
    display: none;
    background-color: var(--bs-secondary-bg);
    color: var(--bs-body-color);
}

.input-group-text {
    font-size: smaller;
}
</style>
