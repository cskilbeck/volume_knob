<script setup>

import midi from './Midi.js'

defineProps({
    device: {
        type: Object,
        required: true
    }
})
</script>

<template>
    <div class="container bg-light border">
        <div class='row p-2'>
            <div class='col-2'>
                <div class='row'>
                    <div class="container text-center">
                        <strong>{{ device.input.name }}</strong>
                    </div>
                </div>
                <div class='row'>
                    <div class="container text-center">
                        {{ device.serial_str }}
                    </div>
                </div>
                <div class='row p-1'>
                    <div class="col">
                    </div>
                    <div class="col-7">
                        <div class='row p-1'>
                            <button class='btn btn-sm btn-primary' v-on:click='toggle_device_led(undefined)'>Toggle
                                LED</button>
                        </div>
                        <div class='row p-1'>
                            <button class='btn btn-sm btn-primary' v-on:click='read_flash(undefined)'>Load</button>
                        </div>
                        <div class='row p-1'>
                            <button class='btn btn-sm btn-primary' v-on:click='write_flash(undefined)'>Save</button>
                        </div>
                    </div>
                    <div class="col">
                    </div>
                </div>
            </div>
            <div class='col-3'>
                <div class="row p-1">
                    <div class="col">
                        <div class="form-check">
                            <label class="form-check-label user-select-none" for="extended_check">Extended
                                control</label>
                            <input class="form-check-input pull-left" id="extended_check" type="checkbox">
                        </div>
                    </div>
                </div>
                <div class="row p-1">
                    <div class="input-group mb-1">
                        <input type="text" class="form-control" id="control1_text">
                        <span class="input-group-text user-select-none">CC LSB</span>
                    </div>
                </div>
                <div class="row p-1">
                    <div class="input-group mb-1">
                        <input type="text" class="form-control" id="control2_text">
                        <span class="input-group-text user-select-none">CC MSB</span>
                    </div>
                </div>
                <div class="row p-1">
                    <div class="col">
                        <div class="form-check">
                            <input class="form-check-input" id="relative_check" type="checkbox">
                            <label class="form-check-label user-select-none" for="relative_check">Relative</label>
                        </div>
                    </div>
                </div>
                <div class="row p-1">
                    <div class="input-group mb-1">
                        <input type="text" class="form-control" id="zeropoint_text">
                        <span class="input-group-text user-select-none">Zero point</span>
                    </div>
                </div>
                <div class="row p-1">
                    <div class="input-group mb-1">
                        <input type="text" class="form-control" id="delta_text">
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
                            <input class="form-check-input" type="radio" id="accel_off" name="acceleration" checked>
                            <label class="form-check-label user-select-none" for="accel_off">Off</label>
                        </div>
                        <div class="form-check">
                            <input class="form-check-input" type="radio" id="accel_low" name="acceleration">
                            <label class="form-check-label user-select-none" for="accel_low">Low</label>
                        </div>
                        <div class="form-check">
                            <input class="form-check-input" type="radio" id="accel_medium" name="acceleration">
                            <label class="form-check-label user-select-none" for="accel_medium">Medium</label>
                        </div>
                        <div class="form-check">
                            <input class="form-check-input" type="radio" id="accel_high" name="acceleration">
                            <label class="form-check-label user-select-none" for="accel_high">High</label>
                        </div>
                    </div>
                </div>
            </div>
            <div class='col-5'>
                <div class='row p-1'>
                    <div class="form-floating p-1">
                        <textarea class="form-control font-monospace" style="height: 100px" cols='40' id='memory_undefined'
                            v-on:input='on_input_change(this, undefined)'></textarea>
                        <label for="memory_undefined">Input</label>
                    </div>
                </div>
                <div class='row p-1'>
                    <div class="form-floating p-1">
                        <textarea class="form-control font-monospace" style="height: 100px" cols='40' id='memory_contents'
                            v-on:input='on_input_change(this, undefined)'></textarea>
                        <label for="memory_contents">Contents</label>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>
