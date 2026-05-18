<script setup>

import HIDDevice_1 from './HIDDevice_1.vue'
import HIDDevice_2 from './HIDDevice_2.vue'
import MidiDevice from './MidiDevice.vue'

const props = defineProps({
    hid_devices: {
        type: [Array, Object],
        required: true
    },
    midi_devices: {
        type: [Array, Object],
        required: true
    }
})

function hid_component(device) {
    if (device.firmware_major === null) return null;
    return device.firmware_major >= 2 ? HIDDevice_2 : HIDDevice_1;
}

</script>

<template>
    <div class="row" v-for="(device, key) in props.hid_devices.value" :key="'hid-' + key">
        <component :is="hid_component(device)" :device="device" v-if="hid_component(device) !== null" />
    </div>
    <div class="row" v-for="device in props.midi_devices.value" :key="'midi-' + device.name">
        <MidiDevice :device="device" />
    </div>
</template>
