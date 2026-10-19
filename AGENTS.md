# Goal

- Create modern SDL port of game.

# Analysis first

- Write analysis document when specific goal is achieved- so your work can be understood and reproduced.

# Paths

- ./NIGHT: Original game.
- ./NIGHT/TG_NIGHT.CUE, ./NIGHT/TG_NIGHT.IMG: Original CD Image
- ./scratch: Temporary scripts/resources
- ./scripts: Store generated scripts here(for extracting graphic, audio, etc...)
- ./docs: Documents about analysis and porting
- ./src: Keep newly created source port to here

# Tools

- use uv for python
- use `dosbox-staging` for running original game
- **Debugging tools (e.g. `ldbg`): always add an arbitrary timeout** so a watch/step/continue cannot loop or block forever. Never run an interactive debugger session unbounded -- cap it (e.g. a timeout wrapper / max-steps / alarm) and recover from the timeout instead of hanging.

## Running original game

Run command below on dosbox for running original game

```bat
MOUNT C ~/Git/night-port/NIGHT
MOUNT D ~/Git/night-port/NIGHT/TG_NIGHT.CUE
C:
NIGHT
```

# How to analysis

- Use ghidra mcp for analysing NIGHT.EXE