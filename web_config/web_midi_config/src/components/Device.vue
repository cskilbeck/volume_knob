<script setup>

import { watch, getCurrentInstance } from 'vue';
import midi from '../Midi.js'

const props = defineProps({
    device: {
        type: Object,
        required: true
    }
});

watch(props.device, (o, n) => {
    let t = o;
    t.config.rot_control_high = Math.max(0, Math.min(t.config.rot_control_high, 127));
    t.config.rot_control_low = Math.max(0, Math.min(t.config.rot_control_low, 127));

    let max_zero_point = t.config.cf_rotate_extended ? (1 << 14) - 1 : (1 << 7) - 1;
    t.config.rot_zero_point = Math.max(16, Math.min(t.config.rot_zero_point, max_zero_point));

    t.config.rot_delta = Math.max(1, Math.min(t.config.rot_delta, t.config.rot_zero_point));

    t.config.rot_limit_high = Math.max(1, Math.min(t.config.rot_limit_high, max_zero_point));
    t.config.rot_limit_low = Math.max(0, Math.min(t.config.rot_limit_low, t.config.rot_limit_high));
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
        <div class='row p-2'>
            <div class='col-2'>
                <div class='row'>
                    <div class="container text-center">
                        <strong>{{ device.name }}</strong>
                    </div>
                </div>
                <div class='row'>
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
            <div class='col-4'>
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
                        <span class="input-group-text user-select-none">CC MSB</span>
                    </div>
                </div>
                <div class="row p-1">
                    <div class="input-group mb-1" v-show="device.config.cf_rotate_extended">
                        <input type="number" class="form-control" v-model.number="device.config.rot_control_low">
                        <span class="input-group-text user-select-none">CC LSB</span>
                    </div>
                </div>
            </div>
            <div class='col-4'>
                <div class="row p-1">
                    <div class="col">
                        <div class="form-check">
                            <input class="form-check-input" type="checkbox" v-model="device.config.cf_rotate_relative">
                            <label class="form-check-label user-select-none" for="relative_check">Relative</label>
                        </div>
                    </div>
                </div>
                <div class="row p-1" v-show="device.config.cf_rotate_relative">
                    <div class="input-group mb-1">
                        <input type="number" class="form-control" v-model.number="device.config.rot_zero_point">
                        <span class="input-group-text user-select-none">Zero point</span>
                    </div>
                </div>
                <div class="row p-1" v-show="device.config.cf_rotate_relative">
                    <div class="input-group mb-1">
                        <input type="number" class="form-control" v-model.number="device.config.rot_delta">
                        <span class="input-group-text user-select-none">Delta</span>
                    </div>
                </div>
                <div class="row p-1" v-show="!device.config.cf_rotate_relative">
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
            <div class='col-2'>
                <div class="row p-1">
                    <div class="col">
                        Acceleration
                    </div>
                </div>
                <div class="row p-1">
                    <div class="col">
                        <select class="form-select form-control w-75" v-model="device.config.cf_acceleration">
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
</template>
