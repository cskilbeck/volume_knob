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

const FLASH_MAX_LEN = 26;

// main webmidi object
let midi = null;

// map of output port ids (e.g output-0) to midi_device objects
let midi_map = new Map();

// map of device_indices (start at 1) to output port ids
let index_map = new Map();

// next midi device index
let midi_index = 1;

// div where devices go
let devices_div = null;

// div where status messages go
let status_div = null;

// keep last N status messages
let status_messages = [];

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

function add_status_message(msg) {
  const max_messages = 4;
  status_messages.push(msg);
  if (status_messages.length > max_messages) {
    status_messages = status_messages.slice(1, max_messages + 1);
  }
  let status = "";
  let sep = "";
  for (const s of status_messages) {
    status = `${status}${sep}${s}`;
    sep = "<br>";
  }
  status_div.innerHTML = status;
}

//////////////////////////////////////////////////////////////////////

function on_page_load() {

  devices_div = document.getElementById("devices");
  status_div = document.getElementById("status");

  navigator.requestMIDIAccess({ "sysex": true })
    .then(
      (midi_object) => {
        midi = midi_object;
        init_devices();
        midi.addEventListener('statechange', function (event) {
          init_devices();
        });

      },
      (err) => {
        add_status_message(`requestMIDIAccess failed: ${err}`);
        console.log(`requestMIDIAccess failed: ${err}`);
      }
    );
}

//////////////////////////////////////////////////////////////////////

function send_midi(midi_device, data) {
  add_status_message(`SEND: ${bytes_to_hex_string(data, data.length, "")}`);
  midi_device.midi_output.send(data);
}

//////////////////////////////////////////////////////////////////////

function init_devices() {

  console.log(`init_devices`);

  midi_map = new Map();
  index_map = new Map();
  midi_index = 1;

  devices_div.innerHTML = "";

  console.log(`${midi.inputs.size} inputs`);
  console.log(`${midi.outputs.size} outputs`);

  for (const input of midi.inputs.values()) {
    input.onmidimessage = function (event) {
      on_midi_message(input, event);
    };
  }

  for (const output of midi.outputs.values()) {
    let existing = midi_map.get(output.id);
    if (existing == undefined) {
      console.log(`New device: ${output.id}`);
      let midi_device = {
        midi_index: midi_index,
        midi_serial_number: 0,
        midi_input: null,
        midi_output: output,
        midi_hex: []
      };
      midi_map.set(output.id, midi_device);

      // for looking up the device when the response arrives
      index_map.set(midi_index, output.id);
      add_status_message(`Scanning device at ${output.id}`);

      send_midi(midi_device, [0xF0, 0x7E, midi_index & 0xff, 0x06, 0x01, 0xF7]);

      midi_index += 1;
    }
  }
  console.log(`init devices scanned ${midi_map.size} devices`);
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

function get_device_from_index(index) {
  const object_id = index_map.get(index);
  if (object_id == undefined) {
    console.log("No such device");
    return;
  }
  return midi_map.get(object_id);
}

//////////////////////////////////////////////////////////////////////

function toggle_device_led(index) {
  const device = get_device_from_index(index);
  if (device === undefined) {
    add_status_message(`Can't find device ${index}`);
  } else {
    add_status_message(`Toggle led for device ${device.midi_serial_number.toString(16).toUpperCase()}`);
    send_midi(device, [0xF0, 0x7E, 0x00, 0x06, 0x02, 0xF7]);
  }
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

function get_sysex_device_index(data) {
  if (compare_array_sections(data, sysex_header, 0, 2) && data[3] == 0x06 && data[data.length - 1] == 0xF7) {
    return data[2];
  }
  return undefined;
}

//////////////////////////////////////////////////////////////////////

function handle_new_device(device, data) {

  let reply_index = data[2];

  // yes, find the corresponding midi_device

  let d = get_device_from_index(reply_index);
  if (d == undefined) {
    add_status_message(`Midi from unknown device: ${device.name}`);
    return;
  }

  // set the serial number

  let b0 = data[10] || 0;
  let b1 = data[11] || 0;
  let b2 = data[12] || 0;
  let b3 = data[13] || 0;

  d.midi_serial_number = b3 | (b2 << 7) | (b1 << 14) | (b0 << 21);
  d.midi_input = device;

  let new_serial_number = d.midi_serial_number.toString(16).toUpperCase();
  add_status_message(`Found device ${device.name}, serial # ${new_serial_number}`);
  console.log(`Midi device ${device.name} at ${device.id} has serial # 0x${new_serial_number} (id ${d.midi_index})`);

  // and input device

  let str = "";
  for (const midi_device of midi_map.values()) {
    let name = midi_device.midi_output.name;
    let serial_number = midi_device.midi_serial_number.toString(16).toUpperCase();
    let midi_index = midi_device.midi_index;

    str += `
    <div class = 'device_container'>
      <div class = 'device_row'>
        <div class = 'device_name'>
          ${name}&nbsp;${serial_number}
        </div>
        <div class = 'device_ui'>
          <div class = 'device_buttons'>
            <button onclick='toggle_device_led(${midi_index})'>Toggle LED</button>
            <button onclick='read_flash(${midi_index})'>Load</button>
            <button onclick='write_flash(${midi_index})'>Save</button>
          </div>
          <div class='device_controls'>
            <textarea rows='2' cols='40' class='memory_input' id='memory_${midi_index}' oninput='on_input_change(this, ${midi_index})'></textarea>
            <textarea readonly rows='2' cols='40' class = 'memory_contents' id='memory_contents_${midi_index}'></textarea>
          </div>
        </div>
      </div>
    </div>
    `;
  }
  // add it to the page
  devices_div.innerHTML = str;
  add_status_message(`Found ${midi_map.size} device(s)`);
}

//////////////////////////////////////////////////////////////////////

function read_flash(index) {
  const device = get_device_from_index(index);
  if (device === undefined) {
    console.log(`load_device: No such device ${index}`);
    return;
  }
  send_midi(device, [0xF0, 0x7E, 0x00, 0x06, 0x03, 0xF7]);
}

//////////////////////////////////////////////////////////////////////

function write_flash(index) {
  const midi_device = get_device_from_index(index);
  if (midi_device === undefined) {
    console.log(`load_device: No such device ${index}`);
    return;
  }
  let data_7bits = [];
  bytes_to_bits7(midi_device.midi_hex, 0, FLASH_MAX_LEN, data_7bits);
  let msg = [0xF0, 0x7E, 0x00, 0x06, 0x04].concat(data_7bits).concat([0xF7]);
  send_midi(midi_device, msg);
}

//////////////////////////////////////////////////////////////////////
// Convert a hex string to a byte array

function hex_to_bytes(hex) {
  let bytes = [];
  for (let c = 0; c < hex.length; c += 2) {
    bytes.push(parseInt(hex.substr(c, 2), 16));
  }
  while (bytes.length < FLASH_MAX_LEN) {
    bytes.push(0xFF);
  }
  return bytes;
}

//////////////////////////////////////////////////////////////////////

function bytes_to_hex_string(data, len, separator) {
  if (separator == undefined) {
    separator = " ";
  }
  let sep = "";
  let str = "";
  if (len === undefined) {
    len = data.length;
  }
  let l = Math.min(len, data.length);
  for (let i = 0; i < l; ++i) {
    str += sep + data[i].toString(16).toUpperCase().padStart(2, '0');
    sep = separator;
  }
  return str;
}

//////////////////////////////////////////////////////////////////////

function on_input_change(e, index) {

  const device = get_device_from_index(index);
  if (device === undefined) {
    console.log("No such device");
    return;
  }
  let stripped = e.value.replace(/\s+/g, '');
  if (stripped.length == 0 || stripped.length % 2 != 0 || stripped.length > (FLASH_MAX_LEN * 2)) {
    device.midi_hex = [];
    console.log("Not a hex string");
    return;
  }

  device.midi_hex = hex_to_bytes(stripped);
  let hex_str = bytes_to_hex_string(device.midi_hex, FLASH_MAX_LEN);
  document.getElementById(`memory_contents_${index}`).innerHTML = hex_str;
  console.log(`Memory for device ${device.midi_serial_number.toString(16).toUpperCase()}: ${hex_str}`);
}

//////////////////////////////////////////////////////////////////////

function is_null_or_undefined(x) {
  return x === null || x === undefined;
}

//////////////////////////////////////////////////////////////////////

function on_midi_message(device, event) {

  const data = event.data;

  console.log(`MIDI MESSAGE: ${data}`);
  let hex_str = bytes_to_hex_string(data, data.length, "");
  add_status_message(`RECV: ${hex_str}`);

  let device_index = get_sysex_device_index(data);

  if (device_index != undefined) {

    switch (data[4]) {

      // device ID response
      case 0x02:
        // if it's a Tiny Midi Knob
        if (compare_array_sections(data, id_response, 5, 10)) {
          // add it to the list
          handle_new_device(device, data);
        }
        break;

      // read flash memory response
      case 0x3:
        let midi_device = get_device_from_index(device_index);
        if (midi_device !== undefined) {
          let flash_data = [];
          bits7_to_bytes(data, 5, FLASH_MAX_LEN, flash_data);
          console.log(`FLASH: ${flash_data}`);
          midi_device.midi_hex = flash_data;
          let s = bytes_to_hex_string(flash_data, FLASH_MAX_LEN);
          document.getElementById(`memory_${device_index}`).value = s;
          document.getElementById(`memory_contents_${device_index}`).innerHTML = s;
          add_status_message(`Memory for ${midi_device.midi_serial_number.toString(16).toUpperCase()}: ${s}`);
        }
        break;

      // write flash memory ACK
      case 0x4:
        add_status_message(`Wrote flash data`);
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////
