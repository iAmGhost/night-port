#!/usr/bin/env node
/* check.js -- `make check`: stage, serve, run, assert, stop.  docs/457.
 *
 * The page has to be served for anything to work (a Range-capable server, for the CD-DA), so the test
 * brings its own server up on a port it can kill again. What it asserts lives in headless.js's NIGHT_CHECK
 * branch; this only owns the process supervision and the wait.
 */
const { spawn } = require('child_process');
const path = require('path');
const http = require('http');

const dir = process.env.DIST || path.join(__dirname, '..', 'dist');
const port = process.env.PORT || '8091';
const ms = process.env.WAIT || '22000';
const keys = process.env.NIGHT_KEYS || 'Space:150 Enter:150 ArrowRight:150 KeyZ:150 Space:150';
const sleep = (t) => new Promise((r) => setTimeout(r, t));

const server = spawn('python3', [path.join(__dirname, 'serve.py'), '--dir', dir, '--port', port],
                     { stdio: ['ignore', 'pipe', 'pipe'] });
server.stdout.on('data', () => {});
server.stderr.on('data', (d) => process.stderr.write('[serve] ' + d));

async function wait_port() {
    for (let i = 0; i < 100; i++) {
        await new Promise((r) => setTimeout(r, 100));
        const ok = await new Promise((res) => {
            const req = http.get(`http://127.0.0.1:${port}/night.wasm`, (r) => { res(r.statusCode === 200); r.resume(); });
            req.on('error', () => res(false));
        });
        if (ok) return true;
    }
    return false;
}

(async () => {
    if (!(await wait_port())) { console.log('the staging server did not come up on ' + port); process.exit(1); }
    /* ?sink=1: pull the PCM rings on a timer instead of through an output device. Headless Chrome has no
     * audio device (its AudioContext errors), and the ring and budget maths is exactly what a check has to
     * have running. Audio through a real device is what NIGHT_HEADFUL=1 is for. */
    const url = `http://127.0.0.1:${port}/index.html?sink=1${process.env.NIGHT_URL_EXTRA || ''}`;
    const child = spawn('node', [path.join(__dirname, 'headless.js'), url, ms,
                                 path.join(__dirname, '..', 'build', 'check.png')],
                        { stdio: 'inherit', env: { ...process.env, NIGHT_CHECK: '1', NIGHT_KEYS: keys } });
    const code = await new Promise((r) => child.on('exit', r));
    server.kill();
    process.exit(code);
})();
