<script setup>

import { watch, getCurrentInstance } from 'vue';
import { VueToggles } from "vue-toggles";
import midi from '../Midi.js'

const props = defineProps({
    device: {
        type: Object,
        required: true
    }
});

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

            <div class='col-2'>
                <div class='row'>
                    <div class="container text-center">
                        <h4>{{ device.name }}</h4>
                    </div>
                </div>
                <div class='row h6'>
                    <div class="container text-center fw-lighter font-monospace small text-success">
                        {{ device.serial_str }}
                    </div>
                </div>
                <div class='row p-1'>
                    <div class="col">
                    </div>
                    <div class="col-7">
                        <div class='row p-1'>
                            <button class='btn btn-sm btn-primary'
                                v-on:click='midi.toggle_device_led(device.device_index)'>Toggle
                                LED</button>
                        </div>
                        <div class='row p-1'>
                            <button class='btn btn-sm btn-primary'
                                v-on:click='midi.read_flash(device.device_index)'>Load</button>
                        </div>
                        <div class='row p-1'>
                            <button class='btn btn-sm btn-primary'
                                v-on:click='midi.write_flash(device.device_index)'>Save</button>
                        </div>
                    </div>
                    <div class="col">
                    </div>
                </div>
            </div>

            <!-- Channel, Delta -->

            <div class='col-4 mx-3'>
                <div class="row">
                    <h5>Rotation</h5>
                </div>
                <div class="row pt-2 border rounded-3 bg-dark">
                    <div class="col-6">
                        <div class="row p-1">
                            <div class="col">
                                <div class="input-group mb-1">
                                    <input type="number" class="form-control" v-model.number="device.config.rot_channel">
                                    <span class="input-group-text user-select-none">Channel</span>
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
                                    <VueToggles uncheckedBg="#0D6EFD" checkedBg="#0D6EFD" :width="100" :height="25"
                                        fontSize="14" checkedText="Relative" uncheckedText="Absolute"
                                        v-model="device.config.cf_rotate_relative" />
                                </div>
                            </div>
                        </div>
                        <div class="row p-1" v-show="device.config.cf_rotate_relative">
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.rot_zero_point">
                                <span class="input-group-text user-select-none">Zero point</span>
                            </div>
                        </div>
                        <div class="row p-1" :class="{ hide: device.config.cf_rotate_relative }">
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.rot_limit_low">
                                <span class="input-group-text user-select-none">Low Limit</span>
                            </div>
                        </div>
                        <div class="row p-1" v-show="!device.config.cf_rotate_relative">
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.rot_limit_high">
                                <span class="input-group-text user-select-none">High Limit</span>
                            </div>
                        </div>
                    </div>
                    <div class='col-5'>
                        <div class="row p-1">
                            <div class="col">
                                <div class="form-check">
                                    <label class="form-check-label user-select-none" for="extended_check">Extended
                                        control</label>
                                    <input class="form-check-input pull-left" type="checkbox"
                                        v-model="device.config.cf_rotate_extended">
                                </div>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="input-group mb-1">
                                <input type="number" class="form-control" v-model.number="device.config.rot_control_high">
                                <span class="input-group-text user-select-none">{{ device.config.cf_rotate_extended ?
                                    'CC MSB' : 'CC' }}</span>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="input-group mb-1" :class="{ hide: !device.config.cf_rotate_extended }">
                                <input type="number" class="form-control" v-model.number="device.config.rot_control_low">
                                <span class="input-group-text user-select-none">CC LSB</span>
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                Acceleration
                            </div>
                        </div>
                        <div class="row p-1">
                            <div class="col">
                                <select class="form-select form-control" v-model="device.config.cf_acceleration">
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
            <div class='col-3 mx-3'>
                <div class="row">
                    <h5>Button</h5>
                </div>
                <div class="row pt-2 border rounded-3 bg-dark">
                    <div class="row">
                        <div class="col">
                            Button controls
                        </div>
                    </div>
                    <div class="row mb-4">
                        <div class="col">
                            Go here...
                        </div>
                    </div>
                </div>
            </div>
            <div class='col-2 mx-3'>
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
</style>
