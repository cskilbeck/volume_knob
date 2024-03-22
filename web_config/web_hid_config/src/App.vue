<script setup>

//////////////////////////////////////////////////////////////////////

import DeviceList from './components/DeviceList.vue'
import DarkMode from './components/DarkMode.vue'
import Modal from './components/Modal.vue'
import hid from './hid.js'
import { ref, onMounted } from 'vue'

const closeableModal = ref(false);
const notSupportedModel = ref(false);

let scan_on_startup = true;

let got_hid_api = ref(false);

got_hid_api.value = navigator.hid || 0;

//////////////////////////////////////////////////////////////////////

function scan_hid() {
  if (got_hid_api.value) {
    hid.init_devices();
  }
}

//////////////////////////////////////////////////////////////////////

</script>

<template>
  <main>

    <div class="container container-wide">
      <div class="row">
        <div class="col-2 pt-4">
          <span v-show='hid.scanned.done'>
            Found {{ Object.keys(hid.hid_devices).length }}
            device{{ Object.keys(hid.hid_devices).length != 1 ? "s" : "" }}
          </span>
        </div>
        <div class="col-8">
          <h5 class="text-center pt-4 pb-3">Tiny USB Knob Configurator</h5>
        </div>
        <div class="col-2 pt-3 text-end">
          <button :disabled="!got_hid_api" class="btn border border-secondary tertiary-bg btn-sm mx-4"
            @click="scan_hid()">
            Scan
          </button>
          <DarkMode />
          <button @click="closeableModal = true" class="btn btn-sm">
            <svg xmlns="http://www.w3.org/2000/svg" width="26" height="26" fill="currentColor" class="bi bi-info-circle"
              viewBox="0 0 16 16">
              <path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14m0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16" />
              <path
                d="m8.93 6.588-2.29.287-.082.38.45.083c.294.07.352.176.288.469l-.738 3.468c-.194.897.105 1.319.808 1.319.545 0 1.178-.252 1.465-.598l.088-.416c-.2.176-.492.246-.686.246-.275 0-.375-.193-.304-.533zM9 4.5a1 1 0 1 1-2 0 1 1 0 0 1 2 0" />
            </svg>
          </button>
        </div>
      </div>
    </div>

    <div class="container container-wide">
      <DeviceList :devices="hid.hid_devices" />
    </div>

    <Modal v-model="closeableModal" maxwidth="30%" closeable header="It's a Configurator for the Tiny USB Knob">
      <div class="row">
        <div class="col my-3 text-center">
          Get your own Tiny USB Knob <a href="https://www.etsy.com/uk/shop/TinyLittleGadgets" target="_blank"
            rel="noreferrer noopener">here</a>
        </div>
      </div>
      <div class="row mt-3">
        <div class="col-3 text-left">
        </div>
        <div class="col-6 text-center">
          Crafted with care by <a class="" href="https://twitter.com/cskilbeck" target="_blank"
            rel="noreferrer noopener">@cskilbeck</a>
        </div>
        <div class="col-3 text-end">
          <a href="https://github.com/cskilbeck/volume_knob" target="_blank" rel="noreferrer noopener"
            class="link-body-emphasis">
            <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
              <path
                d="M12 0c-6.626 0-12 5.373-12 12 0 5.302 3.438 9.8 8.207 11.387.599.111.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23.957-.266 1.983-.399 3.003-.404 1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.911 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576 4.765-1.589 8.199-6.086 8.199-11.386 0-6.627-5.373-12-12-12z" />
            </svg>
          </a>
          <div class="d-inline mx-1">
          </div>
          <a href="https://vuejs.org" target="_blank" rel="noreferrer noopener"> <svg viewBox="0 0 128 128" width="24"
              height="24">
              <path fill="#42b883" d="M78.8,10L64,35.4L49.2,10H0l64,110l64-110C128,10,78.8,10,78.8,10z"></path>
              <path fill="#35495e" d="M78.8,10L64,35.4L49.2,10H25.6L64,76l38.4-66H78.8z"></path>
            </svg>
          </a>
          <div class="d-inline mx-1">
          </div>
          <a href="https://getbootstrap.com" target="_blank" rel="noreferrer noopener" class="link-body-emphasis">
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

    <Modal v-model="notSupportedModel" maxwidth="30%" closeable header="Web MIDI API not found">
      <div class="row">
        <div class="col text-center">
          I'm afraid your browser doesn't support the Web MIDI API
        </div>
      </div>
      <div class="row mt-3">
        <div class="col text-center">
          Find out which browsers will work <a href="https://caniuse.com/midi" target="_blank"
            rel="noreferrer noopener">here</a>
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