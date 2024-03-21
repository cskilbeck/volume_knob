<script setup>

import Device from './Device.vue'

const props = defineProps({
    devices: {
        type: [Array, Object]
    }
})

function got_any_devices() {
    if (Object.keys(props.devices.value).length != 0) {
        return true;
    }
    return false;
}

</script>

<template>
    <div class="row" v-for="(device, key) in props.devices.value">
        <Device :device="device" />
    </div>
</template>
