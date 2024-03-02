<script setup>

import { watch, getCurrentInstance, ref } from 'vue';
import { VueToggles } from "vue-toggles";
import midi from '../Midi.js'
import Modal from './Modal.vue'
import CCDropDown from './CCDropDown.vue';

const props = defineProps({
    device: {
        type: Object,
        required: true
    }
});

const flashModal = ref(false);

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

midi.on_config_changed((device) => {
    props.device.config = device.config;
    instance?.proxy?.$forceUpdate();
});

</script>

<template>
    <div class="container bg-black border rounded-3 py-3">
        <div class='row p-2 pb-3'>

            <!-- Name, Serial, Buttons -->

            <div class='col-lg-2  text-center'>
                <div class='row'>
                    <div class="container">
                        <h5>{{ device.name }}</h5>
                    </div>
                </div>
                <div class='row mx-4'>
                    <div class="container small">
                        Firmware
                        <span class="text-primary-emphasis font-monospace">v{{ device.firmware_version >> 8
                        }}.{{ (device.firmware_version & 0x7f).toString(10).padStart(2, '0') }}</span>

                    </div>
                </div>
                <div class='row mx-4'>
                    <div class="container small">
                        Serial #
                        <span class="text-body-secondary font-monospace">{{ device.serial_str }}</span>
                    </div>
                </div>
                <div class='row mt-3 mx-5'>
                    <div class="col mx-3">
                        <div class='row'>
                            <button class='btn btn-sm btn-primary'
                                v-on:click='midi.flash_device_led(device.device_index)'>Flash
                                LED</button>
                        </div>
                        <div class='row mt-2'>
                            <button class='btn btn-sm btn-primary'
                                v-on:click='midi.read_flash(device.device_index)'>Load</button>
                        </div>
                        <div class='row mt-2'>
                            <button class='btn btn-sm btn-primary'
                                v-on:click='midi.write_flash(device.device_index)'>Save</button>
                        </div>
                        <div class='row mt-3'>
                            <button class='btn btn-sm btn-dark' @click='flashModal = true'>Advanced</button>
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

            <!-- Channel, Delta -->

            <div class='col-lg-3 mx-3'>
                <div class="row">
                    <h5>Rotation</h5>
                </div>
                <div class="row pt-2 border rounded-3 bg-dark">
                    <div class="col-lg">
                        <div class="row p-1">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="device.config.rot_channel">
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
                                    <VueToggles uncheckedBg="#353C40" checkedBg="#353C40" :width="110" :height="25"
                                        dotColor="#202420" fontSize="14" checkedText="Relative" uncheckedText="Absolute"
                                        v-model="device.config.cf_rotate_relative" />
                                </div>
                            </div>
                        </div>
                        <div class="row p-1" v-show="device.config.cf_rotate_relative">
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.rot_zero_point">
                                <span class="input-group-text user-select-none">Zero</span>
                            </div>
                        </div>
                        <div class="row p-1" :class="{ hide: device.config.cf_rotate_relative }">
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.rot_limit_low">
                                <span class="input-group-text user-select-none">Min</span>
                            </div>
                        </div>
                        <div class="row p-1" v-show="!device.config.cf_rotate_relative">
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.rot_limit_high">
                                <span class="input-group-text user-select-none">Max</span>
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
                            <CCDropDown v-model="device.config.rot_control_high" :label="device.config.cf_rotate_extended ?
                                'MSB' : 'CC'" />
                        </div>
                        <div class="row p-1">
                            <CCDropDown v-model="device.config.rot_control_low" label="LSB"
                                :hidden="!device.config.cf_rotate_extended" />
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
            <div class='col-lg-3 mx-3'>
                <div class="row">
                    <h5>Button</h5>
                </div>
                <div class="row pt-2 border rounded-3 bg-dark">
                    <div class="col-lg">
                        <div class="row p-1">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="device.config.btn_channel">
                                    <span class="input-group-text user-select-none">Chan</span>
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                <div class="mt-1 mb-1">
                                    <VueToggles uncheckedBg="#353C40" checkedBg="#353C40" :width="110" :height="25"
                                        dotColor="#202420" fontSize="14" checkedText="Momentary" uncheckedText="Toggle"
                                        v-model="device.config.cf_btn_momentary" />
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.btn_value_1">
                                <span class="input-group-text user-select-none">{{ device.config.cf_btn_momentary
                                    ? "OFF" : "A" }}</span>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.btn_value_2">
                                <span class="input-group-text user-select-none">{{ device.config.cf_btn_momentary
                                    ? "ON" : "B" }}</span>
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
                            <CCDropDown v-model="device.config.btn_control_high" :label="device.config.cf_btn_extended ?
                                'MSB' : 'CC'" />
                        </div>
                        <div class="row p-1">
                            <CCDropDown v-model="device.config.btn_control_low" label="LSB"
                                :hidden="!device.config.cf_btn_extended" />
                        </div>
                    </div>
                </div>
            </div>
            <div class='col-lg-3 mx-3'>
                <div class="row">
                    <h5>LED</h5>
                </div>
                <div class="row pt-2 border rounded-3 bg-dark px-1">
                    <div class="row mx-1 my-1">
                        Flash LED when:
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_rot">Knob is rotated</label>
                                <input class="form-check-input pull-left" type="checkbox" id="flash_on_rot"
                                    v-model="device.config.cf_led_flash_on_rot">
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col">
                            <div class="form-check">
                                <label class="form-check-label user-select-none" for="flash_on_limit">Rotation value hits
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
                                    clicked</label>
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
                                <label class="form-check-label user-select-none" for="track_button_value">LED tracks button
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

.input-group-text {
    font-size: smaller;
}
</style>
