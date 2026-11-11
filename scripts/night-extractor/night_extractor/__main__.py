"""`python3 -m night_extractor` -- the same command as the `night-extractor` console script."""
import sys

from .cli import main

if __name__ == "__main__":
    sys.exit(main())
