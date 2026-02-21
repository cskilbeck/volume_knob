<script setup>

import HIDDevice_1 from './HIDDevice_1.vue'
import HIDDevice_2 from './HIDDevice_2.vue'

const props = defineProps({
    devices: {
        type: [Array, Object]
    }
})

function device_component(device) {
    if (device.firmware_major === null) return null;
    return device.firmware_major >= 2 ? HIDDevice_2 : HIDDevice_1;
}

</script>

<template>
    <div class="row" v-for="(device, key) in props.devices.value">
        <component :is="device_component(device)" :device="device" v-if="device_component(device) !== null" />
    </div>
</template>
