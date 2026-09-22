# Deployment runbook — Cloudflare, TLS, nginx

Companion to `new_hosting_plan.md`. Every command here is meant to be pasted as-is.

**Two shells are in play.** Blocks labelled *(PowerShell, local)* run on your Windows machine;
blocks labelled *(server)* run in bash over `ssh chs@167.71.142.52`, so keep their Unix syntax
exactly as written — the simplest way is to `ssh chs@167.71.142.52` first and paste there.

Two PowerShell gotchas that bite in this particular runbook:

- **`curl` is an alias for `Invoke-WebRequest`.** It does not understand `-s`, `-I` or
  `--resolve`. Always type **`curl.exe`**. Same for `wget`.
- **Backticks are PowerShell's escape character**, so never paste a Markdown backtick into a
  double-quoted string. (This is also why `deploy.ps1` is plain ASCII: Windows PowerShell 5.1
  reads a BOM-less file as ANSI, and a UTF-8 em dash decodes to a smart quote that silently
  terminates a string.)

**Do the domains one at a time, all the way through.** Each is independent, so a mistake on
one cannot take the others down, and `midiadapter.com` (currently serving a Cloudflare 520)
is the one worth doing first.

| Site key | Domain | Web root |
|---|---|---|
| `midiadapter` | midiadapter.com | `/var/www/midiadapter` |
| `midiknob` | midiknob.com | `/var/www/midiknob` |
| `tinyvolumeknob` | tinyvolumeknob.com | `/var/www/tinyvolumeknob` |
| `usbknob` | usbknob.com | `/var/www/usbknob` *(exists; only the cache tweak in §6)* |

---

## 0. The one trap in this whole process

Certificate issuance and the Cloudflare proxy deadlock each other if you do them in the wrong
order:

> Let's Encrypt validates over plain HTTP. With the orange cloud on and **Always Use HTTPS**
> enabled, Cloudflare answers the challenge by redirecting to `https://…`, then tries to reach
> your origin over TLS — using the certificate that does not exist yet. Validation fails, and
> the error looks nothing like the cause.

The fix is to **issue certificates with the record grey-clouded (DNS only), then turn the
proxy back on.** That is step §3 → §4 → §5 below, and it is why they are in that order.

Renewals are not affected: by then the origin has a valid certificate, so the proxied HTTPS
path works normally.

---

## 1. Cloudflare — DNS records

Dashboard → the domain → **DNS → Records**.

### midiknob.com (nothing exists yet)

| Type | Name | Content | Proxy | TTL |
|---|---|---|---|---|
| A | `@` | `167.71.142.52` | **DNS only** for now (grey) | Auto |
| CNAME | `www` | `midiknob.com` | **DNS only** for now (grey) | Auto |

### midiadapter.com (A record exists, orange)

Confirm the A record's content is **`167.71.142.52`** and not something stale. Add the `www`
CNAME if missing. Set both to **DNS only** for the moment.

### tinyvolumeknob.com (currently a redirect to the Square store)

1. **Rules → Redirect Rules** (also check **Rules → Page Rules**, older accounts keep it
   there): find the rule sending `tinyvolumeknob.com/*` to `https://tiny-little-things.square.site/`
   and **delete** it. Decided 2026-09-22 — the store link is not printed anywhere.
2. The zone may have no real A record behind that rule (a redirect-only zone often uses a
   dummy `192.0.2.1` or a proxied placeholder). Set it properly:

| Type | Name | Content | Proxy | TTL |
|---|---|---|---|---|
| A | `@` | `167.71.142.52` | **DNS only** for now (grey) | Auto |
| CNAME | `www` | `tinyvolumeknob.com` | **DNS only** for now (grey) | Auto |

### usbknob.com

No DNS change. Leave it exactly as it is, proxied and working.

### Check before moving on

*(PowerShell, local)* — each must return the origin IP `167.71.142.52`, not a Cloudflare one
(`104.21.*` / `172.67.*`):

```powershell
'midiknob.com','midiadapter.com','tinyvolumeknob.com' | ForEach-Object {
    $ips = (Resolve-DnsName $_ -Type A -Server 8.8.8.8 -ErrorAction SilentlyContinue |
            Where-Object Type -eq 'A').IPAddress -join ', '
    '{0,-22} {1}' -f $_, $(if ($ips) { $ips } else { '(no A record)' })
}
```

DNS propagation through Cloudflare is usually seconds, but give it a minute before certbot.

---

## 2. Server — web roots and files

*(server)* — create the roots once:

```bash
sudo mkdir -p /var/www/midiadapter /var/www/midiknob /var/www/tinyvolumeknob
sudo chown www-data:www-data /var/www/midiadapter /var/www/midiknob /var/www/tinyvolumeknob
```

*(PowerShell, local)* — everything else is `deploy.ps1`, which lives next to this file:

```powershell
cd E:\dev\hid_kbd\web_config

.\deploy.ps1 -WhatIf                    # show the plan, touch nothing
.\deploy.ps1 -Build                     # build, then deploy all four domains
.\deploy.ps1 -Site midiknob.com         # deploy one domain from the existing build
.\deploy.ps1 -Build -Force              # no confirmation prompt (for a scripted run)
```

What it does, in order: optionally runs `npm run build`; validates that every selected
`dist-sites\<domain>\` has an `index.html` and an `assets\` folder **before copying anything**,
so a half-finished build cannot leave one domain updated and another stale; prints a plan
table (bundle hash, file count, size, build time) and asks for confirmation; stages each root
in `/tmp/deploy-<key>` over `scp`; installs it with `sudo cp` over `ssh -t`; and finally reads
each site back **from the origin directly, bypassing Cloudflare**, reporting the `<title>` it
got and warning if the bundle served isn't the one just deployed.

Notes:

- **Nothing is built unless you pass `-Build`**, so what ships is exactly what you verified.
- **Nothing is deleted.** Old hashed bundles stay behind so a browser still holding a cached
  `index.html` keeps working. Prune by hand once or twice a year.
- `ssh -t` is used so `sudo` can prompt for a password. If you'd rather avoid sudo entirely,
  `sudo chown -R chs:www-data /var/www/<key>` on the server once, then deploy with `-NoSudo`.
- A domain that isn't live yet reports *"not serving yet"* in the verify step. That is
  expected until §3–§7 are done for it.
- `-Server` and `-User` are parameters if anything ever moves.

---

## 3. nginx — HTTP-only vhost first

Certbot needs a server block that answers on port 80 for the name being validated. Create the
HTTP half now and the TLS half after the certificate exists.

`/etc/nginx/sites-available/midiknob.com.conf` — repeat per domain, substituting the name and
root in the four obvious places:

```nginx
server {
    listen 80;
    listen [::]:80;
    server_name midiknob.com www.midiknob.com;

    if ($http_host !~* "^(www\.)?midiknob\.com(:\d+)?$") { return 444; }

    root /var/www/midiknob;
    index index.html;

    location ^~ /.well-known/ { try_files $uri =404; }
    location / { try_files $uri $uri/ /index.html; }
}
```

Enable and reload:

*(server)*:

```bash
sudo ln -s /etc/nginx/sites-available/midiknob.com.conf /etc/nginx/sites-enabled/
sudo nginx -t && sudo systemctl reload nginx
curl -s -o /dev/null -w "%{http_code}\n" http://midiknob.com/     # expect 200
```

---

## 4. TLS — issue the certificates

Certbot 5.8.0 is already installed (snap, `/snap/bin/certbot`). Use **webroot**, not the nginx
plugin, so certbot never rewrites these hand-written vhosts:

```bash
sudo certbot certonly --webroot -w /var/www/midiknob \
     -d midiknob.com -d www.midiknob.com \
     --deploy-hook "systemctl reload nginx"

sudo certbot certonly --webroot -w /var/www/midiadapter \
     -d midiadapter.com -d www.midiadapter.com \
     --deploy-hook "systemctl reload nginx"

sudo certbot certonly --webroot -w /var/www/tinyvolumeknob \
     -d tinyvolumeknob.com -d www.tinyvolumeknob.com \
     --deploy-hook "systemctl reload nginx"
```

The records must still be **grey-clouded** at this point (§0). Verify each certificate landed:

```bash
sudo ls /etc/letsencrypt/live/
sudo certbot certificates | grep -A2 midiknob
```

Renewal is automatic via the snap's systemd timer. Confirm it exists and works:

```bash
systemctl list-timers | grep -i certbot
sudo certbot renew --dry-run
```

---

## 5. nginx — the HTTPS vhost

Replace the whole file from §3 with the full version. This is `usbknob.com.conf`'s structure,
which is already the good one, plus the no-cache rule for the document.

`/etc/nginx/sites-available/midiknob.com.conf`:

```nginx
server {
    listen 80;
    listen [::]:80;
    server_name midiknob.com www.midiknob.com;

    if ($http_host !~* "^(www\.)?midiknob\.com(:\d+)?$") { return 444; }

    # Keep ACME reachable over plain HTTP for renewals.
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

    # Hashed, content-addressed bundles — safe to cache hard.
    location ~* \.(?:js|css|svg|png|jpg|jpeg|gif|ico|webp|woff2?|ttf|webmanifest)$ {
        try_files $uri =404;
        expires 30d;
        add_header Cache-Control "public, immutable";
    }

    # The document must NEVER be cached, or a deploy ships new bundles to
    # browsers still holding the old HTML. An `=` match beats the regex above,
    # and try_files' last argument is an internal redirect, so this applies to
    # the SPA fallback too.
    location = /index.html {
        add_header Cache-Control "no-cache, must-revalidate";
    }

    location / { try_files $uri $uri/ /index.html; }

    location ^~ /.well-known/ { try_files $uri =404; }
    location ~ /\.(?!well-known) { deny all; }
}
```

```bash
sudo nginx -t && sudo systemctl reload nginx
```

Verify the origin directly, bypassing Cloudflare entirely — this is exactly what Full (strict)
will do, so if it fails here it will fail there.

*(PowerShell, local)* — note `curl.exe`, not `curl`:

```powershell
curl.exe -sI --resolve midiknob.com:443:167.71.142.52 https://midiknob.com/ | Select-Object -First 3
curl.exe -s  --resolve midiknob.com:443:167.71.142.52 https://midiknob.com/ | Select-String '<title>'
```

Expect `HTTP/1.1 200` and `<title>Tiny MIDI Knob Configurator</title>`. A certificate error
here means Full (strict) will 526 later. (`.\deploy.ps1` runs this same check automatically
after every deploy.)

---

## 6. usbknob.com — the one small change

Add the no-cache block to the existing `/etc/nginx/sites-enabled/usbknob.com.conf`, inside the
`listen 443` server, next to the asset-caching block:

```nginx
    location = /index.html {
        add_header Cache-Control "no-cache, must-revalidate";
    }
```

```bash
sudo nginx -t && sudo systemctl reload nginx
```

Nothing else about usbknob.com changes.

---

## 7. Cloudflare — turn the proxy back on

Per domain, now that the origin serves valid TLS:

1. **DNS → Records:** switch `@` and `www` back to **Proxied** (orange cloud).
2. **SSL/TLS → Overview:** set the encryption mode to **Full (strict)**. All four zones —
   including usbknob.com, which should be checked even though it is untouched otherwise.
3. **SSL/TLS → Edge Certificates:** **Always Use HTTPS = On**. Leave *Automatic HTTPS
   Rewrites* on as well. Minimum TLS 1.2.
4. **Caching:** change nothing. Cloudflare does not cache HTML by default, which is what we
   want; the hashed assets are cached by extension automatically. Do **not** add a cache rule
   for HTML.

Then verify through the edge *(PowerShell, local)*:

```powershell
'midiknob.com','midiadapter.com','tinyvolumeknob.com','usbknob.com' | ForEach-Object {
    $code  = curl.exe -s -o NUL -w '%{http_code}' "https://$_/"
    # -join: curl.exe returns an array of lines, and -match against an array
    # filters it rather than filling in $Matches.
    $html  = (curl.exe -s "https://$_/") -join "`n"
    $title = if ($html -match '<title>([^<]*)</title>') { $Matches[1] } else { '(no title)' }
    '{0,-22} http={1} {2}' -f $_, $code, $title
}
```

Expected:

```
midiknob.com           http=200 <title>Tiny MIDI Knob Configurator</title>
midiadapter.com        http=200 <title>TRS MIDI Adapter Configurator</title>
tinyvolumeknob.com     http=200 <title>Tiny Volume Knob Configurator</title>
usbknob.com            http=200 <title>Tiny USB Things Configurator</title>
```

Also check `https://www.midiknob.com/` redirects to the apex, and that `http://` redirects to
`https://`.

---

## 8. Phase 5 — retiring the legacy apps

**Do not start this until feature parity is confirmed** (`web_hid_config` still carries
`HIDDevice_original.vue`, which has no counterpart in the unified app). These hosts take
roughly 22 and 17 real page loads a day, so a 404 would strand people with working hardware.

First, settle the certificate question — `midi.skilbeck.com` currently serves TLS using the
`hid.skilbeck.com` certificate:

```bash
sudo certbot certificates | grep -B3 -A3 skilbeck
openssl s_client -connect 127.0.0.1:443 -servername midi.skilbeck.com </dev/null 2>/dev/null \
  | openssl x509 -noout -text | grep -A1 "Subject Alternative Name"
```

If `midi.skilbeck.com` is **not** a SAN on that certificate, issue one before continuing:

```bash
sudo certbot certonly --webroot -w /var/www/midi -d midi.skilbeck.com \
     --deploy-hook "systemctl reload nginx"
```

Then replace the body of each vhost's `location /` with a permanent redirect, keeping the TLS
config exactly as it is:

```nginx
# /etc/nginx/sites-enabled/hid.skilbeck.com   (inside the listen 443 server)
location / { return 301 https://tinyvolumeknob.com/; }

# /etc/nginx/sites-enabled/midi.skilbeck.com  (inside the listen 443 server)
location / { return 301 https://midiknob.com/; }
```

Redirect to `/`, not `$request_uri` — both are single-page apps with no real routes.

*(server)*:

```bash
sudo nginx -t && sudo systemctl reload nginx
```

*(PowerShell, local)* — expect `301` and the new Location:

```powershell
'hid.skilbeck.com','midi.skilbeck.com' | ForEach-Object {
    curl.exe -sI "https://$_/" | Select-String '^HTTP|^location'
}
```

Keep the redirect vhosts and their DNS **indefinitely**. Delete `/var/www/hid` and
`/var/www/midi` only after a few weeks of clean logs, and mark `web_hid_config` /
`web_midi_config` as archived in the repo so nobody redeploys them out of habit.

Afterwards, see where the traffic was coming from, so the source links can be updated
*(server)*:

```bash
zcat -f /var/log/nginx/access.log* | grep '"hid\.skilbeck\.com"$' \
  | awk -F'"' '{print $4}' | grep -v '^-$' | sort | uniq -c | sort -rn | head
```

---

## 9. Rollback

Per domain, and quick in every case:

- **Bad bundle:** the previous hashed assets are still in the web root. Copy the previous
  `index.html` back over `/var/www/<key>/index.html`; it references the old bundle, which is
  still there. No rebuild needed.
- **Bad vhost:** `sudo rm /etc/nginx/sites-enabled/<domain>.conf && sudo nginx -t && sudo
  systemctl reload nginx`. The domain falls back to `00-default` (444 → Cloudflare 520),
  which is where midiadapter.com already is today.
- **Bad certificate / TLS mismatch:** set that zone's SSL mode to **Full** (non-strict) as a
  stopgap so the site works while you fix the origin, then put it back to Full (strict).
- **App-level:** every new behaviour is a flag in `src/site.js`. Setting a profile's
  `transports` back to `['hid','midi']` and `silent_reconnect: false` restores today's
  behaviour for that domain without touching the server.
- **Redeploy a known-good build:** `.\deploy.ps1 -Site <domain>` from a checkout of the last
  good commit. It never deletes, so the rollback is itself just another copy.

---

## 10. Order of operations, condensed

1. `.\deploy.ps1 -Build -WhatIf` to confirm the plan, then deploy per domain below
2. Per domain, starting with **midiadapter.com**:
   1. Cloudflare DNS record → **grey cloud**
   2. `mkdir` root *(server)*, then `.\deploy.ps1 -Site <domain>` *(PowerShell)*
   3. HTTP-only vhost → `nginx -t` → reload
   4. `certbot certonly --webroot …`
   5. Full HTTPS vhost → `nginx -t` → reload
   6. Verify origin with `curl --resolve`
   7. Cloudflare → **orange cloud** + **Full (strict)**
   8. Verify through the edge
3. usbknob.com: add the `location = /index.html` block only
4. Confirm all four zones read Full (strict)
5. Later, after the parity check: Phase 5 legacy redirects
