# dusk

**dusk** is an OCCT-based replacement for the legacy [dawn](https://geant4.kek.jp/~tanaka/DAWN/About_DAWN.html) detector visualization tool. It reads STEP files (`.stp`) and produces SVG vector graphics of cross-section and perspective/isometric views.

A companion tool **duskcut** mirrors the `dawncut` cutting-plane utility.

## Quick start

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### dusk — render a STEP file to SVG

```bash
dusk -d detector.stp [options]
```

View parameters are read from `.DAWN_1.history` in the current directory (same format
as dawn, so `dawn_tweak` can be used directly). CLI flags override the history file:

| Flag | Description | History line |
|------|-------------|--------------|
| `--theta <deg>` | Elevation angle (0=top, 90=side) | 3 |
| `--phi <deg>` | Azimuth angle | 2 |
| `--mag <float>` | Magnification | 8 |
| `--draw <int>` | 1=wireframe 2=+hidden 3=outline | 9 |
| `-x/-y/-z <float>` | View target | 5/6/7 |
| `--light-theta` | Light elevation | 19 |
| `--light-phi` | Light azimuth | 18 |
| `-o <file>` | Output SVG (default: `<stem>.svg`) | — |

### duskcut — clip STEP geometry by a half-space

```bash
duskcut nx ny nz d input.stp [output.stp]
```

Same interface as dawncut. The plane `n·x = d` (mm) cuts the geometry;
the half `n·x ≤ d` is retained.

**Example** — cross-section at x=0 keeping negative-x half:
```bash
duskcut -1 0 0 0 detector.stp half.stp
```

## Drop-in replacement for dawn/dawncut

The existing `generate_eps` scripts in `eic/epic/scripts/view*/` can be adapted
by replacing:
- `dawncut` → `duskcut`
- `dawn -d file.prim` → `dusk -d file.stp`
- `.prim` input → `.stp` input (from `convert-to-step` CI job)

The `dawn_tweak` script works unchanged.

## OCCT dependency

Requires [OpenCASCADE](https://dev.opencascade.org/) 7.x (tested with 7.9.1).
In the EIC software environment (eic-shell) it is available at
`/opt/software/linux-x86_64_v2/opencascade-7.9.1-*/`.

## Testing

Unit tests cover parser and SVG writer logic and do not require OCCT.

```bash
cmake -S . -B build-tests -DDUSK_BUILD_APPS=OFF -DDUSK_ENABLE_TESTS=ON
cmake --build build-tests --target unit_tests
ctest --test-dir build-tests --output-on-failure
```

To enable gcov/lcov instrumentation locally:

```bash
cmake -S . -B build-tests \
  -DDUSK_BUILD_APPS=OFF \
  -DDUSK_ENABLE_TESTS=ON \
  -DDUSK_ENABLE_COVERAGE=ON \
  -DCMAKE_BUILD_TYPE=Debug
```

CI runs these tests and uploads coverage via Codecov.

## Code style and static analysis

`dusk` uses `clang-format`, `clang-tidy`, and `pre-commit` in CI.

Set up local hooks:

```bash
pip install pre-commit
pre-commit install
```

Run formatting and checks locally before pushing:

```bash
pre-commit run --all-files
```

CI behavior:

- `pre-commit` enforces formatting and hygiene checks.
- For same-repository pull requests, CI auto-commits pre-commit fixes back to the PR branch.
- `clang-tidy` runs in CI and posts fix suggestions as pull request comments.

## License

Apache 2.0

## Documentation

Docs are served with Docsify at:

- <https://eic.github.io/dusk/>

The CI-generated NIST sample gallery is published at:

- <https://eic.github.io/dusk/reports/geometry-images.html>
