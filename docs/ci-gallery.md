# CI sample gallery

The gallery is generated in CI from a standard NIST STEP fixture set (same
source family used by `eic/g4occt`), then published to GitHub Pages.

## Pipeline summary

1. CI fetches NIST AP203 STEP fixtures.
2. `dusk`/`duskcut` render SVG snapshots for each fixture.
3. A report generator writes `docs/reports/geometry-images.html`.
4. The `deploy` job publishes the Docsify site and gallery on pushes to `main`.

## Live report

- [Geometry Fixture Images](reports/geometry-images.html)
