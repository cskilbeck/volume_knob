// Fake MIDI ports — implement the WebMIDI surface that Midi.js consumes
// (output.send, input.add/removeEventListener via EventTarget, properties
// name / id / state / type / close) so the configurator can be driven
// without USB hardware.

export function make_dummy_midi_ports({
    default_config,
    bytes_from_config,
    bytes_to_bits7,
    bits7_to_bytes,
    CONFIG_LEN,
    MIDI_MANUFACTURER_ID,
    MIDI_FAMILY_CODE_LOW,
    MIDI_FAMILY_CODE_HIGH,
    MIDI_MODEL_NUMBER_LOW,
    MIDI_MODEL_NUMBER_HIGH,
    sysex_request_device_id,
    sysex_request_toggle_led,
    sysex_request_get_flash,
    sysex_request_set_flash,
    sysex_request_bootloader,
    sysex_response_device_id,
    sysex_response_get_flash,
    sysex_response_set_flash_ack,
}) {
    const name = "Demo Tiny MIDI Knob";

    let stored_bytes = bytes_from_config(default_config);
    let dev_idx = 0;

    class DummyMIDIPort extends EventTarget {
        constructor(type, id) {
            super();
            this.name = name;
            this.id = id;
            this.state = "connected";
            this.connection = "open";
            this.type = type;
            this.manufacturer = "Demo";
        }
        close() { /* no-op */ }
        open() { return Promise.resolve(this); }
    }

    const input = new DummyMIDIPort("input", "demo-midi-in");
    const output = new DummyMIDIPort("output", "demo-midi-out");

    function dispatch_midi(reply_bytes) {
        setTimeout(() => {
            const data = new Uint8Array(reply_bytes);
            let ev;
            try {
                ev = new MIDIMessageEvent("midimessage", { data });
            } catch (_) {
                ev = new Event("midimessage");
                ev.data = data;
            }
            input.dispatchEvent(ev);
        }, 0);
    }

    output.send = (bytes) => {
        // Only handle SysEx: F0 7E XX 06 cmd ... F7
        if (bytes[0] !== 0xF0 || bytes[1] !== 0x7E || bytes[3] !== 0x06) return;
        const cmd = bytes[4];

        switch (cmd) {
            case sysex_request_device_id:
                dispatch_midi([
                    0xF0, 0x7E, dev_idx, 0x07, sysex_response_device_id,
                    MIDI_MANUFACTURER_ID,
                    MIDI_FAMILY_CODE_LOW, MIDI_FAMILY_CODE_HIGH,
                    MIDI_MODEL_NUMBER_LOW, MIDI_MODEL_NUMBER_HIGH,
                    // Midi.js reads b0..b3 from data[10..13]; str = `${b3}.${b2}.${b1}.${b0}`.
                    // [0,0,0,2] yields "2.0.0.0".
                    0, 0, 0, 2,
                    0xF7
                ]);
                break;

            case sysex_request_get_flash: {
                const packed = bytes_to_bits7(stored_bytes, 0, CONFIG_LEN);
                dispatch_midi([
                    0xF0, 0x7E, dev_idx, 0x07, sysex_response_get_flash,
                    ...Array.from(packed),
                    0xF7
                ]);
                break;
            }

            case sysex_request_set_flash: {
                const u8 = new Uint8Array(bytes);
                const unpacked = bits7_to_bytes(u8, 5, CONFIG_LEN);
                if (unpacked) stored_bytes = unpacked;
                dispatch_midi([
                    0xF0, 0x7E, dev_idx, 0x07, sysex_response_set_flash_ack, 0xF7
                ]);
                break;
            }

            case sysex_request_toggle_led:
            case sysex_request_bootloader:
                // No-op for demo.
                break;
        }
    };

    function set_device_index(idx) { dev_idx = idx; }

    function emit_cc(channel, cc, value) {
        const bytes = new Uint8Array([
            0xB0 | (channel & 0x0f),
            cc & 0x7f,
            value & 0x7f,
        ]);
        let ev;
        try {
            ev = new MIDIMessageEvent("midimessage", { data: bytes });
        } catch (_) {
            ev = new Event("midimessage");
            ev.data = bytes;
        }
        input.dispatchEvent(ev);
    }

    return { input, output, set_device_index, emit_cc };
}
