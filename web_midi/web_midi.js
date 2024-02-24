"use strict";

//////////////////////////////////////////////////////////////////////

const cc_list = {
  0: "Bank Select",
  1: "Modulation Wheel",
  2: "Breath controller",
  4: "Foot Pedal",
  5: "Portamento Time",
  6: "Data Entry",
  7: "Volume",
  8: "Balance",
  10: "Pan position",
  11: "Expression",
  12: "Effect Control 1",
  13: "Effect Control 2",
  64: "Hold Pedal (on/off)",
  65: "Portamento (on/off)",
  66: "Sostenuto Pedal (on/off)",
  67: "Soft Pedal (on/off)",
  68: "Legato Pedal (on/off)",
  69: "Hold 2 Pedal (on/off)",
  70: "Sound Variation",
  71: "Resonance (Timbre)",
  72: "Sound Release Time",
  73: "Sound Attack Time",
  74: "Frequency Cutoff (Brightness)",
  75: "Sound Control 6",
  76: "Sound Control 7",
  77: "Sound Control 8",
  78: "Sound Control 9",
  79: "Sound Control 10",
  80: "Decay or General Purpose Button 1 (on/off) Roland Tone level 1",
  81: "Hi Pass Filter Frequency or General Purpose Button 2 (on/off) Roland Tone level 2",
  82: "General Purpose Button 3 (on/off) Roland Tone level 3",
  83: "General Purpose Button 4 (on/off) Roland Tone level 4",
  84: "Portamento Amount",
  91: "Reverb Level",
  92: "Tremolo Level",
  93: "Chorus Level",
  94: "Detune Level",
  95: "Phaser Level",
  96: "Data Button increment",
  97: "Data Button decrement",
  120: "All Sound Off",
  121: "All Controllers Off",
  122: "Local Keyboard (on/off)",
  123: "All Notes Off",
  124: "Omni Mode Off",
  125: "Omni Mode On",
  126: "Mono Operation",
  127: "Poly Mode"
};

//////////////////////////////////////////////////////////////////////

let midi = null;
let midi_devices = []
let midi_id = 1;

let devices_div = null;
let status_div = null;

//////////////////////////////////////////////////////////////////////
// expand some bytes into an array of 7 bit values

function bytes_to_bits7(src_data, offset, len, dest) {

  let bits_available = 0;
  let cur_src = 0;
  let bits_remaining = len * 8;

  while (true) {

    if (bits_available < 7) {
      cur_src = (cur_src << 8) | src_data[offset++];
      bits_available += 8;
    }

    while (bits_available >= 7) {
      bits_available -= 7;
      dest.push((cur_src >> bits_available) & 0x7f);
      bits_remaining -= 7;
    }

    if (bits_remaining < 7) {
      dest.push((cur_src << (7 - bits_remaining)) & 0x7f);
      break;
    }
  }
}

//////////////////////////////////////////////////////////////////////
// convert some 7 bit values to bytes

function bits7_to_bytes(src_data, offset, len, dest) {

  let bits_available = 0;
  let cur_src = 0;

  while (dest.length < len) {

    if (bits_available < 8) {
      cur_src = (cur_src << 7) | src_data[offset++];
      bits_available += 7;
    }

    while (bits_available >= 8) {
      bits_available -= 8;
      dest.push((cur_src >> bits_available) & 0xff);
    }
  }
}

//////////////////////////////////////////////////////////////////////

function on_page_load() {

  devices_div = document.getElementById("devices");
  status_div = document.getElementById("status");

  navigator.requestMIDIAccess({ "sysex": true })
    .then(
      (midi_object) => {
        midi = midi_object;
        initDevices();
      },
      (err) => {
        console.log('requestMIDIAccess failed: ', err);
      }
    );
}

//////////////////////////////////////////////////////////////////////

function initDevices() {

  midi_devices = []
  midi_id = 1;

  midi.addEventListener('statechange', function (event) {
    initDevices();
  });

  devices_div.innerHTML = "";

  for (const input of midi.inputs.values()) {
    input.onmidimessage = function (event) {
      on_midi_message(input, event);
    };
  }

  for (const output of midi.outputs.values()) {
    midi_devices[midi_id] = {
      midi_ident: midi_id,
      midi_serial_number: 0,
      midi_input: null,
      midi_output: output,
      midi_hex: ""
    };
    output.send([0xF0, 0x7E, midi_id & 0xff, 0x06, 0x01, 0xF7]);
    midi_id += 1;
  }
}

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

const MIDI_MANUFACTURER_ID = 0x36;    // Cheetah Marketing, defunct?
const MIDI_FAMILTY_CODE_LOW = 0x55;
const MIDI_FAMILTY_CODE_HIGH = 0x44;
const MIDI_MODEL_NUMBER_LOW = 0x33;
const MIDI_MODEL_NUMBER_HIGH = 0x22;

const sysex_header = [0xF0, 0x7E];

const id_response = [
  0xF0, 0x7E, 0x00, 0x06, 0x02,
  MIDI_MANUFACTURER_ID,
  MIDI_FAMILTY_CODE_LOW,
  MIDI_FAMILTY_CODE_HIGH,
  MIDI_MODEL_NUMBER_LOW,
  MIDI_MODEL_NUMBER_HIGH];
// bytes following are 4 x 7bits = 28 bit serial number, then F7 terminator

const memory_response = [0xF0, 0x7E, 0x00, 0x06, 0x03];
// byte following is offset
// byte following is length
// bytes following is flash contents, 7 bits per byte, 

//////////////////////////////////////////////////////////////////////

function is_sysex_message(data) {
  if (compare_array_sections(data, sysex_header, 0, 2) && data[3] == 0x06) {
    return data[2];
  }
  return undefined;
}

//////////////////////////////////////////////////////////////////////

function handle_new_device(device, data) {
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
        <div class = 'device_container'>
          <div class = 'device_row'>
            <div class = 'device_name'>
              ${device.name}&nbsp;${d.midi_serial_number.toString(16).toUpperCase()}
            </div>
            <div class = 'device_ui'>
              <div class = 'device_buttons'>
                <button onclick='toggle_device_led(${i})'>Toggle LED</button>
                <button onclick='read_flash(${i})'>Load</button>
                <button onclick='write_flash(${i})'>Save</button>
              </div>
              <div class='device_controls'>
                <input class = 'memory_input' id='memory_${i}' oninput='on_input_change(this, ${i})'></input>
                <span class = 'memory_input' id='memory_contents_${i}'></span>
              </div>
            </div>
          </div>
        </div>
        `;

      console.log(`Midi device ${device.name} at ${device.id} has serial # 0x${d.midi_serial_number.toString(16)} (id ${d.midi_ident})`);
    }
  }
}

//////////////////////////////////////////////////////////////////////

function read_flash(index) {
  const device = midi_devices[index];
  if (device == undefined || device == null) {
    console.log(`load_device: No such device ${index}`);
    return;
  }
  device.midi_output.send([0xF0, 0x7E, 0x00, 0x06, 0x03, 0xF7]);
}

//////////////////////////////////////////////////////////////////////

function write_flash(index) {
  const device = midi_devices[index];
  if (device == undefined || device == null) {
    console.log(`load_device: No such device ${index}`);
    return;
  }
  let data_7bits = [];
  bytes_to_bits7(device.midi_hex, 0, 8, data_7bits);
  let msg = [0xF0, 0x7E, 0x00, 0x06, 0x04].concat(data_7bits).concat([0xF7]);
  device.midi_output.send(msg);
}

//////////////////////////////////////////////////////////////////////
// Convert a hex string to a byte array

function hexToBytes(hex) {
  let bytes = [];
  for (let c = 0; c < hex.length; c += 2)
    bytes.push(parseInt(hex.substr(c, 2), 16));
  return bytes;
}

//////////////////////////////////////////////////////////////////////

function bytes_to_hex_string(data, len) {
  let sep = "";
  let str = "";
  let l = Math.min(len, data.length);
  for (let i = 0; i < l; ++i) {
    str += sep + data[i].toString(16).padStart(2, '0');
    sep = " ";
  }
  return str;
}

//////////////////////////////////////////////////////////////////////

function on_input_change(e, index) {

  const device = midi_devices[index];
  if (device == undefined || device == null) {
    console.log("No such device");
    return;
  }
  let stripped = e.value.replace(/\s+/g, '');
  if (stripped.length == 0 || stripped.length % 2 != 0 || stripped.length > 16) {
    device.midi_hex = null;
    console.log("Not a hex string");
    return;
  }

  device.midi_hex = hexToBytes(stripped);
  document.getElementById(`memory_contents_${index}`).innerHTML = bytes_to_hex_string(device.midi_hex, 8);
  console.log(`Memory for ${device.midi_ident}:${device.midi_hex}`);
}

//////////////////////////////////////////////////////////////////////

function on_midi_message(device, event) {

  const data = event.data;

  console.log(`MIDI MESSAGE: ${data}`);

  let device_id = is_sysex_message(data);
  if (device_id != undefined) {
    switch (data[4]) {

      case 0x02:
        if (compare_array_sections(data, id_response, 5, 10)) {
          handle_new_device(device, data);
          status_div.innerHTML = `Found ${midi_devices.length} device(s)`;
        }
        break;

      case 0x03:
        let num_bits = (event.length - 6) * 7;
        let num_bytes = (num_bits + 7) / 8;
        let bytes = [];
        let total_bits = 0;
        let current_byte = 0;
        let cur = 5;
        for (const i = 5; i < event.length - 5; ++i) {
          let required_bits = 8 - total_bits;
          if (required_bits > 7) {
            required_bits = 7;
          }
          let extraction = data[cur] >> (7 - required_bits);
          current_byte |= extraction << (8 - total_bits);
          total_bits += required_bits;
          if (total_bits == 8) {
            bytes.push(current_byte);
            total_bits = 0;
          }
        }
        break;

      case 0x10:
        let flash_data = [];
        bits7_to_bytes(data, 5, 8, flash_data);
        console.log(`FLASH: ${flash_data}`);
        document.getElementById(`memory_${device_id}`).value = bytes_to_hex_string(flash_data, 8);
        break;

      case 0x11:
        status_div.innerHTML = `Wrote flash data`;
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////
