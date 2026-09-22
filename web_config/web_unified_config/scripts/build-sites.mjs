//////////////////////////////////////////////////////////////////////
// One Vite build -> one deployable directory per domain.
//
// The bundle is identical everywhere and picks its behaviour from
// location.hostname at runtime (src/site.js). The only thing that CANNOT be
// decided at runtime is the contents of <head>: the tab title is read from the
// static HTML before any JavaScript runs, and crawlers never run it at all. A
// page whose tab says "Tiny USB Things Configurator" while its body says
// "MIDI Knob" puts back exactly the confusion the per-domain split removes.
//
// So: build once, then stamp a <head> per domain. The profile table is
// imported from src/site.js rather than copied, so the two cannot drift.
//
// Output: dist-sites/<host>/ — a complete web root, ready to copy to
// /var/www/<key>/ on the server.

import { readFile, writeFile, mkdir, rm, cp, readdir } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

import { PROFILES } from '../src/site.js';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const dist = join(root, 'dist');
const out_root = join(root, 'dist-sites');

// Everything in dist except index.html is domain-agnostic and copied verbatim.
const VERBATIM_SKIP = new Set(['index.html']);

function head_for(profile) {
    const url = `https://${profile.host}/`;
    const title = profile.title;
    const description = profile.blurb;

    return [
        `  <title>${title}</title>`,
        `  <meta name="description" content="${description}">`,
        `  <link rel="canonical" href="${url}">`,
        `  <meta property="og:type" content="website">`,
        `  <meta property="og:site_name" content="${profile.product}">`,
        `  <meta property="og:title" content="${title}">`,
        `  <meta property="og:description" content="${description}">`,
        `  <meta property="og:url" content="${url}">`,
        `  <meta name="twitter:card" content="summary">`,
    ].join('\n');
}

async function build_site(profile, template) {

    const out = join(out_root, profile.host);

    // Replace the single <title> line the Vite template carries with the full
    // per-domain head. Fail loudly rather than silently shipping four
    // identical documents if index.html ever stops having one.
    if (!/^\s*<title>.*<\/title>\s*$/m.test(template)) {
        throw new Error('index.html has no <title> line to replace — has the template changed?');
    }

    const html = template.replace(/^\s*<title>.*<\/title>\s*$/m, head_for(profile));

    await mkdir(out, { recursive: true });

    for (const entry of await readdir(dist, { withFileTypes: true })) {
        if (VERBATIM_SKIP.has(entry.name)) continue;
        await cp(join(dist, entry.name), join(out, entry.name), { recursive: true });
    }

    await writeFile(join(out, 'index.html'), html, 'utf8');

    console.log(`  ${profile.host.padEnd(22)} -> dist-sites/${profile.host}/  (${profile.title})`);
}

async function main() {

    const template = await readFile(join(dist, 'index.html'), 'utf8')
        .catch(() => { throw new Error('dist/index.html not found — run `vite build` first'); });

    await rm(out_root, { recursive: true, force: true });

    console.log('building per-domain roots:');
    for (const profile of Object.values(PROFILES)) {
        await build_site(profile, template);
    }
}

main().catch(err => {
    console.error(`build-sites failed: ${err.message}`);
    process.exit(1);
});
