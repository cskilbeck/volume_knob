# Per-device hosting plan for the unified configurator

**Status: done, 2026-09-22.** All four domains live and serving their own device; both legacy
hosts retired to permanent redirects; browser verification and `certbot renew --dry-run` both
passed. Server commands and rollback are in **`deployment_runbook.md`**; deploy with
**`deploy.ps1`**.

```
usbknob.com          200  Tiny USB Things Configurator     (frozen everything-view)
midiknob.com         200  Tiny MIDI Knob Configurator
midiadapter.com      200  TRS MIDI Adapter Configurator
tinyvolumeknob.com   200  Tiny Volume Knob Configurator
hid.skilbeck.com     301  -> tinyvolumeknob.com
midi.skilbeck.com    301  -> midiknob.com
```
**Date:** 2026-09-22
**Scope:** `web_unified_config` (app), the Digital Ocean VPS `167.71.142.52` (nginx), Cloudflare DNS
**Not in scope:** firmware, the SysEx protocol, `Midi.js` / `hid.js` protocol code, the legacy apps in `web_hid_config` / `web_midi_config`

---

## 1. What this is for

One Vue app configures three gadgets. Today it is deployed once, at `usbknob.com`, and shows
all of them at once. That creates three concrete problems:

1. **The Web HID picker dialog fires at people who only own a MIDI device.** `hid.js:359`
   calls `navigator.hid.requestDevice()` on every Scan. The API has no quiet variant: it
   always shows the browser's device picker and always requires a user gesture. Someone with
   a MIDI knob clicks Scan and is asked to pick a USB device they don't have.
2. **Firefox users get told the site doesn't support them, when it does.** `App.vue:15`
   evaluates `!!navigator.hid` → `false` on Firefox, `scan()` skips HID, and everything MIDI
   works fine — but the page renders a yellow *"This browser does not support Web HID"*
   banner and a UI half-built for devices they don't own.
3. **Almost everybody owns exactly one gadget.** A unified page asks every visitor to work
   out which of three panels is theirs.

The fix is to make the site know which device the visitor has *before* it scans. The domain
they arrived on is that knowledge, and the domains are already bought.

A fourth problem surfaced while measuring, and turns out to be the largest of all — see
§3 (Traffic data). About 44% of visitors are on a platform where the configurator can
never work, and are currently told nothing.

---

## 2. Current state (audited 2026-09-22)

### 2.1 App

`E:\dev\hid_kbd\web_config\web_unified_config`, Vue 3 + Vite 5 + Bootstrap 5, no router.

The device-specific machinery already exists and needs no redesign:

| Thing | Where | Note |
|---|---|---|
| MIDI device registry | `src/midi/Midi.js:304` `DEVICE_TYPES` | keyed by model number; holds config map, defaults, flags, and `component` name |
| Knob model | `Midi.js:48` | `0x3322`, component `MidiDevice` |
| Adapter model | `Midi.js:207` | `0x3323`, component `MidiAdapterDevice` |
| MIDI component dispatch | `DeviceList.vue` `midi_component()` | returns `null` for unknown → renders nothing |
| HID component dispatch | `DeviceList.vue` `hid_component()` | by `firmware_major` (≥2 → `HIDDevice_2`) |
| HID filter | `hid.js:359` | VID `0x16D0`, PID `0x114B`, usage 1, usagePage `0xFF00` |
| Scan entry point | `App.vue` `scan()` | fires HID picker and `requestMIDIAccess({sysex:true})` concurrently |
| API presence flags | `App.vue:15-16` | `got_hid_api`, `got_midi_api` |
| Registry export | `Midi.js:1205` | `DEVICE_TYPES` is already exported |

Because `midi_component()` already treats "unknown type" as "render nothing", per-site
filtering is a predicate over an existing table, not a refactor.

**`navigator.hid.getDevices()` is never called anywhere.** Already-granted HID devices can be
enumerated with no dialog at all; the app doesn't do it, so returning visitors get the picker
on every single visit.

### 2.2 Server

`ssh chs@167.71.142.52` — Ubuntu, nginx, sites in `/etc/nginx/sites-enabled/`:

```
00-default                        → returns 444 for any unmatched host
usbknob.com.conf                  → root /var/www/usbknob      ← the unified app, current build
hid.skilbeck.com                  → root /var/www/hid          ← legacy HID-only app
midi.skilbeck.com                 → root /var/www/midi         ← legacy MIDI-only app
gadgets.skilbeck.com.conf         → Ghost blog, proxy to :2368
skilbeck.com.conf, makethewords.com.conf, wifiswitch*.conf
```

- `/var/www/usbknob` currently holds `index-D1OZN820.js` + `index-Dtqps-mN.css`, which are
  **byte-for-byte the current local `dist/`**. usbknob.com is already serving this exact app.
- `usbknob.com.conf` is the good modern template: SPA fallback, 30d `immutable` on hashed
  assets, `/.well-known/` passthrough, dotfile deny, www→apex redirect, and a
  `if ($http_host !~* ...) { return 444; }` guard.
- `/var/www/usbknob/assets` holds **14 bundles going back to May** — deploys never prune.
  This is arguably correct (old `index.html` in someone's cache still resolves) but it is
  accumulation by accident rather than by policy.
- `00-default` completes the TLS handshake using the `skilbeck.com` cert, then `return 444`.
  That is why an unconfigured proxied domain shows a Cloudflare **520** rather than a
  connection error.
- `/etc/nginx/conf.d/cloudflare.conf` sets `set_real_ip_from` for Cloudflare's ranges, so
  access logs carry **real client IPs**, and `log_format combined_with_host` appends the
  `Host` header as the last field. Both matter for §3.
- certbot 5.8.0 present at `/snap/bin/certbot` (snap).
- `midi.skilbeck.com` is serving TLS using **`/etc/letsencrypt/live/hid.skilbeck.com/`** certs.
  Presumably a SAN, but worth confirming; if not, that host is one renewal away from breaking.

### 2.3 DNS / Cloudflare

All four domains use Cloudflare nameservers (`anirban`/`brenda.ns.cloudflare.com`).

| Domain | A record | Proxy | Status today |
|---|---|---|---|
| usbknob.com | 104.21.66.243 / 172.67.210.2 (CF) | on | **200, serving the app** |
| midiadapter.com | 104.21.51.150 / 172.67.182.19 (CF) | on | **520** — no vhost at origin, falls into `00-default`'s 444 |
| midiknob.com | **none** | — | dead, NS only |
| tinyvolumeknob.com | 104.21.70.129 / 172.67.223.141 (CF) | on | **301 → `https://tiny-little-things.square.site/`** (being removed, §5.3) |
| www.tinyvolumeknob.com | same | on | same redirect |
| www.midiknob.com, www.midiadapter.com | none | — | dead |

Two things to note. **midiadapter.com is publicly broken right now** — visitors get a
Cloudflare error page. And **tinyvolumeknob.com is currently in use as a store shortlink**;
the 301 is served by Cloudflare itself (`Server: cloudflare`, origin never touched), so it is
a Redirect Rule or Page Rule in the dashboard, not anything on the VPS. Repurposing that
domain means editing Cloudflare and deciding what happens to the shortlink (§5.3).

---

## 3. Traffic data (why the plan includes a mobile message)

Method: `zcat -f /var/log/nginx/access.log*` over 31 Aug – 22 Sep 2026, filtered to
`Host == usbknob.com`, status 200, request `GET /` or `GET /index.html` (i.e. page loads, not
asset fetches), with obvious bots/tools excluded by UA (`bot|crawl|spider|HeadlessChrome|curl|python|Go-http|okhttp`).
Counting unique client IPs, which the real-IP module makes meaningful.

| Browser / platform | Unique IPs | Page loads | Can it configure? |
|---|---|---|---|
| Chrome desktop | 384 | 612 | yes |
| **Safari mobile (iPhone/iPad)** | **290** | 450 | **no — impossible** |
| other desktop (unidentified UA) | 88 | 96 | unknown |
| **Firefox desktop** | **53** | 58 | yes, via the add-on dialog |
| Safari desktop | 34 | 35 | no |
| Chrome mobile | 28 | 34 | no |
| Edge desktop | 17 | 35 | yes |
| Opera desktop | 2 | 2 | yes |
| Opera mobile / Firefox mobile | 3 / 1 | 3 / 1 | no |

Readings:

- **~322 visitors (≈40%) arrive on a phone or tablet.** On iOS every browser is WebKit
  underneath, and WebKit ships neither Web MIDI nor Web HID, so nothing can ever work there.
  They currently get a configurator that silently does nothing. These are almost certainly
  people tapping a link from an Etsy/Square listing or scanning a code on packaging.
- **Firefox desktop is 53 visitors (~11% of the desktop visitors who can succeed at all)** —
  small, but larger than Safari desktop, and it costs nothing to keep because it already works.
- Chrome + Edge + Opera desktop ≈ 403 visitors are the population the tool actually serves.

Caveats: three weeks only; the 88 "other desktop" are unidentified UAs (likely a mix of
privacy tooling and bots that dodge the filter); one IP is not one person (CGNAT, offices).
The ratios are the point, not the absolute counts.

### 3.1 Firefox and SysEx

Confirmed by screenshot on 2026-09-22: Firefox still gates **SysEx** MIDI access behind an
**AMO site-permission add-on install**, with the wording *"This access is not guaranteed to be
safe. Only continue if you trust this site."* The whole config protocol is SysEx, so there is
no way to avoid it — non-SysEx MIDI is useless here, since even device identification is a
SysEx Identity Request.

Consequences for this plan:

- The add-on is **bound to the origin**, so a Firefox user configuring a knob *and* an adapter
  installs it twice under the new domain split. Accepted cost; affects very few people.
- **The silent auto-connect in §4.6 must never fire on Firefox.** Triggering that dialog on
  page load, with no click, would be strictly worse than today.
- Mitigation is copy, not code: warn *before* the click, in our own words, and offer Chrome as
  the smoother path. The dialog is alarming mostly because it arrives with no context.

**Verdict on Firefox: keep it working, don't chase it.** It works today; no code is needed to
"support" it, only ~30 minutes of framing copy. It should not constrain any design decision.

---

## 4. Target design

### 4.1 Domain → profile

| Domain | Profile | Transports used | Devices shown |
|---|---|---|---|
| tinyvolumeknob.com | `volumeknob` | HID only | Volume Knob (`HIDDevice_1` / `HIDDevice_2`) |
| midiknob.com | `midiknob` | MIDI only | model `0x3322` (`MidiDevice`) |
| midiadapter.com | `adapter` | MIDI only | model `0x3323` (`MidiAdapterDevice`) |
| usbknob.com | `all` | HID + MIDI | everything — **behaviour frozen exactly as it is today** |
| localhost, IPs, previews, `?site=` | `all` (or the named override) | as profile | support & dev view |

`usbknob.com` keeping the everything-view is deliberate: it is the URL already in the wild,
already on printed material, and the one a customer types when they don't know whether their
knob is the HID or the MIDI variant. The specific domains are what goes on new packaging.

**Decided 2026-09-22: usbknob.com does not change.** It is not becoming a chooser, and the
`all` profile is defined as "what the app does today" — both transports scanned on Scan, every
device rendered, the existing `partial_support` banners retained. Every behavioural change in
this plan (hard transport gating, wrong-door notices, banner removal, silent reconnect) is
therefore expressed as a **per-profile flag that is off for `all`**, so the new domains can
diverge without touching the one that already has users.

The single exception, **confirmed 2026-09-22: the mobile and unsupported-browser messages
(§4.2) are added on every site, usbknob.com included.** Those visitors currently get a page
that silently does nothing; ~40% of usbknob.com's traffic is in that position (§3).

**Single build, four deploys.** The bundle is identical everywhere; only the static
`index.html` head differs per domain, and behaviour is chosen at runtime from
`location.hostname`.

### 4.2 Three rendering states

Every site renders exactly one of:

1. **`ready`** — the API this profile needs exists. Show the configurator.
2. **`no-api`** — desktop browser without the needed API (Safari desktop; Firefox on a HID
   site). Show *"This needs Chrome, Edge or Opera on a desktop computer"* with a caniuse link.
3. **`no-platform`** — mobile/tablet. Show *"Configuring needs a desktop or laptop computer"*,
   the domain name in large text so it can be re-typed later, and a mail-to-self link.

**Decision rule: capability decides the state, platform only chooses the wording.** Feature
detection first (`navigator.hid` / `navigator.requestMIDIAccess`); if the needed API is
missing, *then* ask whether this is a touch device to pick message 2 or 3. This avoids UA
sniffing being load-bearing.

Touch detection needs one wrinkle: iPadOS Safari reports a desktop UA, so use
`navigator.userAgentData?.mobile === true`, else `/iPhone|iPad|iPod|Android|Mobile/i`, else
`navigator.maxTouchPoints > 1 && /Macintosh/.test(navigator.userAgent)` (iPad masquerading).

Plus one non-blocking notice, independent of state:

4. **Firefox + MIDI site + permission not yet granted** → the pre-emptive add-on explanation,
   shown *above the Scan button*, before anything is clicked.

The existing `partial_support` banners in `App.vue:152-170` are **suppressed on single-device
profiles** (`partial_banners: false`) and **kept on `all`**: on a single-device site, "this
browser lacks the other API" is noise and state 2 already covers the API that matters, but
usbknob.com's behaviour is frozen, so they stay there.

### 4.3 New file: `src/site.js`

Dependency-free (no imports) so it can be imported from anywhere without cycles. Component
names are strings, matching `DEVICE_TYPES[].component` in `Midi.js`.

```js
// src/site.js — which gadget this hostname is for.
//
// One bundle serves four domains. The domain decides which transport we touch
// and which devices we render; everything else is identical. Runtime lookup,
// not a build flag, so `npm run dev` and ?site= can reach every profile.

// Flags exist so that `all` (usbknob.com) can be pinned to today's behaviour
// while the single-device sites diverge:
//   partial_banners   keep the "browser lacks the other API" banners
//   firefox_notice    explain Firefox's SysEx add-on before it is triggered
//   wrong_door        offer a link when a device we don't render turns up
//   silent_reconnect  re-attach already-granted devices on load, without a click
const PROFILES = {
    volumeknob: {
        key: 'volumeknob',
        title: 'Tiny Volume Knob Configurator',
        product: 'Tiny Volume Knob',
        blurb: 'Configures the volume/keyboard (HID) version of the Tiny USB Knob.',
        transports: ['hid'],
        midi_components: [],
        demos: ['hid'],
        store_url: 'https://tiny-little-things.square.site/',
        partial_banners: false,
        wrong_door: true,
        silent_reconnect: true,
    },
    midiknob: {
        key: 'midiknob',
        title: 'Tiny MIDI Knob Configurator',
        product: 'Tiny MIDI Knob',
        blurb: 'Configures the MIDI version of the Tiny USB Knob.',
        transports: ['midi'],
        midi_components: ['MidiDevice'],
        demos: ['midi'],
        store_url: 'https://tiny-little-things.square.site/',
        partial_banners: false,
        wrong_door: true,
        silent_reconnect: true,
    },
    adapter: {
        key: 'adapter',
        title: 'TRS MIDI Adapter Configurator',
        product: 'TRS MIDI Adapter',
        blurb: 'Configures the TRS-to-USB MIDI adapter.',
        transports: ['midi'],
        midi_components: ['MidiAdapterDevice'],
        demos: ['adapter'],
        store_url: 'https://tiny-little-things.square.site/',
        partial_banners: false,
        wrong_door: true,
        silent_reconnect: true,
    },
    // usbknob.com — frozen. Today's behaviour, nothing added but the
    // unsupported-platform messages.
    all: {
        key: 'all',
        title: 'Tiny USB Things Configurator',
        product: 'Tiny USB Things',
        blurb: 'Configures every Tiny USB Things gadget.',
        transports: ['hid', 'midi'],
        midi_components: ['MidiDevice', 'MidiAdapterDevice'],
        demos: ['hid', 'midi', 'adapter'],
        store_url: 'https://www.etsy.com/uk/shop/TinyLittleGadgets',
        partial_banners: true,
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

// Where a device we refuse to render actually belongs. Used for the
// "wrong door" notice — only reachable on a MIDI site, which enumerates
// every port anyway and therefore KNOWS what it found.
export const COMPONENT_HOME = {
    MidiDevice: { url: 'https://midiknob.com', name: 'Tiny MIDI Knob' },
    MidiAdapterDevice: { url: 'https://midiadapter.com', name: 'TRS MIDI Adapter' },
};

export function resolve_profile(hostname, search) {
    const override = new URLSearchParams(search || '').get('site');
    if (override && PROFILES[override]) return PROFILES[override];
    return PROFILES[HOSTS[(hostname || '').toLowerCase()] ?? 'all'];
}

export const site = resolve_profile(location.hostname, location.search);

export const wants_hid = site.transports.includes('hid');
export const wants_midi = site.transports.includes('midi');
```

Unknown hostnames (localhost, `127.0.0.1`, a bare IP, any future preview host) fall through to
`all`, which is today's behaviour — so nothing breaks in development.

### 4.4 `App.vue` changes

**a. Gate the scan.**

```js
import { site, wants_hid, wants_midi } from './site.js'

async function scan() {
    if (scanning.value) return;
    scanning.value = true;
    try {
        const hid_promise = (wants_hid && got_hid_api.value)
            ? hid.init_devices() : Promise.resolve();

        if (wants_midi && got_midi_api.value) {
            // ... unchanged requestMIDIAccess / midi.init_devices() block ...
        }
        await hid_promise.catch(() => { });
    } finally {
        scanning.value = false;
    }
}
```

That single `wants_hid` guard is the entire fix for problem 1: on a MIDI domain,
`navigator.hid.requestDevice()` is unreachable.

**b. Support state.**

```js
const needed_api_present = computed(() =>
    (wants_hid && got_hid_api.value) || (wants_midi && got_midi_api.value));

const is_touch_device = /* per §4.2 */;

const support_state = computed(() =>
    needed_api_present.value ? 'ready' : (is_touch_device ? 'no-platform' : 'no-api'));
```

For the `all` profile, `ready` if *either* API is present (today's behaviour).

**c. Title, blurb and demo buttons from `site`.** `<h5>{{ site.title }}</h5>`; each demo button
wrapped in `v-if="site.demos.includes('hid')"` etc.; the info modal body uses `site.blurb` and
`site.product`.

**d. Wrong-door notice.**

```js
const foreign_devices = computed(() => midi.midi_devices.value
    .filter(d => d.type && !site.midi_components.includes(d.type.component))
    .map(d => COMPONENT_HOME[d.type.component])
    .filter(Boolean));
```

Gated on `site.wrong_door`, so it never appears on usbknob.com — which renders those devices
anyway, making the notice meaningless there.

Rendered as one line per distinct device: *"Found a TRS MIDI Adapter — configure it at
midiadapter.com"*. This costs nothing because a MIDI site already sends an Identity Request
to every port. The mirror case (a MIDI device plugged in while on tinyvolumeknob.com) is
**undetectable without the MIDI permission we are avoiding**, so that site gets a static
footer line instead: *"Got the MIDI version? midiknob.com"*.

**e. Gate** the two `partial_support` banners on `site.partial_banners` — kept on usbknob.com,
hidden on the single-device sites (§4.2).

### 4.5 `DeviceList.vue` change

```js
import { site } from '../site.js'

function midi_component(device) {
    const name = device.type?.component;
    if (!name || !site.midi_components.includes(name)) return null;
    return MIDI_COMPONENTS[name] ?? null;
}
```

HID needs no filter: the only HID devices that can ever appear are already restricted by the
VID/PID filter at `hid.js:359`, and only the `volumeknob` and `all` profiles scan HID at all.

Demo devices flow through the same filter (a dummy MIDI device is assigned a real
`DEVICE_TYPES` entry), so the demo buttons stay consistent with the site automatically.

### 4.6 Silent reconnect (the other half of problem 1)

Both paths below are gated on `site.silent_reconnect`, which is **false for `all`** — so
usbknob.com keeps its click-to-Scan behaviour untouched. Neither path can ever raise a prompt
(`getDevices()` returns only already-granted devices; the MIDI call happens only when the
permission is already `granted`), so switching it on for usbknob.com later is a one-line
change if you decide you want it.

**HID.** Add to `hid.js`:

```js
function attach_listeners() {           // extracted from init_devices()
    navigator.hid.removeEventListener("connect", on_connection_event);
    navigator.hid.removeEventListener("disconnect", on_connection_event);
    navigator.hid.addEventListener("connect", on_connection_event);
    navigator.hid.addEventListener("disconnect", on_connection_event);
}

// Devices the user has ALREADY granted. No picker, no user gesture.
async function init_known_devices() {
    attach_listeners();
    const devices = await navigator.hid.getDevices();
    for (const d of devices) {
        if (d.vendorId !== 0x16D0 || d.productId !== 0x114B) continue;
        const existing = hid_devices.value[d.productName];
        if (existing && existing.firmware_major !== null) continue;
        if (!d.opened) await d.open();
        await init_device(d);
    }
    if (devices.length) scanned.value.done = true;
}
```

Called from `App.vue` `onMounted()` when `wants_hid && got_hid_api`. `getDevices()` returns
only previously-granted devices, so a first-time visitor sees nothing and the flow is
unchanged; a returning visitor's knob simply appears.

**MIDI.** Same idea, but the Firefox trap makes the guard mandatory:

```js
async function midi_already_granted() {
    try {
        if (!navigator.permissions?.query) return false;
        const s = await navigator.permissions.query({ name: 'midi', sysex: true });
        return s.state === 'granted';
    } catch {
        return false;   // Firefox throws on the unknown permission name — do NOT
    }                   // fall through to requestMIDIAccess, that is the add-on prompt
}
```

Only when this returns `true` do we call `requestMIDIAccess({sysex:true})` on mount. Chromium
supports the query; Firefox does not, so Firefox keeps today's click-driven flow and never
sees an unprompted add-on dialog.

### 4.7 Per-domain `index.html`

Metadata must be right before any JS runs — otherwise the tab says "Tiny USB Things
Configurator" while the page says "MIDI Knob", which reintroduces exactly the confusion the
domains are meant to remove. `index.html` is static, so this is a build step, not runtime.

New `scripts/build-sites.mjs`, run after `vite build`:

- read `dist/index.html`
- for each of the four domains: substitute `<title>`, add `<meta name="description">`,
  `og:title` / `og:description` / `og:url` / `og:image`, and (optionally) a per-domain favicon
- write `dist-sites/<domain>/` with `assets/` and `favicon.ico` copied across

Vite emits absolute `/assets/...` paths, so every copy works at any domain root. Wire it up as
`"build": "vite build && node scripts/build-sites.mjs"`.

Per-domain favicons are optional and can come later; one shared `favicon.ico` is acceptable
for the first cut.

---

## 5. Server & DNS work

### 5.1 nginx: four vhosts, four roots

Clone `usbknob.com.conf` per domain, changing only `server_name`, the host-guard regex, and
`root`. Full template for a new site (`/etc/nginx/sites-available/midiknob.com.conf`):

```nginx
server {
    listen 80;
    listen [::]:80;
    server_name midiknob.com www.midiknob.com;

    if ($http_host !~* "^(www\.)?midiknob\.com(:\d+)?$") { return 444; }

    location ^~ /.well-known/ { root /var/www/midiknob; try_files $uri =404; }
    location / { return 301 https://midiknob.com$request_uri; }
}

server {
    listen 443 ssl;
    server_name midiknob.com www.midiknob.com;

    if ($http_host !~* "^(www\.)?midiknob\.com(:\d+)?$") { return 444; }
    if ($host = www.midiknob.com) { return 301 https://midiknob.com$request_uri; }

    root /var/www/midiknob;
    index index.html;

    ssl_certificate     /etc/letsencrypt/live/midiknob.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/midiknob.com/privkey.pem;
    include /etc/letsencrypt/options-ssl-nginx.conf;
    ssl_dhparam /etc/letsencrypt/ssl-dhparams.pem;

    include /etc/nginx/mime.types;

    # Hashed bundles: immutable, cache hard.
    location ~* \.(?:js|css|svg|png|jpg|jpeg|gif|ico|webp|woff2?|ttf|webmanifest)$ {
        try_files $uri =404;
        expires 30d;
        add_header Cache-Control "public, immutable";
    }

    # The document itself must never be cached, or a deploy ships new bundles
    # to browsers still holding the old HTML. An `=` match beats the regex
    # above, and try_files' final argument is an internal redirect, so this
    # also applies to the SPA fallback.
    location = /index.html {
        add_header Cache-Control "no-cache, must-revalidate";
    }

    location / { try_files $uri $uri/ /index.html; }

    location ^~ /.well-known/ { try_files $uri =404; }
    location ~ /\.(?!well-known) { deny all; }
}
```

Roots to create: `/var/www/midiknob`, `/var/www/midiadapter`, `/var/www/tinyvolumeknob`.
`/var/www/usbknob` stays as-is. (The directory name `usbknob` remains accurate under this
plan, since that domain keeps the everything-view.)

Add the same `location = /index.html` block to `usbknob.com.conf`.

### 5.2 Certificates

**Decided 2026-09-22: Let's Encrypt on the instance, exactly as usbknob.com does it. Every
zone on Cloudflare set to Full (strict).** No Cloudflare Origin CA certificates — one
mechanism, one renewal path, one thing to understand at 2am.

```bash
sudo certbot certonly --nginx -d midiknob.com       -d www.midiknob.com
sudo certbot certonly --nginx -d midiadapter.com    -d www.midiadapter.com
sudo certbot certonly --nginx -d tinyvolumeknob.com -d www.tinyvolumeknob.com
```

`certonly` so certbot issues and renews but does **not** rewrite the hand-written vhosts.
HTTP-01 works through the Cloudflare proxy — usbknob.com already holds a real LE cert while
proxied, so this path is proven on this box.

Order matters: **issue the certificate before enabling the vhost that references it**, or
nginx fails to reload. Either point the vhost at the `skilbeck.com` cert for the first reload,
or run certbot while the domain still falls through to `00-default`.

Dashboard check per zone (SSL/TLS → Overview): all four must read **Full (strict)**. Anything
on Flexible is sending plaintext to the origin and should be fixed regardless of this plan.
After the certs are in, verify the origin directly, bypassing Cloudflare:

```bash
curl -sI --resolve midiknob.com:443:167.71.142.52 https://midiknob.com/ | head -3
```

### 5.3 DNS / Cloudflare changes

| Domain | Change |
|---|---|
| midiknob.com | add `A @ → 167.71.142.52` (proxied) and `CNAME www → @` (proxied) |
| midiadapter.com | already resolves; add `CNAME www → @`. The 520 disappears the moment its vhost exists |
| tinyvolumeknob.com | **delete the Cloudflare Redirect Rule** to `tiny-little-things.square.site`; point `A @ → 167.71.142.52` (proxied) and `CNAME www → @` (proxied) |
| usbknob.com | no change |

**Decided 2026-09-22: the store redirect goes.** tinyvolumeknob.com becomes the Volume Knob
configurator outright. Three consequences to handle:

- The rule lives in the **Cloudflare dashboard** (Rules → Redirect Rules / Page Rules), not on
  the VPS. Delete it there, then add the A and CNAME records. If the domain is currently a
  redirect-only zone it may have no A record at all behind the rule — check.
- **Anything already printed or linked that used tinyvolumeknob.com as a store shortlink stops
  reaching the store.** The store remains reachable at its own `tiny-little-things.square.site`
  URL; worth a scan of Etsy/Square listings, business cards and packaging for that link before
  the switch.
- Each profile therefore carries a `store_url`, surfaced in the info modal, so the configurator
  still funnels people to the shop. `tinyvolumeknob` points at the Square store, inheriting the
  traffic the redirect used to carry; `all` keeps today's Etsy link, unchanged.

### 5.4 Deploy

Local (git-bash on Windows):

```bash
npm run build                      # vite build && node scripts/build-sites.mjs
for d in usbknob midiknob midiadapter tinyvolumeknob; do
  scp -r dist-sites/$d.com/* chs@167.71.142.52:/tmp/deploy-$d/
  ssh chs@167.71.142.52 "sudo cp -r /tmp/deploy-$d/* /var/www/$d/ && rm -rf /tmp/deploy-$d"
done
```

Deliberately **no `--delete` / no pruning**: old hashed bundles stay so that any browser still
holding a cached `index.html` keeps working. Prune by hand once or twice a year (§7).

Cloudflare does not cache HTML by default, and the `no-cache` header makes it explicit, so no
purge step is required. If a Cache Rule for HTML is ever added, a purge must be added here.

---

## 6. Rollout order

Each phase is independently shippable and independently verifiable.

**Phase 0 — stop the bleeding (30 min).**
Create `/var/www/midiadapter` with the current build and its vhost + cert. midiadapter.com
stops serving 520. It will show the unified app until Phase 2; that is strictly better than an
error page.

**Phase 1 — app changes, deployed only to usbknob.com (half a day).**
`site.js`, scan gating, list filtering, support states, title/demo wiring, wrong-door notice.
**Verify every profile before touching DNS**, using the override on the existing domain:

- `https://usbknob.com/?site=midiknob` → no HID dialog on Scan, only knobs listed
- `https://usbknob.com/?site=adapter` → only adapters listed, knob shows the wrong-door line
- `https://usbknob.com/?site=volumeknob` → no MIDI permission prompt at all
- `https://usbknob.com/` → **unchanged behaviour**, which the `all` profile's flags guarantee:
  both transports scanned, every device rendered, `partial_support` banners still present, no
  silent reconnect. This is the regression test that matters most in Phase 1.

This is the whole point of runtime resolution: the risky part ships and is tested with zero
infrastructure change and an instant rollback (redeploy the previous bundle).

**Phase 2 — domains (1–2 hours).**
Delete the tinyvolumeknob.com redirect rule in Cloudflare, add the DNS records, create three
new roots, three vhosts, certs, the per-domain `index.html` build step and the deploy script.
Verify each domain serves its own title and its own device, and that usbknob.com is untouched.

**Phase 3 — silent reconnect (2 hours).**
`getDevices()` on mount; permission-gated `requestMIDIAccess` on mount. Verify explicitly on
Firefox that **no** add-on prompt appears without a click.

**Phase 4 — copy (1–2 hours).**
Firefox pre-warning, mobile/desktop messages, wrong-door wording, info-modal text per site.

**Phase 5 — retire the legacy apps (1 hour).**
Decided 2026-09-22. `hid.skilbeck.com` and `midi.skilbeck.com` serve the superseded
single-purpose apps from `web_hid_config` and `web_midi_config`.

**They are still in real use — this is a redirect, not a deletion.** Measured 31 Aug – 22 Sep:

| Host | Page loads | Unique IPs | Recent rate |
|---|---|---|---|
| hid.skilbeck.com | 439 | 300 | ~22/day |
| midi.skilbeck.com | 466 | 319 | ~17/day |
| usbknob.com (for scale) | 1498 | — | ~126/day |

Together that's roughly a quarter of usbknob.com's traffic, arriving daily, almost certainly
from links and bookmarks that predate usbknob.com. Serving them a 404 or a dead domain would
strand people who own working hardware.

Steps:

1. **Confirm feature parity first.** The unified app must do everything the legacy ones did
   before their URLs stop reaching them — `web_hid_config` still carries
   `HIDDevice_original.vue`, which has no counterpart in the unified tree. Check nothing is
   lost, especially for older firmware.
2. Replace each vhost body with a permanent redirect, keeping the TLS config (the cert is
   still needed to serve a redirect over HTTPS):
   ```nginx
   location / { return 301 https://tinyvolumeknob.com/; }   # hid.skilbeck.com
   location / { return 301 https://midiknob.com/; }         # midi.skilbeck.com
   ```
   Redirect to `/`, not `$request_uri` — both are single-page apps with no real routes, so
   there are no deep links worth preserving.
3. **Fix the certificate first.** `midi.skilbeck.com` currently serves TLS using
   `/etc/letsencrypt/live/hid.skilbeck.com/`. Confirm it is a SAN on that cert; if not, issue
   one for `midi.skilbeck.com` before the redirect vhost goes live, or the redirect itself
   breaks at the next renewal.
4. Keep the redirect vhosts and their DNS **indefinitely** — they cost nothing and the
   inbound links will not stop.
5. Delete `/var/www/hid` and `/var/www/midi` only after a few weeks of clean redirect logs.
6. Mark `web_hid_config` and `web_midi_config` as archived in the repo so nobody deploys them
   again by habit.

**Phase 6 — optional polish.**
Per-domain favicons and OG images. Landing pages, if the domains ever need to sell rather than
only configure.

---

## 7. Test matrix

| Case | Where | Expected |
|---|---|---|
| Scan on midiknob.com | Chrome desktop | MIDI permission prompt only; **no HID picker** |
| Scan on tinyvolumeknob.com | Chrome desktop | HID picker only; **no MIDI prompt** |
| Scan on usbknob.com | Chrome desktop | both, as today |
| Adapter plugged in | midiknob.com | not rendered; "configure it at midiadapter.com" line |
| Knob plugged in | midiadapter.com | not rendered; wrong-door line |
| Returning visitor, knob already paired | tinyvolumeknob.com | device appears on load, **no dialog** |
| Returning visitor, MIDI already granted | midiknob.com, Chrome | device appears on load, no click |
| First visit | midiknob.com, Firefox | our explanation shown **before** any prompt; add-on dialog only after clicking Scan |
| Page load | midiknob.com, Firefox | **no add-on dialog without a click** |
| Any site | Safari desktop | `no-api` message, no broken UI |
| Any site | iPhone | `no-platform` message, domain shown large, mail-to-self link |
| Two identical adapters | midiadapter.com | two independent panels (regression check on the `device.key` work) |
| Demo buttons | each site | only that site's demo button visible |
| `?site=all` | any domain | everything, for support |
| Tab title | each domain | correct **before** JS runs (check with JS disabled) |
| Old cached index.html | any | still resolves, because old bundles are never pruned |
| `https://hid.skilbeck.com/` | after Phase 5 | 301 → tinyvolumeknob.com, valid TLS, no warning |
| `https://midi.skilbeck.com/` | after Phase 5 | 301 → midiknob.com, valid TLS, no warning |
| Origin reached directly | `curl --resolve <domain>:443:167.71.142.52` | valid publicly-trusted cert (required by Full (strict)) |

---

## 8. Risks and known issues

**Introduced by this plan:**

- **Per-origin permissions.** Anyone who has used usbknob.com re-grants MIDI (and re-pairs
  HID) once on the new domain. Harmless for the ~one-device-each majority; worth a line of
  copy on first visit.
- **Firefox add-on per origin** (§3.1). Two installs for a two-gadget Firefox user.
- **Four deploy targets** instead of one. Mitigated by the deploy loop in §5.4; the bundle is
  identical, so there is no version skew *between* domains unless a deploy half-fails.
- **tinyvolumeknob.com's store shortlink** changes meaning. Check what is already printed.

**Pre-existing, found during this audit, NOT part of this work:**

- **`any_scanned` is broken.** `hid.js:9` and `Midi.js:65` both declare `let scanned = ref({})`,
  but `hid.js` writes `scanned.value.done` while `Midi.js` writes `scanned.done` — a property
  on the ref object itself. `App.vue:94` reads `hid.scanned.done || midi.scanned.done`, so the
  HID half is always `undefined`, and neither read is reactively tracked, which means the
  computed has no dependencies and never re-evaluates. The "Found N devices" text is therefore
  unreliable. Cheap fix (`ref(false)` on both sides, or `reactive({done:false})`), and it sits
  in the scan flow this plan is touching — fold it in if convenient, but it is not required.
- **`hid_devices` is keyed by `d.productName`** (`hid.js:302`), so two identical knobs collide
  and the second is silently dropped. This is the exact bug that was fixed on the MIDI side by
  moving to `device.key` (the port id); HID needs the same treatment, keyed on a serial or a
  per-device token. Out of scope here, but it will bite eventually.
- **`midi.skilbeck.com` serves TLS with the `hid.skilbeck.com` certificate.** Confirm it is a
  SAN on that cert, or it breaks at some renewal. **Now load-bearing:** Phase 5 keeps that
  hostname alive permanently as a redirect, so the certificate has to keep working.
- **14 stale bundles in `/var/www/usbknob/assets`**, back to May. Harmless and mildly useful;
  worth a policy decision rather than leaving it accidental.

---

## 9. Files touched

**New:**

| Path | Purpose |
|---|---|
| `web_unified_config/src/site.js` | hostname → profile, component→home table |
| `web_unified_config/scripts/build-sites.mjs` | per-domain `index.html` + copies |
| `web_unified_config/src/components/UnsupportedNotice.vue` *(optional)* | the `no-api` / `no-platform` states |
| `/etc/nginx/sites-available/midiknob.com.conf` | + symlink into `sites-enabled` |
| `/etc/nginx/sites-available/midiadapter.com.conf` | ditto |
| `/etc/nginx/sites-available/tinyvolumeknob.com.conf` | ditto |
| `/var/www/{midiknob,midiadapter,tinyvolumeknob}/` | web roots |

**Modified:**

| Path | Change |
|---|---|
| `src/App.vue` | scan gating, support states, title/blurb/demos from `site`, wrong-door notice, delete `partial_support` banners, `onMounted` silent reconnect |
| `src/components/DeviceList.vue` | filter `midi_component()` by `site.midi_components` |
| `src/hid/hid.js` | extract `attach_listeners()`, add `init_known_devices()` using `getDevices()` |
| `src/midi/Midi.js` | *(no change required — `DEVICE_TYPES` is already exported)* |
| `package.json` | `build` runs the post-build script |
| `/etc/nginx/sites-enabled/usbknob.com.conf` | add `location = /index.html` no-cache block |

**Unchanged:** all SysEx/protocol code, the device registry, every device panel component,
firmware.

---

## 10. Summary of decisions

| Decision | Choice | Why |
|---|---|---|
| Runtime hostname vs build flag | **runtime** | one bundle, `?site=` testing, no build matrix; the only build-time part is `<title>`/OG |
| Gate hard or soft | **hard** on transport | a soft gate re-fires the HID picker, which is problem 1 |
| Cross-device help | **detect on MIDI sites, static link on the HID site** | MIDI sites already know what they found; the HID site cannot know without the prompt we're avoiding |
| Fallback domain | **usbknob.com = everything, frozen** | it's the URL in the wild and the one typed by people unsure which variant they own; all new behaviour is behind per-profile flags that are off for `all` |
| tinyvolumeknob.com | **store redirect deleted**, apex serves the Volume Knob configurator | decided 2026-09-22; store stays reachable via each profile's `store_url` |
| Support/dev view | **`?site=all` + localhost default** | keeps the unified page as a tool for you, not for customers |
| Firefox | **keep, don't chase** | already works; ~11% of capable desktop visitors; cost is copy, not code |
| Mobile | **explicit message** | ≈40% of traffic, currently gets a dead page |
| Roots | **four, one per domain** | per-domain `<title>` must be correct at first paint; allows later divergence |
| Old bundle pruning | **none** | cached `index.html` keeps working |
| TLS | **Let's Encrypt on the instance, all zones Full (strict)** | one mechanism, matching usbknob.com; no second cert system to remember |
| Legacy apps | **301 redirect, kept indefinitely** | ~40 real page loads/day between them; deleting would strand owners of working hardware |
| Mobile messaging | **every site, usbknob.com included** | those visitors get nothing today |

---

## 11. Open questions

**Settled 2026-09-22:**

- ~~tinyvolumeknob.com apex~~ → store redirect deleted, apex serves the configurator (§5.3).
- ~~usbknob.com as a chooser~~ → no. Frozen as today's everything-view (§4.1).
- ~~Cloudflare SSL mode / cert strategy~~ → all zones Full (strict); Let's Encrypt on the
  instance via `certbot certonly`, as usbknob.com already does (§5.2).
- ~~Legacy apps~~ → retired by permanent redirect, not deleted; they still take ~40 real page
  loads a day between them (Phase 5).
- ~~Mobile messaging on usbknob.com~~ → yes, added everywhere (§4.1, §4.2).

**Also settled during implementation:**

- ~~tinyvolumeknob.com's A record~~ → added, proxied. The store redirect turned out to be a
  **Cloudflare Redirect Rule** (phase `http_request_dynamic_redirect`, expression `true`,
  target `concat("https://tiny-little-things.square.site", http.request.uri.path)`) — invisible
  in Page Rules because it is a different product with a separate quota. It also swallowed
  `/.well-known/acme-challenge/`, which would have broken renewal. `cf-find-redirect.ps1`
  finds this class of thing via the API.
- ~~`midi.skilbeck.com` SAN~~ → yes, it is on the `hid.skilbeck.com` certificate (to 9 Nov 2026).
- ~~Feature parity~~ → confirmed. `HIDDevice_original.vue` is dead code, imported by nothing;
  the legacy `DeviceList.vue` dispatch is identical to the unified one, and the unified
  `HIDDeviceBase` is a superset (same 412 lines plus a DEMO badge).
- ~~tinyvolumeknob.com printed anywhere~~ → no, so the store redirect was deleted outright.
- ~~Per-domain favicons~~ → not wanted; one shared favicon across all four sites.

**Traffic correction (measured 2026-09-22).** The earlier "~22 and ~17 real page loads a day"
for the legacy hosts was **wrong** — UA filtering does not catch cloud scanners that spoof
Chrome. Counting only IPs that fetched the page *and* its assets: 37 and 56 over three weeks
against usbknob.com's 133, i.e. roughly 2-3 real visitors a day each. Still not zero, and
still worth the redirect, but an order of magnitude below the first estimate. Top sources are
Tencent Cloud, `198.235.x` and `34.32.x` — scanners, on all three hosts equally.
