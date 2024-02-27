<script setup>

import Device from './DeviceComponent.vue'

defineProps({
    devices: {
        type: Array,
        required: true
    }
})
</script>

<template>
    <div class="container">
        <div class="row" v-for="device in devices.value">
            <Device v-bind:device="device" />
        </div>
    </div>
</template>
