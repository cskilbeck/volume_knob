<script setup>

import Device from './Device.vue'

defineProps({
    devices: {
        type: [Array, Object]
    }
})

</script>

<template>
    <div class="row" v-for="device in devices.value">
        <Device :device="device" />
    </div>
</template>
