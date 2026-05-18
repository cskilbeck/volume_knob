// Fake HIDDevice — implements just enough of the WebHID surface that hid.js
// consumes (sendReport, addEventListener/removeEventListener via EventTarget,
// productName / vendorId / productId / opened) to drive the configurator with
// no hardware attached.

export function make_dummy_hid_device({
    default_config,
    bytes_from_config,
    hid_custom_command: cmd,
    hid_custom_response: resp,
}) {

    let stored = Array.from(bytes_from_config(default_config));

    class DummyHIDDevice extends EventTarget {
        constructor() {
            super();
            this.productName = "Demo Tiny USB Knob";
            this.vendorId = 0x16D0;
            this.productId = 0x114B;
            this.opened = true;
        }

        async open() { }
        async close() { }

        async sendReport(reportId, data) {
            const bytes = data instanceof Uint8Array ? data : new Uint8Array(data);
            const op = bytes[0];

            if (op === cmd.hcc_get_config) {
                this._reply([resp.hcc_here_is_config, ...stored]);
            } else if (op === cmd.hcc_get_firmware_version) {
                // hid.js reads b3=data[4] as the major; firmware_version_str = `${b3}.${b2}.${b1}.${b0}`.
                // [resp, 0, 0, 0, 2] yields firmware_major=2 → routes to HIDDevice_2.vue, str "2.0.0.0".
                this._reply([resp.hcc_here_is_firmware_version, 0, 0, 0, 2]);
            } else if (op === cmd.hcc_flash_led) {
                this._reply([resp.hcc_led_flashed]);
            } else if (op === cmd.hcc_set_config) {
                stored = Array.from(bytes.slice(1));
                this._reply([resp.hcc_set_config_ack]);
            }
            // hcc_goto_bootloader: no-op for demo.
        }

        _reply(bytes) {
            // hid.js handler reads e.device, e.reportId, and `new Uint8Array(e.data.buffer)`.
            // Construct an Event with those three fields patched on.
            setTimeout(() => {
                const ev = new Event("inputreport");
                ev.device = this;
                ev.reportId = 0;
                ev.data = new DataView(new Uint8Array(bytes).buffer);
                this.dispatchEvent(ev);
            }, 0);
        }
    }

    return new DummyHIDDevice();
}
