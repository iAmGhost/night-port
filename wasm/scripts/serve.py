#!/usr/bin/env python3
"""Serve the built browser port.  docs/457.

Why this exists instead of `python3 -m http.server`: two of the three things the page loads need a
server that is not the stdlib default.

  * CD-DA is streamed with HTTP Range requests (wasm/src/wasm_disc.c reads 150 KB windows out of a
    40-87 MB track file, because downloading a whole track to hear the first second of it is not a
    thing a browser port gets to do). Python's SimpleHTTPRequestHandler answers a Range request with
    200 and the entire file, so the reader would either eat the bandwidth or believe it had the window.
  * night.wasm needs application/wasm or the browser refuses to compile it.

Everything else is the default handler: index.html, no-cache while iterating, and following the
symlinks `make stage` leaves in data/cd so 604 MB is never copied.
"""
import argparse
import mimetypes
import os
import socketserver
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler

mimetypes.add_type('application/wasm', '.wasm')
mimetypes.add_type('application/javascript', '.js')


class RangeHandler(SimpleHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'          # keep-alive: the page asks for many ranges per song

    def send_response(self, code, message=None):
        super().send_response(code, message)
        # Iterating on the port means rebuilding night.wasm under the same URL, and a cached copy of the
        # old one is the most confusing bug you can chase in a browser build.
        self.send_header('Cache-Control', 'no-cache')

    def guess_type(self, path):
        if path.endswith('.wasm'):
            return 'application/wasm'
        return super().guess_type(path)

    def send_head(self):
        path = self.translate_path(self.path)
        rng = self.headers.get('Range')
        if not rng or not os.path.isfile(path):
            if rng:
                self.send_error(HTTPStatus.REQUESTED_RANGE_NOT_SATISFIABLE, 'no Range support')
                return None
            return super().send_head()

        spec = rng.split('=', 1)
        if len(spec) != 2 or spec[0] != 'bytes':
            self.send_error(HTTPStatus.REQUESTED_RANGE_NOT_SATISFIABLE, 'bad Range')
            return None
        size = os.path.getsize(path)
        start_s, _, end_s = spec[1].partition('-')
        try:
            if start_s == '':                        # suffix-Bytes: the last N
                start, end = max(0, size - int(end_s)), size - 1
            else:
                start = int(start_s)
                end = int(end_s) if end_s else size - 1
        except ValueError:
            self.send_error(HTTPStatus.REQUESTED_RANGE_NOT_SATISFIABLE, 'bad Range')
            return None
        if start >= size or end < start:
            self.send_error(HTTPStatus.REQUESTED_RANGE_NOT_SATISFIABLE, 'unsatisfiable')
            return None
        end = min(end, size - 1)
        length = end - start + 1

        f = open(path, 'rb')
        f.seek(start)
        self.send_response(HTTPStatus.PARTIAL_CONTENT)
        self.send_header('Content-Type', self.guess_type(path))
        self.send_header('Content-Length', str(length))
        self.send_header('Content-Range', f'bytes {start}-{end}/{size}')
        self.send_header('Accept-Ranges', 'bytes')
        self.end_headers()
        return f

    def log_message(self, fmt, *args):
        try:
            rng = self.headers.get('Range')
            if rng:
                args = (str(args[0]) + ' ' + rng,) + tuple(args[1:])
        except Exception:
            pass
        super().log_message(fmt, *args)


class ThreadingServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dir', required=True)
    ap.add_argument('--port', type=int, default=8091)
    ap.add_argument('--host', default='127.0.0.1')
    a = ap.parse_args()
    root = os.path.abspath(a.dir)
    if not os.path.exists(os.path.join(root, 'index.html')):
        raise SystemExit(f'{root} has no index.html -- run `make stage` first')

    def handler(*rest):
        return RangeHandler(*rest, directory=root)

    with ThreadingServer((a.host, a.port), handler) as httpd:
        print(f'serving {root} (Range-capable) on http://{a.host}:{a.port}/index.html')
        httpd.serve_forever()


if __name__ == '__main__':
    main()
