#!/usr/bin/env python3
import html
import sys
from pathlib import Path


def rel_to_output(image_path: Path, output_path: Path) -> str:
    try:
        rel = image_path.resolve().relative_to(output_path.resolve().parent)
    except ValueError:
        rel = Path(image_path.name)
    return str(rel).replace("\\", "/")


def caption_from_name(stem: str) -> str:
    return stem.replace("-", " ")


def generate_page(items: list[tuple[str, str]]) -> str:
    cards = []
    for src, caption in items:
        cards.append(
            f"""<article class="card">
  <img src="{html.escape(src)}" alt="{html.escape(caption)}">
  <div class="caption">{html.escape(caption)}</div>
</article>"""
        )

    gallery = "\n".join(cards) if cards else "<p>No rendered images were found for this run.</p>"
    return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>dusk geometry fixture gallery</title>
  <style>
    body {{
      font-family: Arial, sans-serif;
      margin: 2rem;
      background: #fafafa;
      color: #222;
    }}
    .meta {{
      color: #666;
      margin-bottom: 1.25rem;
    }}
    .grid {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
      gap: 1rem;
    }}
    .card {{
      margin: 0;
      border: 1px solid #ddd;
      border-radius: 8px;
      background: #fff;
      padding: 0.75rem;
    }}
    .card img {{
      width: 100%;
      height: auto;
      display: block;
      border: 1px solid #eee;
      background: #fff;
    }}
    .caption {{
      margin-top: 0.5rem;
      font-size: 0.9rem;
      color: #444;
    }}
  </style>
</head>
<body>
  <h1>dusk geometry fixture images</h1>
  <p class="meta">Generated from NIST CTC STEP fixtures in CI.</p>
  <section class="grid">
{gallery}
  </section>
</body>
</html>
"""


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: generate_geometry_gallery.py <images_dir> <output_html>", file=sys.stderr)
        return 1

    images_dir = Path(sys.argv[1])
    output_html = Path(sys.argv[2])
    output_html.parent.mkdir(parents=True, exist_ok=True)

    exts = ("*.svg", "*.png", "*.jpg", "*.jpeg")
    image_files = []
    for ext in exts:
        image_files.extend(images_dir.glob(ext))
    image_files = sorted(image_files)

    items = [
        (rel_to_output(img, output_html), caption_from_name(img.stem))
        for img in image_files
    ]

    output_html.write_text(generate_page(items), encoding="utf-8")
    print(f"Wrote gallery page: {output_html} ({len(items)} images)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
