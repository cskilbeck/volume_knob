<script setup>

import HIDDevice from './HIDDevice.vue'

const props = defineProps({
    devices: {
        type: [Array, Object]
    }
})

</script>

<template>
    <div class="row" v-for="(device, key) in props.devices.value">
        <HIDDevice :device="device" />
    </div>
</template>
