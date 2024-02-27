<script setup>

import Device from './Device.vue'

defineProps({
    devices: {
        type: [Array, Object],
        required: true
    }
})
</script>

<template>
    <div class="row" v-for="device in devices.value">
        <Device v-bind:device="device" />
    </div>
</template>
