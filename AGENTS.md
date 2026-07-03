# AGENTS.md

## Scope

Instructions in this file apply to the repository root and all subdirectories.

## Build Directory Policy

Agents must use a designated build directory for all work:

- `build-agent` for CMake build files
- `install-agent` for install output

`cmake -S . -B build-agent -DCMAKE_INSTALL_PREFIX=install-agent ...`

**Never build in, configure, or otherwise touch `build` or `install`.**

If `build-agent` does not exist yet, that means no one has configured it for
you: run the `cmake -S . -B build-agent ...` command above yourself to create
it first. A missing `build-agent` directory is never a reason to fall back to
`build` — it is an instruction to create `build-agent`.

## Build/Test Flags

When building tests, always configure CMake with these options:

- `-DASTRE_BUILD_TESTS=ON`

Use build configuration Debug or RelWithDebInfo.

The unit test build target is:

- `AstreEngineTests`

## Build/Test Execution Gate

Do **not** rebuild tests by default for every change.

Skip test rebuild when changes are non-compiled only, for example:

- Documentation (`README.md`, `AGENTS.md`, docs)
- Static resources/UI assets only, unless tests explicitly depend on them
- Pure formatting/comment-only edits in files that do not alter compiled/tested behavior

When rebuild is needed, prefer incremental verification:

- Build `AstreEngineTests`
- Run only relevant test cases first (targeted filter)
- Run broader/full test pass only when the scope of changes justifies it

Do **not** run build or test commands automatically after making changes.

Only run build/test commands when the user gives an explicit command/request to do so.

Examples of gated commands include:

- `cmake --build ...`
- `ctest ...`
- running `AstreEngineTests` directly

## Temporary working area

All documentation, plans, tasks descriptions, etc. should be placed in the `.agent/docs` directory.

All temporary files, logs, etc. should be placed in the `.agent/tmp` directory.

All temporary scripts, agents, etc. should be placed in the `.agent/bin` directory.
