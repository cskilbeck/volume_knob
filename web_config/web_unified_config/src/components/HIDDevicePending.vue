<script setup>

//////////////////////////////////////////////////////////////////////
// Shown while a HID device is known but has not identified itself yet, and
// when it has given up.
//
// Before this existed, `hid_component()` returned null for any device whose
// firmware_major was still null, so a knob that paired correctly but whose
// firmware-version reply went missing rendered NOTHING - the page looked
// empty, the device looked broken, and the only sign of life was a console
// line. A device we are talking to should be on screen, even mid-handshake.

import { computed } from 'vue'
import hid from '../hid/hid.js'

const props = defineProps({
    device: { type: Object, required: true }
});

const failed = computed(() => props.device.identify_failed === true);

function retry() {
    hid.retry_identify(props.device);
}

</script>

<template>

    <div class="container border rounded-0 bg-device border-secondary bg-secondary-subtle py-3 mb-4" style="min-width: 1000px;">
        <div class="row align-items-center">

            <div class="col-5 ps-4">
                <strong>{{ device.name }}</strong>
            </div>

            <div class="col text-body-secondary small">

                <span v-if="!failed">
                    <span class="spinner-border spinner-border-sm me-2" role="status" aria-hidden="true"></span>
                    Connecting&hellip;
                </span>

                <span v-else>
                    This device isn't responding. Unplug it, plug it back in, and try again
                    &mdash; if it keeps happening, a different USB port or cable is worth a go.
                </span>

            </div>

            <div class="col-2 text-end pe-4">
                <button v-if="failed" class="btn btn-sm border border-secondary tertiary-bg rounded-0" @click="retry()">
                    Try again
                </button>
            </div>

        </div>
    </div>

</template>

<style scoped>
.tertiary-bg {
    background-color: var(--bs-tertiary-bg);
}

.tertiary-bg:hover {
    background-color: var(--bs-secondary-bg);
}
</style>
