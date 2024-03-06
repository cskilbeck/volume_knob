<script setup>

//////////////////////////////////////////////////////////////////////

import DeviceList from './components/DeviceList.vue'
import DarkMode from './components/DarkMode.vue'
import Modal from './components/Modal.vue'
import midi from './Midi.js'
import { ref } from 'vue'

const closeableModal = ref(false);

//////////////////////////////////////////////////////////////////////

navigator.requestMIDIAccess({ "sysex": true })
  .then(
    async (main_midi_object) => {
      midi.on_midi_startup(main_midi_object);
    },
    (err) => {
      console.log(`requestMIDIAccess failed: ${err}`);
    }
  );

</script>

<template>
  <main>
    <header>
      <h5 class="text-center mt-4 mb-4">Tiny USB MIDI Knob Configurator</h5>
    </header>
    <div class="container container-wide">
      <div class="row mb-3">
        <div class="col-2">
          <span v-show='midi.scanned.done'>
            Found {{ midi.midi_devices.value.length }}
            device{{ midi.midi_devices.value.length != 1 ? "s" : "" }}
          </span>
        </div>
        <div class="col-8 text-center">
          <button class="btn border border-secondary tertiary-bg btn-sm" @click="midi.init_devices()">
            Scan
          </button>
        </div>
        <div class="col-2 text-end">
          <DarkMode />
          <button @click="closeableModal = true" class="btn tertiary-bg border border-secondary btn-sm">
            About
          </button>
        </div>
      </div>
      <DeviceList :devices="midi.midi_devices" />
    </div>
    <Modal v-model="closeableModal" maxwidth="30%" closeable header="It's a Configurator for the Tiny USB Midi Knob">
      <div class="row">
        <div class="col text-center h5">
          Crafted with care by <a class="" href="https://twitter.com/cskilbeck" target="_blank"
            rel="noreferrer noopener">@cskilbeck</a>
        </div>
      </div>
      <div class="row mt-3">
        <div class="col text-center">
          Get your own Tiny USB Midi Knob <a href="https://www.etsy.com/uk/shop/TinyLittleGadgets" target="_blank"
            rel="noreferrer noopener">here</a>
        </div>
      </div>
      <div class="row mt-3">
        <div class="col text-end">
          <a href="https://vuejs.org" target="_blank" rel="noreferrer noopener"> <svg viewBox="0 0 128 128" width="24"
              height="24">
              <path fill="#42b883" d="M78.8,10L64,35.4L49.2,10H0l64,110l64-110C128,10,78.8,10,78.8,10z"></path>
              <path fill="#35495e" d="M78.8,10L64,35.4L49.2,10H25.6L64,76l38.4-66H78.8z"></path>
            </svg>
          </a>
          <div class="d-inline mx-1">
          </div>
          <a href="https://getbootstrap.com" target="_blank" rel="noreferrer noopener">
            <svg xmlns="http://www.w3.org/2000/svg" width="30" height="24" viewBox="0 0 118 94" role="img">
              <title>Bootstrap</title>
              <path fill-rule="evenodd" clip-rule="evenodd"
                d="M24.509 0c-6.733 0-11.715 5.893-11.492 12.284.214 6.14-.064 14.092-2.066 20.577C8.943 39.365 5.547 43.485 0 44.014v5.972c5.547.529 8.943 4.649 10.951 11.153 2.002 6.485 2.28 14.437 2.066 20.577C12.794 88.106 17.776 94 24.51 94H93.5c6.733 0 11.714-5.893 11.491-12.284-.214-6.14.064-14.092 2.066-20.577 2.009-6.504 5.396-10.624 10.943-11.153v-5.972c-5.547-.529-8.934-4.649-10.943-11.153-2.002-6.484-2.28-14.437-2.066-20.577C105.214 5.894 100.233 0 93.5 0H24.508zM80 57.863C80 66.663 73.436 72 62.543 72H44a2 2 0 01-2-2V24a2 2 0 012-2h18.437c9.083 0 15.044 4.92 15.044 12.474 0 5.302-4.01 10.049-9.119 10.88v.277C75.317 46.394 80 51.21 80 57.863zM60.521 28.34H49.948v14.934h8.905c6.884 0 10.68-2.772 10.68-7.727 0-4.643-3.264-7.207-9.012-7.207zM49.948 49.2v16.458H60.91c7.167 0 10.964-2.876 10.964-8.281 0-5.406-3.903-8.178-11.425-8.178H49.948z"
                fill="currentColor"></path>
            </svg>
          </a>
        </div>
      </div>
    </Modal>
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