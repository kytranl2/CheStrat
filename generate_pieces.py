#!/usr/bin/env python3
"""Generate chess piece PNG images with clean letter-based design."""

from PIL import Image, ImageDraw, ImageFont
import os

SIZE = 80
OUT_DIR = 'assets/pieces'
os.makedirs(OUT_DIR, exist_ok=True)

PIECE_LETTERS = {
    'wK': 'K', 'wQ': 'Q', 'wR': 'R', 'wB': 'B', 'wN': 'N', 'wP': 'P',
    'bK': 'K', 'bQ': 'Q', 'bR': 'R', 'bB': 'B', 'bN': 'N', 'bP': 'P',
}

# Find a bold font
font = None
for fp in [
    '/System/Library/Fonts/Supplemental/Arial Bold.ttf',
    '/System/Library/Fonts/Helvetica.ttc',
    '/System/Library/Fonts/SFNSDisplay-Bold.otf',
]:
    try:
        font = ImageFont.truetype(fp, 44)
        print(f"Using font: {fp}")
        break
    except Exception:
        continue

if font is None:
    font = ImageFont.load_default()
    print("Using default font")

for name, letter in PIECE_LETTERS.items():
    img = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    is_white = name[0] == 'w'

    # Draw rounded rectangle background
    margin = 4
    fill_color = (245, 240, 230, 250) if is_white else (50, 50, 50, 250)
    outline_color = (80, 70, 60) if is_white else (180, 170, 160)

    # Draw filled rounded rect
    r = 12  # corner radius
    x0, y0, x1, y1 = margin, margin, SIZE - margin, SIZE - margin
    draw.rounded_rectangle([x0, y0, x1, y1], radius=r, fill=fill_color,
                           outline=outline_color, width=2)

    # Draw the letter
    text_color = (50, 40, 30) if is_white else (230, 225, 215)
    bbox = draw.textbbox((0, 0), letter, font=font)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    x = (SIZE - tw) / 2 - bbox[0]
    y = (SIZE - th) / 2 - bbox[1]

    # Draw shadow
    shadow_color = (0, 0, 0, 60) if is_white else (0, 0, 0, 100)
    draw.text((x + 1, y + 1), letter, fill=shadow_color, font=font)
    draw.text((x, y), letter, fill=text_color, font=font)

    img.save(os.path.join(OUT_DIR, f'{name}.png'))
    print(f"  Created {name}.png ({os.path.getsize(os.path.join(OUT_DIR, f'{name}.png'))} bytes)")

print("Done!")
