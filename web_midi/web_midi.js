//////////////////////////////////////////////////////////////////////

let midiIn = [];
let midiOut = [];

//////////////////////////////////////////////////////////////////////

function refresh() {
  navigator.requestMIDIAccess({ "sysex": true })
    .then(
      (midi) => midiReady(midi),
      (err) => console.log('Something went wrong', err));
}

//////////////////////////////////////////////////////////////////////

function midiReady(midi) {
  // Also react to device changes.
  midi.addEventListener('statechange', (event) => initDevices(event.target));
  initDevices(midi); // see the next section!
}

//////////////////////////////////////////////////////////////////////

function initDevices(midi) {

  console.log("Init MIDI Devices");
  // Reset.
  midiIn = [];
  midiOut = [];

  // MIDI devices that send you data.
  const inputs = midi.inputs.values();
  for (let input = inputs.next(); input && !input.done; input = inputs.next()) {
    midiIn.push(input.value);
  }
  console.log(`Found ${midiIn.length} input MIDI Devices`);

  // MIDI devices that you send data to.
  const outputs = midi.outputs.values();
  for (let output = outputs.next(); output && !output.done; output = outputs.next()) {
    midiOut.push(output.value);
  }

  console.log(`Found ${midiOut.length} output MIDI Devices`);

  displayDevices();
  startListening();
}

//////////////////////////////////////////////////////////////////////
// Start listening to MIDI messages.

function startListening() {
  for (const input of midiIn) {
    input.addEventListener('midimessage', midiMessageReceived);
  }
}

//////////////////////////////////////////////////////////////////////

function midiMessageReceived(event) {

  const timestamp = Date.now();

  console.log(`Got midi messge, ${event.data.length} long`);

  for (let x in event.data) {
    console.log(`${event.data[x].toString(16)}`);
  }
}

//////////////////////////////////////////////////////////////////////

function flash(device_index) {
  let device = midiOut[device_index];
  console.log(`Device ${device.name} flashes`);
  device.send([0xF0, 0x7D, 114, 0xF7]);
}

//////////////////////////////////////////////////////////////////////

function displayDevices() {
  let devices_div = document.getElementById("devices");
  let devices = `Found ${midiIn.length} input devices<br>`;
  let index = 0;
  for (const device of midiOut) {
    devices += `<div>
                  <span>${device.name}</span>
                  <button id='flash_button' onclick='flash(${index});'>Flash</button>
                </div>`;
    index += 1;
  }
  devices_div.innerHTML = devices;
}

