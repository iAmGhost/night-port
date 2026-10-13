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

# Tools
* use uv for python
* use `dosbox-staging` for running original game

## Running original game

Run command below on dosbox for running original game

```bat
MOUNT C ~/Git/night-port/NIGHT
MOUNT D ~/Git/night-port/NIGHT/TG_NIGHT.CUE
C:
NIGHT
```

# How to analysis
* Use ghidra mcp for analysing NIGHT.EXE