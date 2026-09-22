<script setup>

//////////////////////////////////////////////////////////////////////

import DeviceList from './components/DeviceList.vue'
import DarkMode from './components/DarkMode.vue'
import Modal from './components/Modal.vue'
import UnsupportedNotice from './components/UnsupportedNotice.vue'
import hid from './hid/hid.js'
import midi from './midi/Midi.js'
import { site, wants_hid, wants_midi, OTHER_SITES, COMPONENT_HOME } from './site.js'
import { is_firefox, is_touch_device } from './platform.js'
import { computed, onMounted, ref } from 'vue'

const closeableModal = ref(false);

let got_hid_api = ref(!!navigator.hid);
let got_midi_api = ref(!!navigator.requestMIDIAccess);

let midi_started = false;
const scanning = ref(false);

// The static index.html carries the right title per domain (see
// scripts/build-sites.mjs), but `npm run dev` and ?site= don't, so keep them
// in step.
document.title = site.title;

// Set when requestMIDIAccess REJECTS — which is a different failure from the API
// being absent, and the one that used to be invisible. A browser that has Web
// MIDI can still refuse: the user (or a policy) can deny the permission, and it
// is remembered, so every later visit fails silently with an empty page and no
// explanation. It was only ever written to the console.
const midi_denied = ref(false);

// Does THIS SITE's transport exist here? A MIDI-only site does not care that
// Web HID is missing, which is exactly the noise Firefox users used to get.
const needed_api_present = computed(() =>
  (wants_hid && got_hid_api.value) || (wants_midi && got_midi_api.value));

// Capability decides, platform only picks the wording (see platform.js).
const support_state = computed(() =>
  needed_api_present.value ? 'ready' : (is_touch_device ? 'no-platform' : 'no-api'));

// Firefox has Web MIDI but hides SysEx behind an add-on install, with a dialog
// that reads like a malware warning. Say so first, in our own words, so it
// arrives expected rather than out of nowhere. Only on sites that need MIDI,
// and only while it hasn't already been granted.
// Off on usbknob.com, whose behaviour is frozen — flip firefox_notice there if
// you decide the explanation is worth having on the everything-view too.
const firefox_midi_warning = computed(() =>
  site.firefox_notice && is_firefox && wants_midi &&
  support_state.value === 'ready' && !midi_started);

// One API missing is NOT the same as none: the app still does its job for the
// other kind of device, so this is an inline notice rather than a modal that
// has to be dismissed on every visit. Chrome on Android is the real case —
// Web MIDI yes, Web HID no. Kept only on usbknob.com (the everything-view);
// on a single-device site the missing API is either fatal (handled by
// support_state) or irrelevant.
const partial_support = computed(() =>
  site.partial_banners && got_hid_api.value !== got_midi_api.value);

//////////////////////////////////////////////////////////////////////

async function scan() {

  if (scanning.value) return;
  scanning.value = true;

  try {
    // Fire HID picker concurrently with MIDI access. We re-enable the Scan
    // button when the HID picker is dismissed (so a second click doesn't
    // stack a duplicate init_device on the same device).
    //
    // wants_hid is what keeps navigator.hid.requestDevice() — and its
    // unavoidable device picker — away from people who only own a MIDI
    // device. It is the whole point of the per-domain split.
    const hid_promise = (wants_hid && got_hid_api.value)
      ? hid.init_devices() : Promise.resolve();

    if (wants_midi && got_midi_api.value) {
      if (!midi_started) {
        try {
          const main_midi_object = await navigator.requestMIDIAccess({ "sysex": true });
          midi.on_midi_startup(main_midi_object);
          midi_started = true;
        } catch (err) {
          console.log(`requestMIDIAccess failed: ${err}`);
          midi_denied.value = true;
        }
      }
      if (midi_started) {
        midi.init_devices();
      }
    }

    await hid_promise.catch(() => { /* picker errors are non-fatal */ });
  } finally {
    scanning.value = false;
  }
}

//////////////////////////////////////////////////////////////////////
// Re-attach devices the user has ALREADY granted, on load, with no click and
// no prompt of any kind. Off for usbknob.com, whose behaviour is frozen.

async function midi_already_granted() {
  try {
    if (!navigator.permissions?.query) return false;
    const status = await navigator.permissions.query({ name: 'midi', sysex: true });
    return status.state === 'granted';
  } catch (err) {
    // Firefox throws on the unknown permission name. Returning false here is
    // load-bearing: falling through to requestMIDIAccess would pop the
    // add-on install dialog with no user gesture behind it, which is strictly
    // worse than the behaviour we are trying to improve.
    return false;
  }
}

onMounted(async () => {

  if (!site.silent_reconnect || support_state.value !== 'ready') return;

  if (wants_hid && got_hid_api.value) {
    // getDevices() returns only devices already granted, so a first-time
    // visitor sees nothing and nothing is shown to them.
    hid.init_known_devices().catch(err => console.log(`init_known_devices: ${err}`));
  }

  if (wants_midi && got_midi_api.value && !midi_started) {
    if (await midi_already_granted()) {
      try {
        midi.on_midi_startup(await navigator.requestMIDIAccess({ "sysex": true }));
        midi_started = true;
        midi.init_devices();
      } catch (err) {
        console.log(`silent requestMIDIAccess failed: ${err}`);
      }
    }
  }
});

function toggle_hid_demo() {
  if (hid.has_dummy.value) hid.remove_dummy_device();
  else hid.add_dummy_device();
}

function toggle_midi_demo() {
  if (midi.has_dummy.value) midi.remove_dummy_device();
  else midi.add_dummy_device();
}

function toggle_adapter_demo() {
  if (midi.has_dummy_adapter.value) midi.remove_dummy_adapter();
  else midi.add_dummy_adapter();
}

const device_count = computed(() =>
  Object.keys(hid.hid_devices.value).length + midi.midi_devices.value.length
);

const any_scanned = computed(() => hid.scanned.done || midi.scanned.done);

const scan_disabled = computed(() =>
  !needed_api_present.value || scanning.value
);

// A device this site does not render, but which answered an Identity Request
// anyway — so we know exactly what it is and where it belongs. Costs nothing:
// a MIDI site probes every port regardless. Never shown on usbknob.com, which
// renders these devices itself.
const wrong_door = computed(() => {
  if (!site.wrong_door) return [];
  const homes = midi.midi_devices.value
    .filter(d => d.type && !site.midi_components.includes(d.type.component))
    .map(d => COMPONENT_HOME[d.type.component])
    .filter(Boolean);
  return [...new Map(homes.map(h => [h.url, h])).values()];
});

//////////////////////////////////////////////////////////////////////

</script>

<template>
  <main>

    <div class="container container-wide">
      <!-- Flex rather than three fixed col-4s: the toolbar sizes to its
           content, so adding a button pushes the title along instead of
           wrapping the row. -->
      <div class="d-flex align-items-center pt-3 pb-2 gap-3">
        <div class="flex-shrink-0 text-nowrap">
          <span v-show='any_scanned'>
            Found {{ device_count }}
            device{{ device_count != 1 ? "s" : "" }}
          </span>
        </div>
        <div class="flex-grow-1">
          <h5 class="text-center mb-0">{{ site.title }}</h5>
        </div>
        <div class="flex-shrink-0 text-end text-nowrap">
          <button v-if="site.demos.includes('hid')" class="btn border border-secondary tertiary-bg btn-sm mx-1 rounded-0" @click="toggle_hid_demo()">
            {{ hid.has_dummy.value ? '− Demo HID' : '+ Demo HID' }}
          </button>
          <button v-if="site.demos.includes('midi')" class="btn border border-secondary tertiary-bg btn-sm mx-1 rounded-0" @click="toggle_midi_demo()">
            {{ midi.has_dummy.value ? '− Demo MIDI' : '+ Demo MIDI' }}
          </button>
          <button v-if="site.demos.includes('adapter')" class="btn border border-secondary tertiary-bg btn-sm mx-1 rounded-0" @click="toggle_adapter_demo()">
            {{ midi.has_dummy_adapter.value ? '− Demo Adapter' : '+ Demo Adapter' }}
          </button>
          <button v-if="support_state === 'ready'" :disabled="scan_disabled" class="btn border border-secondary tertiary-bg btn-sm mx-2 rounded-0" @click="scan()">
            Scan
          </button>
          <DarkMode />
          <button @click="closeableModal = true" class="btn btn-sm">
            <svg xmlns="http://www.w3.org/2000/svg" width="26" height="26" fill="currentColor" class="bi bi-info-circle" viewBox="0 0 16 16">
              <path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14m0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16" />
              <path
                d="m8.93 6.588-2.29.287-.082.38.45.083c.294.07.352.176.288.469l-.738 3.468c-.194.897.105 1.319.808 1.319.545 0 1.178-.252 1.465-.598l.088-.416c-.2.176-.492.246-.686.246-.275 0-.375-.193-.304-.533zM9 4.5a1 1 0 1 1-2 0 1 1 0 0 1 2 0" />
            </svg>
          </button>
        </div>
      </div>
    </div>

    <div class="container container-wide" v-if="firefox_midi_warning">
      <!-- Said BEFORE the click, because Firefox's own dialog says "This access
           is not guaranteed to be safe" and arrives with no context. -->
      <div class="alert alert-secondary rounded-0 py-2 px-3 small">
        <strong>Firefox needs a small add-on to reach MIDI devices.</strong>
        When you click Scan, Firefox will offer to install it and will warn you
        about trusting this site &mdash; that is its standard wording for any site
        using MIDI, and the add-on only grants access to MIDI devices. Chrome, Edge
        and Opera work without it.
      </div>
    </div>

    <div class="container container-wide" v-if="wrong_door.length">
      <div class="alert alert-info rounded-0 py-2 px-3 small" v-for="home in wrong_door" :key="home.url">
        Found a <strong>{{ home.name }}</strong> &mdash; that one is configured at
        <a :href="home.url">{{ home.url.replace('https://', '') }}</a>.
      </div>
    </div>

    <div class="container container-wide" v-if="partial_support || midi_denied">

      <div class="alert alert-warning rounded-0 py-2 px-3 small" v-if="midi_denied">
        <strong>This browser blocked access to MIDI devices.</strong>
        Web MIDI needs permission, and a refusal is remembered &mdash; so nothing will
        appear until it is allowed. Click the icon at the left of the address bar, set
        MIDI device control to <em>Allow</em>, then reload and scan again.
      </div>

      <div class="alert alert-warning rounded-0 py-2 px-3 small" v-if="partial_support && !got_midi_api">
        <strong>This browser does not support Web MIDI.</strong>
        The TRS MIDI Adapter and the MIDI version of the Tiny USB Knob cannot be
        configured here. HID keyboards still work.
        <a href="https://caniuse.com/midi" target="_blank" rel="noreferrer noopener">Which
          browsers support Web MIDI</a>.
      </div>

      <div class="alert alert-warning rounded-0 py-2 px-3 small" v-if="partial_support && !got_hid_api">
        <strong>This browser does not support Web HID.</strong>
        HID keyboards and the HID version of the Tiny USB Knob cannot be configured here.
        MIDI devices still work.
        <a href="https://caniuse.com/webhid" target="_blank" rel="noreferrer noopener">Which
          browsers support Web HID</a>.
      </div>

    </div>

    <div class="container container-wide">
      <DeviceList v-if="support_state === 'ready'" :hid_devices="hid.hid_devices" :midi_devices="midi.midi_devices" />
      <UnsupportedNotice v-else :state="support_state" />
    </div>

    <!-- The HID site cannot detect a MIDI device without asking for the MIDI
         permission we are avoiding, so it gets a standing link rather than the
         wrong_door notice above. -->
    <div class="container container-wide" v-if="OTHER_SITES.length">
      <div class="row">
        <div class="col text-center small text-body-secondary my-4">
          Got a different one?
          <span v-for="(other, i) in OTHER_SITES" :key="other.url">
            <span v-if="i > 0"> &middot; </span>
            <a :href="other.url" class="text-body-secondary">{{ other.name }}</a>
          </span>
        </div>
      </div>
    </div>

    <Modal v-model="closeableModal" maxwidth="40%" closeable :header="`It's a Configurator for the ${site.product}`">
      <div class="row">
        <div class="col my-3 text-center">
          {{ site.blurb }}
        </div>
      </div>
      <div class="row">
        <div class="col my-2 text-center">
          Get your own {{ site.product }} <a :href="site.store_url" target="_blank" rel="noreferrer noopener">here</a>
        </div>
      </div>
      <div class="row mt-3">
        <div class="col text-start ms-2">
          <a class="" href="https://twitter.com/cskilbeck" target="_blank" rel="noreferrer noopener">@cskilbeck</a>
        </div>
        <div class="col text-end">
          <span class="align-text-bottom">
            <a href="https://github.com/cskilbeck/volume_knob" target="_blank" rel="noreferrer noopener" class="link-body-emphasis mx-1">
              <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
                <path
                  d="M12 0c-6.626 0-12 5.373-12 12 0 5.302 3.438 9.8 8.207 11.387.599.111.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23.957-.266 1.983-.399 3.003-.404 1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.911 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576 4.765-1.589 8.199-6.086 8.199-11.386 0-6.627-5.373-12-12-12z" />
              </svg>
            </a>
            <a href="https://vuejs.org" target="_blank" rel="noreferrer noopener" class="mx-1"> <svg viewBox="0 0 128 128" width="24" height="24">
                <path fill="#42b883" d="M78.8,10L64,35.4L49.2,10H0l64,110l64-110C128,10,78.8,10,78.8,10z"></path>
                <path fill="#35495e" d="M78.8,10L64,35.4L49.2,10H25.6L64,76l38.4-66H78.8z"></path>
              </svg>
            </a>
            <a href="https://getbootstrap.com" target="_blank" rel="noreferrer noopener" class="link-body-emphasis mx-1">
              <svg xmlns="http://www.w3.org/2000/svg" width="30" height="24" viewBox="0 0 118 94" role="img">
                <title>Bootstrap</title>
                <path fill-rule="evenodd" clip-rule="evenodd"
                  d="M24.509 0c-6.733 0-11.715 5.893-11.492 12.284.214 6.14-.064 14.092-2.066 20.577C8.943 39.365 5.547 43.485 0 44.014v5.972c5.547.529 8.943 4.649 10.951 11.153 2.002 6.485 2.28 14.437 2.066 20.577C12.794 88.106 17.776 94 24.51 94H93.5c6.733 0 11.714-5.893 11.491-12.284-.214-6.14.064-14.092 2.066-20.577 2.009-6.504 5.396-10.624 10.943-11.153v-5.972c-5.547-.529-8.934-4.649-10.943-11.153-2.002-6.484-2.28-14.437-2.066-20.577C105.214 5.894 100.233 0 93.5 0H24.508zM80 57.863C80 66.663 73.436 72 62.543 72H44a2 2 0 01-2-2V24a2 2 0 012-2h18.437c9.083 0 15.044 4.92 15.044 12.474 0 5.302-4.01 10.049-9.119 10.88v.277C75.317 46.394 80 51.21 80 57.863zM60.521 28.34H49.948v14.934h8.905c6.884 0 10.68-2.772 10.68-7.727 0-4.643-3.264-7.207-9.012-7.207zM49.948 49.2v16.458H60.91c7.167 0 10.964-2.876 10.964-8.281 0-5.406-3.903-8.178-11.425-8.178H49.948z"
                  fill="currentColor"></path>
              </svg>
            </a>
          </span>
        </div>
      </div>
    </Modal>

    <!-- The old "not supported" modal lived here. It has been replaced by
         UnsupportedNotice above: a modal had to be dismissed on every visit and
         then left the reader looking at a page that could never work, and it
         could not tell "wrong browser" from "wrong kind of device". -->
  </main>
</template>

<style>
.tertiary-bg {
  background-color: var(--bs-tertiary-bg);
}

.tertiary-bg:hover {
  background-color: var(--bs-secondary-bg)
}

.container-wide {
  min-width: 80%;
}

body {
  transition: background-color 0.2s, color 0.2s;
}
</style>
