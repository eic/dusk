# Usage

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)"
```

## Render a STEP file

```bash
dusk -d detector.stp -o detector.svg --theta 165 --phi 75 --draw 3
```

Common flags:

- `--theta`, `--phi`: camera orientation
- `--mag`: magnification
- `--draw`: `1` wireframe, `2` visible + hidden, `3` outline
- `-x`, `-y`, `-z`: target position

## Cut then render

```bash
duskcut -1 0 0 0 detector.stp section.stp
dusk -d section.stp -o section.svg
```
