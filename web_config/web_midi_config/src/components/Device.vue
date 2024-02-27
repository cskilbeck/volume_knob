<script setup>

import { defineProps, watch } from 'vue';
import midi from './Midi.js'

const props = defineProps({
    device: {
        type: Object,
        required: true
    }
});

watch(props.device, (o, n) => {
    n = o;
    n.config.cc_msb = Math.max(0, Math.min(n.config.cc_msb, 127));
    n.config.cc_lsb = Math.max(0, Math.min(n.config.cc_lsb, 127));
    n.config.zero_point = Math.max(16, Math.min(n.config.zero_point, 128));
    n.config.delta = Math.max(1, Math.min(n.config.delta, 127));
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
                            <input class="form-check-input pull-left" id="extended_check" type="checkbox"
                                v-model="device.config.extended">
                        </div>
                    </div>
                </div>
                <div class="row p-1">
                    <div class="input-group mb-1">
                        <input type="number" class="form-control" id="control1_text" v-model.number="device.config.cc_msb">
                        <span class="input-group-text user-select-none">CC MSB</span>
                    </div>
                </div>
                <div class="row p-1">
                    <div class="input-group mb-1" v-show="device.config.extended">
                        <input type="number" class="form-control" id="control2_text" v-model.number="device.config.cc_lsb">
                        <span class="input-group-text user-select-none">CC LSB</span>
                    </div>
                </div>
            </div>
            <div class='col-4'>
                <div class="row p-1">
                    <div class="col">
                        <div class="form-check">
                            <input class="form-check-input" id="relative_check" type="checkbox"
                                v-model="device.config.relative">
                            <label class="form-check-label user-select-none" for="relative_check">Relative</label>
                        </div>
                    </div>
                </div>
                <div class="row p-1" v-show="device.config.relative">
                    <div class="input-group mb-1">
                        <input type="number" class="form-control" id="zeropoint_text"
                            v-model.number="device.config.zero_point">
                        <span class="input-group-text user-select-none">Zero point</span>
                    </div>
                </div>
                <div class="row p-1" v-show="device.config.relative">
                    <div class="input-group mb-1">
                        <input type="number" class="form-control" id="delta_text" v-model.number="device.config.delta">
                        <span class="input-group-text user-select-none">Delta</span>
                    </div>
                </div>
            </div>
            <div class='col-2'>
                <div class="row">
                    <div class="col-1">
                    </div>
                    <div class="col-5">
                        Acceleration
                    </div>
                </div>
                <div class="row p-3">
                    <div class="col-1">
                    </div>
                    <div class="col-5">
                        <div class="form-check">
                            <input class="form-check-input" type="radio" id="accel_off" name="acceleration" value="0"
                                checked>
                            <label class="form-check-label user-select-none" for="accel_off">Off</label>
                        </div>
                        <div class="form-check">
                            <input class="form-check-input" type="radio" id="accel_low" name="acceleration" value="1">
                            <label class="form-check-label user-select-none" for="accel_low">Low</label>
                        </div>
                        <div class="form-check">
                            <input class="form-check-input" type="radio" id="accel_medium" name="acceleration" value="2">
                            <label class="form-check-label user-select-none" for="accel_medium">Medium</label>
                        </div>
                        <div class="form-check">
                            <input class="form-check-input" type="radio" id="accel_high" name="acceleration" value="3">
                            <label class="form-check-label user-select-none" for="accel_high">High</label>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>
