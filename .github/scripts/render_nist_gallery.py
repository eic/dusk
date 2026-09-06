#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path


def run(cmd):
    subprocess.run(cmd, check=True)


def render_fixture(dusk_bin: Path, duskcut_bin: Path, fixture: Path, out_dir: Path) -> bool:
    fixture_id = fixture.stem
    iso_out = out_dir / f"{fixture_id}-iso.svg"
    section_step = out_dir / f"{fixture_id}-section.step"
    section_out = out_dir / f"{fixture_id}-section.svg"

    try:
        run([
            str(dusk_bin), "-d", str(fixture),
            "--theta", "165", "--phi", "75", "--draw", "3",
            "--mesh-deflection", "10",
            "-o", str(iso_out),
        ])
        run([
            str(duskcut_bin), "-1", "0", "0", "0",
            str(fixture), str(section_step),
        ])
        run([
            str(dusk_bin), "-d", str(section_step),
            "--theta", "90", "--phi", "90", "--draw", "1",
            "--mesh-deflection", "10",
            "-o", str(section_out),
        ])
        return True
    except subprocess.CalledProcessError as exc:
        print(f"WARNING: render failed for {fixture_id}: {exc}", file=sys.stderr)
        return False
    finally:
        if section_step.exists():
            section_step.unlink()


def main() -> int:
    if len(sys.argv) != 5:
        print(
            "Usage: render_nist_gallery.py <dusk-bin> <duskcut-bin> <fixtures-dir> <output-dir>",
            file=sys.stderr,
        )
        return 1

    dusk_bin = Path(sys.argv[1])
    duskcut_bin = Path(sys.argv[2])
    fixtures_dir = Path(sys.argv[3])
    out_dir = Path(sys.argv[4])
    out_dir.mkdir(parents=True, exist_ok=True)

    fixtures = sorted(list(fixtures_dir.glob("*.step")) + list(fixtures_dir.glob("*.stp")))
    if not fixtures:
        print(f"ERROR: no STEP fixtures found in {fixtures_dir}", file=sys.stderr)
        return 1

    success = 0
    for fixture in fixtures:
        if render_fixture(dusk_bin, duskcut_bin, fixture, out_dir):
            success += 1

    if success == 0:
        print("ERROR: rendering failed for all fixtures.", file=sys.stderr)
        return 1

    print(f"Rendered {success}/{len(fixtures)} fixture(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
