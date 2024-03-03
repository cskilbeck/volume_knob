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
      midi.on_midi(main_midi_object);
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
          <span>
            Found {{ midi.midi_devices.value.length }} device{{ midi.midi_devices.value.length != 1 ? "s" : "" }}
          </span>
        </div>
        <div class="col-8 text-center">
          <button class="btn border tertiary-bg btn-sm" @click="midi.init_devices()">
            Scan
          </button>
        </div>
        <div class="col-2 text-end">
          <DarkMode />
          <button @click="closeableModal = true" class="btn tertiary-bg border btn-sm">
            About
          </button>
          <Modal v-model="closeableModal" maxwidth="30%" closeable
            header="It's a Configurator for the Tiny USB Midi Knob">
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
            <div class="row mt-5">
              <div class="col text-end">
                <a href="https://vuejs.org/" target="_blank" rel="noreferrer noopener">Vue.js</a> / <a
                  href="https://getbootstrap.com/" target="_blank" rel="noreferrer noopener">Bootstrap</a>
              </div>
            </div>
          </Modal>
        </div>
      </div>
      <DeviceList :devices="midi.midi_devices" />
    </div>
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
</style>