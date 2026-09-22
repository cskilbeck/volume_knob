//////////////////////////////////////////////////////////////////////
// Which gadget this hostname is for.
//
// One bundle serves four domains. The hostname decides which transport we
// touch and which devices we render; the code is identical everywhere. This
// is resolved at RUNTIME rather than baked in at build time so that `npm run
// dev`, an IP address, a preview host and ?site= can all reach every profile
// without a separate build.
//
// The point of the split is that a MIDI-only site must never reach
// navigator.hid.requestDevice(), which always throws up the browser's device
// picker — surprising anyone who only owns a MIDI device, and impossible to
// suppress. Knowing the device up front is the only fix.
//
// usbknob.com is deliberately FROZEN as the everything-view: it is the URL
// already in the wild and the one typed by someone who doesn't know whether
// their knob is the HID or the MIDI variant. Every behaviour added for the
// single-device sites is therefore a per-profile flag that is off for `all`.

// Exported so scripts/build-sites.mjs can generate one index.html per domain
// from the same table the app reads at runtime. Nothing in here may touch
// `location` at module scope, or importing it from node would blow up.
export const PROFILES = {

    volumeknob: {
        key: 'volumeknob',
        host: 'tinyvolumeknob.com',
        title: 'Tiny Volume Knob Configurator',
        product: 'Tiny Volume Knob',
        blurb: 'Configures the volume / keyboard (HID) version of the Tiny USB Knob.',
        transports: ['hid'],
        midi_components: [],
        demos: ['hid'],
        store_url: 'https://tiny-little-things.square.site/',
        partial_banners: false,
        firefox_notice: true,
        wrong_door: true,
        silent_reconnect: true,
    },

    midiknob: {
        key: 'midiknob',
        host: 'midiknob.com',
        title: 'Tiny MIDI Knob Configurator',
        product: 'Tiny MIDI Knob',
        blurb: 'Configures the MIDI version of the Tiny USB Knob.',
        transports: ['midi'],
        midi_components: ['MidiDevice'],
        demos: ['midi'],
        store_url: 'https://tiny-little-things.square.site/',
        partial_banners: false,
        firefox_notice: true,
        wrong_door: true,
        silent_reconnect: true,
    },

    adapter: {
        key: 'adapter',
        host: 'midiadapter.com',
        title: 'TRS MIDI Adapter Configurator',
        product: 'TRS MIDI Adapter',
        blurb: 'Configures the TRS-to-USB MIDI adapter.',
        transports: ['midi'],
        midi_components: ['MidiAdapterDevice'],
        demos: ['adapter'],
        store_url: 'https://tiny-little-things.square.site/',
        partial_banners: false,
        firefox_notice: true,
        wrong_door: true,
        silent_reconnect: true,
    },

    // usbknob.com — frozen. Today's behaviour exactly: both transports on
    // Scan, every device rendered, the existing partial-support banners, no
    // silent reconnect. The only addition is the unsupported-platform notice,
    // which replaces a page that currently does nothing at all for those
    // visitors.
    all: {
        key: 'all',
        host: 'usbknob.com',
        title: 'Tiny USB Things Configurator',
        product: 'Tiny USB Things',
        blurb: 'Configures every Tiny USB Things gadget: the HID and MIDI versions of the Tiny USB Knob, and the TRS MIDI Adapter.',
        transports: ['hid', 'midi'],
        midi_components: ['MidiDevice', 'MidiAdapterDevice'],
        demos: ['hid', 'midi', 'adapter'],
        store_url: 'https://www.etsy.com/uk/shop/TinyLittleGadgets',
        partial_banners: true,
        firefox_notice: false,
        wrong_door: false,
        silent_reconnect: false,
    },
};

const HOSTS = {
    'tinyvolumeknob.com': 'volumeknob',
    'www.tinyvolumeknob.com': 'volumeknob',
    'midiknob.com': 'midiknob',
    'www.midiknob.com': 'midiknob',
    'midiadapter.com': 'adapter',
    'www.midiadapter.com': 'adapter',
    'usbknob.com': 'all',
    'www.usbknob.com': 'all',
};

// Where a device we refuse to render actually belongs.
//
// Only reachable from a MIDI site, which enumerates every port and sends an
// Identity Request to each one regardless — so it already KNOWS it found the
// other gadget, and pointing at the right domain costs nothing. The mirror
// case (a MIDI device plugged in while on the HID site) cannot be detected
// without the MIDI permission we are deliberately not asking for, which is
// why the HID site gets a static link instead.
export const COMPONENT_HOME = {
    MidiDevice: { url: 'https://midiknob.com', name: 'Tiny MIDI Knob' },
    MidiAdapterDevice: { url: 'https://midiadapter.com', name: 'TRS MIDI Adapter' },
};

export function resolve_profile(hostname, search) {
    const override = new URLSearchParams(search || '').get('site');
    if (override && PROFILES[override]) {
        return PROFILES[override];
    }
    // Unknown hosts — localhost, a bare IP, any future preview host — get the
    // everything-view, which is also the support view.
    return PROFILES[HOSTS[(hostname || '').toLowerCase()] ?? 'all'];
}

// The `typeof location` guard is what lets the build script import this file
// under node, where there is no document to resolve against.
export const site = typeof location !== 'undefined'
    ? resolve_profile(location.hostname, location.search)
    : PROFILES.all;

export const wants_hid = site.transports.includes('hid');
export const wants_midi = site.transports.includes('midi');

// The other single-device sites, for the "got a different gadget?" footer.
// Empty on `all`, which already renders everything.
export const OTHER_SITES = site.key === 'all' ? [] :
    ['volumeknob', 'midiknob', 'adapter']
        .filter(k => k !== site.key)
        .map(k => ({ name: PROFILES[k].product, url: `https://${PROFILES[k].host}` }));

export default site;
