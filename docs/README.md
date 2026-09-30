# Docs

## This branch

- [DUPLOTRON.md](DUPLOTRON.md) is the working record: the scheduler-patched
  CodeWarrior, the ground rules, what's been tried and what's been ruled out.
  Read this before re-deriving something that's already been disproved.
- [dependencies.md](dependencies.md) is what the build needs.

## PC port

- [PCPORT.md](PCPORT.md) is the design record for the port that lives on the
  `treedome` branch. It's here because the argument for it was written while the
  decomp was still the only thing in the repo.

## Reference

Inherited from [bfbbdecomp](https://github.com/bfbbdecomp/bfbb) and still
accurate:

- [splits.md](splits.md), format of `config/GQPE78/splits.txt`
- [symbols.md](symbols.md), format of `config/GQPE78/symbols.txt`
- [common_bss.md](common_bss.md), how `-common on` places uninitialised globals.
  Worth knowing: retail's placement tells you whether a symbol was static.
- [comment_section.md](comment_section.md), CodeWarrior's `.comment` section and
  what it says about the compiler that built a unit

`getting_started.md`, `github_actions.md` and `images/` were dropped. They were
dtk template setup for starting a new decomp ("rename GAMEID", "create a repo
from this template") and pointed at a `.github.example/` folder that doesn't
exist here. Real CI is in `.github/workflows/`.
