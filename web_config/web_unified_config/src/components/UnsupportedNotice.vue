<script setup>

//////////////////////////////////////////////////////////////////////
// Shown INSTEAD of the configurator when the browser cannot possibly work.
//
// Two different problems, which want two different answers:
//
//   no-platform  a phone or tablet. Nothing will ever work here, in any
//                browser, so the only useful thing we can do is help them
//                get this URL onto a real computer.
//
//   no-api       a desktop browser without the API this site needs —
//                Safari (neither API), or Firefox on the HID site. A
//                different browser on the SAME machine fixes it.
//
// Which one is chosen is decided by capability first (see App.vue); the
// platform flags only pick the wording.

import { computed } from 'vue'
import { site, wants_hid, wants_midi } from '../site.js'
import { is_ios } from '../platform.js'

const props = defineProps({
    state: { type: String, required: true },   // 'no-platform' | 'no-api'
});

const here = computed(() => `${location.protocol}//${location.host}/`);

// A phone can't configure anything, but it can hand the address to a machine
// that can. mailto: is the lowest-friction way to do that without asking for
// anything or storing anything.
const mail_link = computed(() => {
    const subject = `${site.product} configurator`;
    const body = `Open this on a desktop or laptop computer, in Chrome or Edge:\n\n${here.value}\n`;
    return `mailto:?subject=${encodeURIComponent(subject)}&body=${encodeURIComponent(body)}`;
});

const needs = computed(() => {
    if (wants_hid && wants_midi) return 'Web HID or Web MIDI';
    return wants_hid ? 'Web HID' : 'Web MIDI';
});

</script>

<template>

    <div class="row justify-content-center my-4">
        <div class="col" style="max-width: 46rem;">

            <!-- ============================================ phone / tablet -->
            <div v-if="props.state === 'no-platform'" class="card rounded-0 border-secondary">
                <div class="card-body px-4 py-4">

                    <h5 class="card-title mb-3">This needs a desktop or laptop computer</h5>

                    <p class="mb-3">
                        Configuring the {{ site.product }} uses a browser feature
                        ({{ needs }}) that phones and tablets don't have
                        <span v-if="is_ios">&mdash; on an iPhone or iPad, no browser has it,
                            because they all use the same engine underneath</span>.
                        Your {{ site.product }} is fine; it just can't be set up from here.
                    </p>

                    <p class="mb-2">On a computer, using Chrome, Edge or Opera, go to:</p>

                    <p class="fs-4 text-break my-3">
                        <strong>{{ site.host }}</strong>
                    </p>

                    <a :href="mail_link" class="btn btn-sm border border-secondary tertiary-bg rounded-0">
                        Email this link to myself
                    </a>

                    <hr class="my-4">

                    <p class="small mb-0">
                        Need one? <a :href="site.store_url" target="_blank" rel="noreferrer noopener">The shop</a>.
                    </p>
                </div>
            </div>

            <!-- ====================================== desktop, wrong browser -->
            <div v-else class="card rounded-0 border-secondary">
                <div class="card-body px-4 py-4">

                    <h5 class="card-title mb-3">This browser can't talk to USB devices</h5>

                    <p class="mb-3">
                        Configuring the {{ site.product }} needs {{ needs }}, which this
                        browser doesn't support. <strong>Chrome, Edge or Opera</strong> on the
                        same computer will work &mdash; nothing needs to be installed, and
                        your {{ site.product }} is fine.
                    </p>

                    <p class="small mb-0">
                        Which browsers work:
                        <a v-if="wants_hid" href="https://caniuse.com/webhid" target="_blank" rel="noreferrer noopener">Web HID</a>
                        <span v-if="wants_hid && wants_midi"> and </span>
                        <a v-if="wants_midi" href="https://caniuse.com/midi" target="_blank" rel="noreferrer noopener">Web MIDI</a>.
                    </p>
                </div>
            </div>

        </div>
    </div>

</template>
