#!/usr/bin/env python3
import argparse
import html
from pathlib import Path


def rel_to_output(image_path: Path, output_path: Path) -> str:
    try:
        rel = image_path.resolve().relative_to(output_path.resolve().parent)
    except ValueError:
        rel = Path(image_path.name)
    return str(rel).replace("\\", "/")


def caption_from_name(stem: str) -> str:
    return stem.replace("-", " ")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a simple static HTML gallery from image files."
    )
    parser.add_argument("images_dir", type=Path)
    parser.add_argument("output_html", type=Path)
    parser.add_argument(
        "--title",
        default="dusk geometry fixture gallery",
        help="HTML page title.",
    )
    parser.add_argument(
        "--heading",
        default="dusk geometry fixture images",
        help="Main page heading.",
    )
    parser.add_argument(
        "--meta",
        default="Generated from NIST CTC STEP fixtures in CI.",
        help="Metadata sentence shown below heading.",
    )
    return parser.parse_args()


def generate_page(items: list[tuple[str, str]], title: str, heading: str, meta: str) -> str:
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
  <title>{html.escape(title)}</title>
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
  <h1>{html.escape(heading)}</h1>
  <p class="meta">{html.escape(meta)}</p>
  <section class="grid">
{gallery}
  </section>
</body>
</html>
"""


def main() -> int:
    args = parse_args()
    images_dir = args.images_dir
    output_html = args.output_html
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

    output_html.write_text(
        generate_page(items, title=args.title, heading=args.heading, meta=args.meta),
        encoding="utf-8",
    )
    print(f"Wrote gallery page: {output_html} ({len(items)} images)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
