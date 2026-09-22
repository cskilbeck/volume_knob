//////////////////////////////////////////////////////////////////////
// What we are running on.
//
// Used only to CHOOSE THE WORDING of a failure, never to decide whether
// something works: capability detection (navigator.hid /
// navigator.requestMIDIAccess) decides that. If the API we need is missing,
// these flags pick between "use a desktop computer" and "use a different
// browser", because those are very different problems for the reader.
//
// Roughly 40% of visitors arrive on a phone, where neither Web MIDI nor Web
// HID exists in any browser (on iOS everything is WebKit underneath), and
// until now they were shown a configurator that silently did nothing.

const ua = navigator.userAgent || '';

// Firefox has Web MIDI, but gates SysEx — which is the entire config
// protocol — behind installing a site-permission add-on from AMO. The dialog
// is alarming and arrives with no context, so we explain it BEFORE the user
// clicks anything, and we must never trigger it without a click.
export const is_firefox = /Firefox\//.test(ua) && !/Seamonkey\//.test(ua);

export const is_touch_device = (() => {
    // Chromium's client hints, where available, are the only honest answer.
    if (navigator.userAgentData?.mobile === true) return true;
    if (/iPhone|iPad|iPod|Android|Mobile/i.test(ua)) return true;
    // iPadOS Safari reports a desktop Macintosh UA and has to be caught by
    // its touch points, or every iPad gets the "use a different browser"
    // message when what it actually needs is a different computer.
    if (/Macintosh/.test(ua) && (navigator.maxTouchPoints || 0) > 1) return true;
    return false;
})();

export const is_ios = /iPhone|iPad|iPod/i.test(ua) ||
    (/Macintosh/.test(ua) && (navigator.maxTouchPoints || 0) > 1);
