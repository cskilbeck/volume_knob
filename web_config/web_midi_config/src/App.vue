<script setup>

//////////////////////////////////////////////////////////////////////

import DeviceList from './components/DeviceList.vue'
import 'bootstrap/dist/css/bootstrap.min.css'
import midi from './components/Midi.js'
import Modal from './components/Modal.vue'
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
      <h5 class="text-center mt-3">Tiny USB MIDI Knob Configurator</h5>
    </header>
    <div class="container">
      <div class="row mb-3">
        <div class="col">
          Found {{ midi.midi_devices.value.length }} device{{ midi.midi_devices.value.length != 1 ? "s" : "" }}
        </div>
        <div class="col text-center">
          <button class="btn btn-info btn-sm" @click="midi.init_devices()">
            Scan
          </button>
        </div>
        <div class="col text-end">
          <button @click="closeableModal = true" class="btn btn-secondary btn-sm">
            Info
          </button>
          <Modal v-model="closeableModal" maxwidth="40%" closeable
            header="It's a Configurator for the Tiny USB Midi Knob">
            <p class="text-center">Made by <a href="https://twitter.com/cskilbeck" target="_blank"
                rel="noreferrer noopener">@cskilbeck</a><br>
              On 27/02/2024<br>
              Using <a href="https://vuejs.org/" target="_blank" rel="noreferrer noopener">Vue.js</a><br>
              And <a href="https://getbootstrap.com/" target="_blank" rel="noreferrer noopener">Bootstrap</a></p>
          </Modal>
        </div>
      </div>
      <DeviceList :devices="midi.midi_devices" />
    </div>
  </main>
</template>
