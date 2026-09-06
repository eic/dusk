# CI sample galleries

The galleries are generated in CI and published to GitHub Pages.

## Pipeline summary

1. CI fetches NIST AP203 STEP fixtures.
2. CI converts the default ePIC detector geometry to STEP in `eic_ci:nightly`
   after loading `/opt/detector/epic-main/bin/thisepic.sh`.
3. `dusk`/`duskcut` render SVG snapshots for both input sets.
4. A report generator writes separate pages under `docs/reports/`.
5. The `deploy` job publishes the Docsify site and galleries on pushes to `main`.

## Live reports

- [Geometry Fixture Images](reports/geometry-images.html)
- [ePIC Geometry Images](reports/epic-geometry-images.html)
