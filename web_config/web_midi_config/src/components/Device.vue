<script setup>

import { watch, getCurrentInstance, ref } from 'vue';
import { VueToggles } from "vue-toggles";
import midi from '../Midi.js'
import Modal from './Modal.vue'

const props = defineProps({
    device: {
        type: Object,
        required: true
    }
});

const flashModal = ref(false);

// sanitize the values

watch(props.device, (o, n) => {
    let t = o;
    let c = t.config;

    let max_zero_point = c.cf_rotate_extended ? (1 << 14) : (1 << 7);
    let max_high_value = max_zero_point - 1;

    c.rot_control_high = Math.max(0, Math.min(c.rot_control_high, 127));
    c.rot_control_low = Math.max(0, Math.min(c.rot_control_low, 127));
    c.rot_zero_point = Math.max(16, Math.min(c.rot_zero_point, max_zero_point));
    c.rot_delta = Math.max(1, Math.min(c.rot_delta, c.rot_zero_point));
    c.rot_limit_high = Math.max(2, Math.min(c.rot_limit_high, max_high_value));
    c.rot_limit_low = Math.max(0, Math.min(c.rot_limit_low, c.rot_limit_high - 1));
    c.rot_channel = Math.max(0, Math.min(c.rot_channel, 15));

    let btn_limit = c.cf_btn_extended ? (1 << 14) : (1 << 7);

    c.btn_control_high = Math.max(0, Math.min(c.btn_control_high, 127));
    c.btn_control_low = Math.max(0, Math.min(c.btn_control_low, 127));
    c.btn_value_1 = Math.max(0, Math.min(c.btn_value_1, btn_limit - 1));
    c.btn_value_2 = Math.max(0, Math.min(c.btn_value_2, btn_limit - 1));
    c.btn_channel = Math.max(0, Math.min(c.btn_channel, 15));

    n = t;
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

            <div class='col-lg-2'>
                <div class='row'>
                    <div class="container text-center">
                        <h4>{{ device.name }}</h4>
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
                <div class='row mt-1'>
                    <div class="col mx-5">
                        <div class='row'>
                            <button class='btn btn-sm btn-primary'
                                v-on:click='midi.toggle_device_led(device.device_index)'>Toggle
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
                                <div class="row">
                                    <div class="col mb-1">
                                        <p class="text-center text-warning">Warning! Only mess with this if you're
                                            quite sure you know what you're doing...</p>
                                        <p>Instructions for performing the firmware update are available <a
                                                href='https://skilbeck.com' target="_blank"
                                                rel="noreferrer noopener">here.</a>
                                            To get back to normal mode you can unplug the device and plug it back in.</p>
                                        <p></p>
                                    </div>
                                </div>
                                <div class="row">
                                    <div class="col text-center">
                                        <button class='btn btn-sm btn-danger'
                                            v-on:click='midi.flash_mode(device.device_index)'>Put this device into Firmware
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
                                    <span class="input-group-text user-select-none small_text">Chan</span>
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="device.config.rot_delta">
                                    <span class="input-group-text user-select-none small_text">Delta</span>
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
                                <span class="input-group-text user-select-none small_text">Zero</span>
                            </div>
                        </div>
                        <div class="row p-1" :class="{ hide: device.config.cf_rotate_relative }">
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.rot_limit_low">
                                <span class="input-group-text user-select-none small_text">Min</span>
                            </div>
                        </div>
                        <div class="row p-1" v-show="!device.config.cf_rotate_relative">
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.rot_limit_high">
                                <span class="input-group-text user-select-none small_text">Max</span>
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
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.rot_control_high">
                                <span class="input-group-text user-select-none small_text">{{
                                    device.config.cf_rotate_extended ?
                                    'MSB' : 'CC' }}</span>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="input-group mb-1" :class="{ hide: !device.config.cf_rotate_extended }">
                                <input type="number" class="form-control" v-model.number="device.config.rot_control_low">
                                <span class="input-group-text user-select-none small_text">LSB</span>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                Acceleration
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                <select class="form-select form-control small_text" v-model="device.config.cf_acceleration">
                                    <option value="0" selected>Off</option>
                                    <option value="1" selected>Low</option>
                                    <option value="2" selected>Medium</option>
                                    <option value="3" selected>High</option>
                                </select>
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
                                    <span class="input-group-text user-select-none small_text">Chan</span>
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
                                <span class="input-group-text user-select-none small_text">{{ device.config.cf_btn_momentary
                                    ? "OFF" : "A" }}</span>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.btn_value_2">
                                <span class="input-group-text user-select-none small_text">{{ device.config.cf_btn_momentary
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
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.btn_control_high">
                                <span class="input-group-text user-select-none small_text">{{
                                    device.config.cf_btn_extended ?
                                    'MSB' : 'CC' }}</span>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="input-group mb-1" :class="{ hide: !device.config.cf_btn_extended }">
                                <input type="number" class="form-control" v-model.number="device.config.btn_control_low">
                                <span class="input-group-text user-select-none small_text">LSB</span>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
            <div class='col-lg-3 mx-3'>
                <div class="row">
                    <h5>LED</h5>
                </div>
                <div class="row pt-2 border rounded-3 bg-dark">
                    <div class="row">
                        <div class="col">
                            LED controls
                        </div>
                    </div>
                    <div class="row mb-4">
                        <div class="col">
                            Go here...
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

.small_text {
    font-size: smaller;
}

.smaller_text {
    font-size: small;
}
</style>
