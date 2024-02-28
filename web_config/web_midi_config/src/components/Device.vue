<script setup>

import { defineProps, watch, getCurrentInstance } from 'vue';
import midi from './Midi.js'

const props = defineProps({
    device: {
        type: Object,
        required: true
    }
});

watch(props.device, (o, n) => {
    let t = o;
    t.config.cc_msb = Math.max(0, Math.min(t.config.cc_msb, 127));
    t.config.cc_lsb = Math.max(0, Math.min(t.config.cc_lsb, 127));
    t.config.zero_point = Math.max(16, Math.min(t.config.zero_point, 128));
    t.config.delta = Math.max(1, Math.min(t.config.delta, 127));
    n = t;
});

const instance = getCurrentInstance();

midi.on_config_changed((device) => {
    props.device.config = device.config;
    instance?.proxy?.$forceUpdate();
});

</script>

<template>
    <div class="container " style="background-color:#181818;">
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
                            <input class="form-check-input pull-left" type="checkbox" v-model="device.config.extended">
                        </div>
                    </div>
                </div>
                <div class="row p-1">
                    <div class="input-group mb-1">
                        <input type="number" class="form-control" v-model.number="device.config.cc_msb">
                        <span class="input-group-text user-select-none">CC MSB</span>
                    </div>
                </div>
                <div class="row p-1">
                    <div class="input-group mb-1" v-show="device.config.extended">
                        <input type="number" class="form-control" v-model.number="device.config.cc_lsb">
                        <span class="input-group-text user-select-none">CC LSB</span>
                    </div>
                </div>
            </div>
            <div class='col-4'>
                <div class="row p-1">
                    <div class="col">
                        <div class="form-check">
                            <input class="form-check-input" type="checkbox" v-model="device.config.relative">
                            <label class="form-check-label user-select-none" for="relative_check">Relative</label>
                        </div>
                    </div>
                </div>
                <div class="row p-1" v-show="device.config.relative">
                    <div class="input-group mb-1">
                        <input type="number" class="form-control" v-model.number="device.config.zero_point">
                        <span class="input-group-text user-select-none">Zero point</span>
                    </div>
                </div>
                <div class="row p-1" v-show="device.config.relative">
                    <div class="input-group mb-1">
                        <input type="number" class="form-control" v-model.number="device.config.delta">
                        <span class="input-group-text user-select-none">Delta</span>
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
                        <select class="form-select form-control" v-model="device.config.acceleration">
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
