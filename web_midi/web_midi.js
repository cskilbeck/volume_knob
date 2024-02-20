//////////////////////////////////////////////////////////////////////

"use strict";

//////////////////////////////////////////////////////////////////////

let midi = null;
let midi_devices = []
let midi_id = 1;

let devices_div = null;
let status_div = null;

//////////////////////////////////////////////////////////////////////

function on_page_load() {

  devices_div = document.getElementById("devices");
  status_div = document.getElementById("status");

  navigator.requestMIDIAccess({ "sysex": true })
    .then(

      (midi_object) => {

        midi = midi_object;
        initDevices();
        midi.addEventListener('statechange', function (event) {
          initDevices();
        });
      },

      (err) => {

        console.log('Something went wrong', err);
      }
    );
}

//////////////////////////////////////////////////////////////////////

function initDevices() {

  midi_devices = []
  midi_id = 1;

  devices_div.innerHTML = "";

  for (const input of midi.inputs.values()) {
    input.onmidimessage = function (event) {
      on_midi_message(input, event);
    };
  }

  for (const output of midi.outputs.values()) {
    midi_devices.push({
      midi_ident: midi_id,
      midi_serial_number: 0,
      midi_input: null,
      midi_output: output
    });
    output.send([0xF0, 0x7E, midi_id & 0xff, 0x06, 0x01, 0xF7]);
    midi_id += 1;
  }
}

//////////////////////////////////////////////////////////////////////

const MIDI_MANUFACTURER_ID = 0x36;    // Cheetah Marketing, defunct?
const MIDI_FAMILTY_CODE_LOW = 0x55;
const MIDI_FAMILTY_CODE_HIGH = 0x44;
const MIDI_MODEL_NUMBER_LOW = 0x33;
const MIDI_MODEL_NUMBER_HIGH = 0x22;

const id_response = [
  0xF0, 0x7E, 0x00, 0x06, 0x02,
  MIDI_MANUFACTURER_ID,
  MIDI_FAMILTY_CODE_LOW,
  MIDI_FAMILTY_CODE_HIGH,
  MIDI_MODEL_NUMBER_LOW,
  MIDI_MODEL_NUMBER_HIGH];
// bytes following are 4 x 7bits = 28 bit serial number, then F7 terminator

//////////////////////////////////////////////////////////////////////

function compare_array_sections(a, b, s, e) {
  for (const i = s; i < e; ++i) {
    if (a[i] != b[i]) {
      return false;
    }
    return true;
  }
}

//////////////////////////////////////////////////////////////////////

function toggle_device_led(index) {
  const device = midi_devices[index];
  if (device == undefined || device == null) {
    console.log("No such device");
    return;
  }
  console.log(`Toggle led for device ${device.midi_serial_number.toString(16)}`);

  // MMC play to toggle led
  device.midi_output.send([0xF0, 0x7E, 0x00, 0x06, 0x02, 0xF7]);
}

//////////////////////////////////////////////////////////////////////

function on_midi_message(device, event) {

  const data = event.data;

  // is it an ident response?

  if (data.length == id_response.length + 5 &&
    compare_array_sections(data, id_response, 0, 2) &&
    compare_array_sections(data, id_response, 3, 10)) {

    let reply_id = data[2];

    // yes, find the corresponding midi_device

    for (const i in midi_devices) {

      const d = midi_devices[i];

      if (d.midi_ident === reply_id) {

        // set the serial number

        let b0 = data[10] || 0;
        let b1 = data[11] || 0;
        let b2 = data[12] || 0;
        let b3 = data[13] || 0;

        d.midi_serial_number = b3 | (b2 << 7) | (b1 << 14) | (b0 << 21);

        // and input device

        d.midi_input = device;

        // add it to the page
        devices_div.innerHTML += `
        <div>
          <span>
            <button onclick='toggle_device_led(${i})'>Toggle LED</button>
            Midi device ${device.name} at ${device.id} has serial # 0x${d.midi_serial_number.toString(16)} (id ${d.midi_ident})<br>
          </span>
        </div>
        `;

        console.log(`Midi device ${device.name} at ${device.id} has serial # 0x${d.midi_serial_number.toString(16)} (id ${d.midi_ident})`);
      }
    }
    status_div.innerHTML = `Found ${midi_devices.length} midi device(s)`;
  }
}

//////////////////////////////////////////////////////////////////////
