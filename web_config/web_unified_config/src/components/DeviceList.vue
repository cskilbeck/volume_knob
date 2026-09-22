<script setup>

import HIDDevice_1 from './HIDDevice_1.vue'
import HIDDevice_2 from './HIDDevice_2.vue'
import MidiDevice from './MidiDevice.vue'
import MidiAdapterDevice from './MidiAdapterDevice.vue'
import { site } from '../site.js'

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

// Which component renders a MIDI device is decided by its registry entry (see
// DEVICE_TYPES in Midi.js), which is resolved from the model number in its
// identity reply. A device that has not answered yet, or answered with a model
// we do not know, has no type and renders nothing.
const MIDI_COMPONENTS = {
    MidiDevice,
    MidiAdapterDevice,
};

// ...and then narrowed to the devices THIS SITE is for. A single-device site
// still enumerates every MIDI port — Web MIDI has no per-device permission, so
// it cannot do otherwise — but it renders only its own gadget. App.vue turns
// the ones we skip here into a link to the right domain.
function midi_component(device) {
    const name = device.type?.component;
    if (!name || !site.midi_components.includes(name)) return null;
    return MIDI_COMPONENTS[name] ?? null;
}

</script>

<template>
    <div class="row" v-for="(device, key) in props.hid_devices.value" :key="'hid-' + key">
        <component :is="hid_component(device)" :device="device" v-if="hid_component(device) !== null" />
    </div>
    <!-- Keyed on device.key (the output port's id), not on the name: two
         adapters both call themselves "midi_adapter", and duplicate keys make
         Vue's instance reuse unreliable. -->
    <div class="row" v-for="device in props.midi_devices.value" :key="'midi-' + device.key">
        <component :is="midi_component(device)" :device="device" v-if="midi_component(device) !== null" />
    </div>
</template>
