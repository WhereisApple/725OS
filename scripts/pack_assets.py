#!/usr/bin/env python3
"""Convert the project's transparent PNG sheets into an embedded RGBA asset pack."""
import argparse
from pathlib import Path
from PIL import Image

SHEETS = [
    ("asset_pacman", "PacMan.png", 16, 16),
    ("asset_ghost_red", "redGhost.png", 16, 16),
    ("asset_ghost_blue", "blueGhost.png", 16, 16),
    ("asset_ghost_green", "greenGhost.png", 16, 16),
    ("asset_ghost_orange", "orangeGhost.png", 16, 16),
    ("asset_ghost_yellow", "yellowGhost.png", 16, 16),
    ("asset_coin", "Coin.png", 16, 16),
    ("asset_coin_transparent", "CoinTransparent.png", 16, 16),
    ("asset_big_coin", "BigCoin.png", 16, 16),
    ("asset_big_coin_transparent", "BigCoinTransparent.png", 16, 16),
    ("asset_tileset", "Tileset.png", 48, 48),
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--assets", required=True, type=Path)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--source", required=True, type=Path)
    args = parser.parse_args()
    blob = bytearray()
    records = []
    for symbol, filename, fw, fh in SHEETS:
        path = args.assets / filename
        image = Image.open(path).convert("RGBA")
        if image.width % fw or image.height % fh:
            raise SystemExit(f"{path}: dimensions must be multiples of {fw}x{fh}")
        offset = len(blob)
        blob.extend(image.tobytes())
        records.append((symbol, offset, image.width, image.height, fw, fh,
                        image.width // fw, image.height // fh))
    args.binary.parent.mkdir(parents=True, exist_ok=True)
    args.source.parent.mkdir(parents=True, exist_ok=True)
    args.binary.write_bytes(blob)
    lines = [
        '#include "assets.h"',
        'extern const UINT8 _binary_build_assets_bin_start[];',
    ]
    for symbol, offset, width, height, fw, fh, cols, rows in records:
        lines.append(
            f'const SpriteSheet {symbol} = {{ '
            f'_binary_build_assets_bin_start + {offset}, {width}, {height}, '
            f'{fw}, {fh}, {cols}, {rows} }};'
        )
    args.source.write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
