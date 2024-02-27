<script setup>

//////////////////////////////////////////////////////////////////////

import TestComponent from './components/TestComponent.vue'
import 'bootstrap/dist/css/bootstrap.min.css'
import midi from './components/Midi.js'

//////////////////////////////////////////////////////////////////////

navigator.requestMIDIAccess({ "sysex": true })
  .then(
    async (main_midi_object) => {
      midi.on_midi(main_midi_object);
    },
    (err) => {
      midi.add_status_message(`requestMIDIAccess failed: ${err}`);
      console.log(`requestMIDIAccess failed: ${err}`);
    }
  );

</script>

<template>
  <main>
    <div class="container text-center p-3">
      <header>
        <img alt="Vue logo" class="logo" src="./assets/logo.svg" width="32" height="32" />
      </header>
      <h5>Tiny USB MIDI Knob Configurator</h5>
      <button class="btn btn-info btn-sm" @click="midi.init_devices()">Scan</button>
    </div>
    <div class="container">
      <TestComponent :devices="midi.midi_devices" />
    </div>
  </main>
</template>
