# NIGHT — WebAssembly port

A SDL-free WebAssembly port of the 1994 DOS fighting game NIGHT by TG Entertainment.

## How to play

### Host it yourself

Serve this directory with any HTTP server:

    python3 -m http.server 8000 --directory dist-codeonly

Then open http://localhost:8000/ in a browser.

### Bring your own game data

The port ships without game data. Pick NIGHT.RES, or a CUE + IMG pair, or a zip containing them.
The page reads the file locally — nothing is uploaded.

## Controls

- Arrow keys — movement
- Z — punch
- X — kick
- Space — block
- Enter — start / menu

Touch controls are available on mobile.
