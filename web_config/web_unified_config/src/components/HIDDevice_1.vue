<script setup>
import HIDDeviceBase from './HIDDeviceBase.vue'
defineProps({ device: { type: Object, required: true } })
</script>

<template>
    <HIDDeviceBase :device="device" :supports_mouse="false" />
</template>
