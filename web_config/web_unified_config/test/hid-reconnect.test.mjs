// Regression test for the 2026-09-30 report: a knob that paired fine but never
// appeared in the UI.
//
// Two faults, reproduced here against the real hid.js with a faked WebHID:
//
//   1. init_known_devices() filtered on vendor/product only, so on Windows -
//      where Chrome enumerates a device once per top-level collection - it also
//      picked up the knob's KEYBOARD collection, opened it, and had its writes
//      rejected (NotAllowedError). That left a device in hid_devices that could
//      never answer.
//
//   2. get_config and get_firmware_version go out back to back with no
//      request/response correlation. The device answered the first and dropped
//      the second, and since DeviceList.vue renders nothing while
//      firmware_major is null, a working knob stayed invisible with no error.
//
// Run: node --test test/hid-reconnect.test.mjs   (or: npm run test:hid)

import { test } from 'node:test';
import assert from 'node:assert/strict';

import hid from '../src/hid/hid.js';

const CMD_GET_CONFIG = 1, CMD_GET_VERSION = 2;
const RSP_CONFIG = 1, RSP_VERSION = 2;

const KEYBOARD = [{ usagePage: 0x01, usage: 0x06 }];
const VENDOR = [{ usagePage: 0xFF00, usage: 1 }, { usagePage: 0x0C, usage: 1 }];

class FakeDevice extends EventTarget {
    constructor({ name, collections, drop_version = 0 }) {
        super();
        this.productName = name;
        this.vendorId = 0x16D0;
        this.productId = 0x114B;
        this.collections = collections;
        this.opened = false;
        this.writes = 0;
        this.version_asks = 0;
        this.drop_version = drop_version;
    }
    async open() { this.opened = true; }
    async close() { this.opened = false; }

    async sendReport(id, data) {
        this.writes++;
        const op = new Uint8Array(data)[0];

        // Chrome refuses writes to a protected (keyboard/mouse) collection.
        if (!this.collections.some(c => c.usagePage === 0xFF00 && c.usage === 1)) {
            throw new Error('NotAllowedError: Failed to write the report.');
        }
        if (op === CMD_GET_CONFIG) {
            this.#reply([RSP_CONFIG, 0x82, ...new Array(24).fill(0)]);
        }
        if (op === CMD_GET_VERSION) {
            this.version_asks++;
            if (this.version_asks > this.drop_version) {
                this.#reply([RSP_VERSION, 0, 0, 0, 2]);
            }
        }
    }
    #reply(bytes) {
        setTimeout(() => {
            const ev = new Event('inputreport');
            ev.device = this;
            ev.reportId = 0;
            ev.data = new DataView(new Uint8Array(bytes).buffer);
            this.dispatchEvent(ev);
        }, 0);
    }
}

function install(devices) {
    Object.defineProperty(globalThis, 'navigator', {
        value: {
            hid: {
                getDevices: async () => devices,
                addEventListener() { }, removeEventListener() { },
            }
        },
        configurable: true, writable: true,
    });
    // Each test starts from an empty device list.
    for (const name of Object.keys(hid.hid_devices.value)) {
        delete hid.hid_devices.value[name];
    }
}

test('the keyboard collection is skipped, never opened or written to', async () => {
    const ghost = new FakeDevice({ name: 'Tiny USB Knob', collections: KEYBOARD });
    const real = new FakeDevice({ name: 'Tiny USB Knob BC15FCC5', collections: VENDOR });
    install([ghost, real]);

    await hid.init_known_devices();

    assert.equal(ghost.opened, false, 'ghost must not be opened');
    assert.equal(ghost.writes, 0, 'ghost must never be written to');
    assert.ok(!('Tiny USB Knob' in hid.hid_devices.value), 'ghost must not reach hid_devices');
    assert.ok('Tiny USB Knob BC15FCC5' in hid.hid_devices.value, 'real device must be present');
});

test('a dropped firmware-version reply is retried until it lands', async () => {
    const real = new FakeDevice({ name: 'knob', collections: VENDOR, drop_version: 2 });
    install([real]);

    await hid.init_known_devices();
    const d = hid.hid_devices.value['knob'];

    assert.equal(d.firmware_major, 2, 'version must be recovered');
    assert.equal(d.firmware_version_str, '2.0.0.0');
    assert.equal(real.version_asks, 3, 'one initial ask plus two retries');
    assert.equal(d.identifying, false, 'retry loop must finish');
});

test('a healthy device is not retried at all', async () => {
    const real = new FakeDevice({ name: 'knob', collections: VENDOR, drop_version: 0 });
    install([real]);

    await hid.init_known_devices();
    const d = hid.hid_devices.value['knob'];

    assert.equal(d.firmware_major, 2);
    assert.equal(real.version_asks, 1, 'no redundant traffic when the device answers');
});

test('a device that never answers gives up rather than looping forever', async () => {
    const real = new FakeDevice({ name: 'knob', collections: VENDOR, drop_version: 99 });
    install([real]);

    await hid.init_known_devices();
    const d = hid.hid_devices.value['knob'];

    assert.equal(d.firmware_major, null);
    assert.equal(real.version_asks, 5, 'one ask plus four retries, then stop');
    assert.equal(d.identifying, false, 'retry loop must finish');
    // The UI shows the pending panel while identifying and the failure state
    // once this flips - never an empty page.
    assert.equal(d.identify_failed, true, 'failure must be visible to the UI');
});

test('Try again recovers a device that has given up', async () => {
    const real = new FakeDevice({ name: 'knob', collections: VENDOR, drop_version: 99 });
    install([real]);

    await hid.init_known_devices();
    const d = hid.hid_devices.value['knob'];
    assert.equal(d.identify_failed, true, 'precondition: it gave up');

    // The device starts behaving - a replug, a different port, or just luck.
    real.drop_version = 0;
    real.version_asks = 0;

    await hid.retry_identify(d);

    assert.equal(d.firmware_major, 2, 'retry must recover it without a reload');
    assert.equal(d.identify_failed, false, 'failure state must clear');
    assert.equal(d.identifying, false, 'retry loop must finish');
});
