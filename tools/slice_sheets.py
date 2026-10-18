#!/usr/bin/env python3
"""Turns the hand-laid character sheets in assets/sources/ into the grid sheets the game reads.

The source sheets have labels and irregular spacing, so each animation is given
a region; frames inside it are split on the transparent gaps between them.
Every frame is then re-centred in a uniform cell so the animation doesn't wobble:
  - ground animations: feet on the bottom of the cell
  - air animations: body centre at body height, flames and smoke ignored

Usage: python3 tools/slice_sheets.py [debug_dir]
"""

import sys
from collections import deque
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent / "assets"
ALPHA = 60          # pixels above this alpha are part of a sprite
MERGE_GAP = 2       # body columns closer than this belong to the same frame
MIN_WIDTH = 30      # thinner bodies are debris, not frames
PAD = 4


def animation(region, fps, ground=False, mode="loop", frames=None, effects_ahead=False, splits=()):
    """One animation of a sheet.

    region:         box (x0, y0, x1, y1) holding the frames, labels left out
    ground:         feet on the floor (run) rather than flying
    mode:           "loop", "once" (holds the last frame) or "hide" (vanishes after the last frame)
    frames:         indices to keep, all by default
    effects_ahead:  loose effects (muzzle flash, blade arc) belong to the body on their left;
                    by default they are trails and belong to the body on their right
    splits:         sheet x positions where touching frames must be cut apart
    """
    return {"region": region, "fps": fps, "ground": ground, "mode": mode,
            "frames": frames, "effects_ahead": effects_ahead, "splits": splits}


# name: (source sheet, portrait box or None for the head of the first run frame, {animation: animation(...)})
CHARACTERS = {
    "johny": ("sources/johny.png", None, {
        "run":   animation((0, 177, 500, 330), 10, ground=True),
        "rise":  animation((0, 337, 500, 494), 12),
        "hover": animation((0, 337, 500, 494), 12),
        "fall":  animation((0, 177, 500, 330), 1, frames=[1]),
    }),
    "david": ("sources/david.png", None, {
        "run":   animation((0, 184, 500, 343), 10, ground=True),
        "rise":  animation((0, 349, 500, 497), 12),
        "hover": animation((0, 349, 500, 497), 12),
        "fall":  animation((0, 184, 500, 343), 1, frames=[1]),
    }),
    "jinx": ("sources/jinx.png", (1105, 929, 1185, 1005), {
        "run":     animation((665, 42, 1200, 140), 12, ground=True),
        "takeoff": animation((665, 185, 1536, 299), 16, mode="once"),
        "rise":    animation((0, 476, 765, 592), 12),
        "hover":   animation((0, 345, 1536, 420), 14),
        "fall":    animation((770, 476, 1536, 592), 12),
        "death":   animation((680, 779, 1480, 874), 10, mode="hide"),
        "attack":  animation((780, 640, 1255, 732), 16, mode="once", effects_ahead=True),
    }),
    "mikasa": ("sources/mikasa.png", (1140, 925, 1212, 1000), {
        "run":     animation((655, 40, 1240, 144), 12, ground=True),
        "takeoff": animation((0, 182, 640, 308), 16, mode="once"),
        "rise":    animation((650, 185, 1536, 308), 12),
        "hover":   animation((0, 351, 860, 442), 12),
        "fall":    animation((870, 346, 1536, 472), 12),
        "death":   animation((750, 795, 1536, 877), 10, mode="once"),
        "attack":  animation((795, 505, 1536, 615), 16, mode="once", effects_ahead=True, splits=[1168]),
    }),
}


def core_mask(px):
    """The character itself: opaque pixels that aren't flames or smoke."""
    r, g, b, a = (px[..., i].astype(int) for i in range(4))
    flame = (r > 200) & (g > 110) & (b < 170)
    smoke = (r > 165) & (g > 165) & (b > 165) & (np.abs(r - b) < 40)
    return (a > 200) & ~flame & ~smoke


def label_blobs(mask):
    """8-connected components of mask, as a label array (0 = background)."""
    labels = np.zeros(mask.shape, dtype=np.int32)
    h, w = mask.shape
    count = 0
    for sy, sx in zip(*np.nonzero(mask)):
        if labels[sy, sx]:
            continue
        count += 1
        labels[sy, sx] = count
        queue = deque([(sy, sx)])
        while queue:
            y, x = queue.popleft()
            for ny in (y - 1, y, y + 1):
                for nx in (x - 1, x, x + 1):
                    if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not labels[ny, nx]:
                        labels[ny, nx] = count
                        queue.append((ny, nx))
    return labels, count
DISPLAY_NAMES = {"johny": "JOHNNY"}
# Colour of the exhaust particles the game adds under the jetpack (default neon yellow)
EXHAUST = {"mikasa": "0 240 255", "jinx": "255 40 200"}


def split_frames(pixels, region, effects_ahead=False, splits=()):
    """Frames in region, left to right, as (box, mask) in sheet coordinates.

    Trails link neighbouring frames, so frames are found from their bodies. Every blob
    then goes to the body it overlaps, or else to the next body on its right (trails point
    backwards) or on its left (effects_ahead); the mask keeps a neighbour's trail out.
    """
    x0, y0, x1, y1 = region
    area = pixels[y0:y1, x0:x1]
    columns = core_mask(area).any(axis=0)

    bodies, start = [], None
    for x, filled in enumerate(list(columns) + [False]):
        if filled and start is None:
            start = x
        elif not filled and start is not None:
            if bodies and start - bodies[-1][1] < MERGE_GAP:
                bodies[-1] = (bodies[-1][0], x)
            else:
                bodies.append((start, x))
            start = None
    for split in splits:
        cut = split - x0
        bodies = [part for b in bodies
                  for part in ([(b[0], cut), (cut, b[1])] if b[0] < cut < b[1] else [b])]
    bodies = [b for b in bodies if b[1] - b[0] >= MIN_WIDTH]

    labels, count = label_blobs(area[..., 3] > ALPHA)
    owner = np.zeros(count + 1, dtype=np.int32) - 1
    for blob in range(1, count + 1):
        xs = np.nonzero((labels == blob).any(axis=0))[0]
        overlaps = [max(0, min(b[1], xs[-1] + 1) - max(b[0], xs[0])) for b in bodies]
        if max(overlaps) > 0:
            owner[blob] = int(np.argmax(overlaps))
        elif effects_ahead:
            left = [i for i, b in enumerate(bodies) if b[1] <= xs[0]]
            owner[blob] = left[-1] if left else 0
        else:
            right = [i for i, b in enumerate(bodies) if b[0] >= xs[-1]]
            owner[blob] = right[0] if right else len(bodies) - 1

    frames = []
    for i in range(len(bodies)):
        mask = np.isin(labels, np.nonzero(owner == i)[0])
        ys, xs = np.nonzero(mask)
        box = (xs.min(), ys.min(), xs.max() + 1, ys.max() + 1)
        frames.append(((x0 + box[0], y0 + box[1], x0 + box[2], y0 + box[3]), mask[box[1]:box[3], box[0]:box[2]]))
    return frames


def crop_frame(sheet, box, mask):
    frame = np.array(sheet.crop(box))
    frame[~mask] = 0
    return Image.fromarray(frame)


def body_centre(frame):
    core = core_mask(np.array(frame))
    if not core.any():
        core = np.array(frame)[..., 3] > ALPHA
    ys, xs = np.nonzero(core)
    return xs.mean(), ys.mean()


def slice_character(name, sheet_name, portrait_box, animations, debug_dir):
    sheet = Image.open(ROOT / sheet_name).convert("RGBA")
    pixels = np.array(sheet)
    out = ROOT / name
    out.mkdir(exist_ok=True)

    frames = {}
    for anim, spec in animations.items():
        found = [crop_frame(sheet, box, mask)
                 for box, mask in split_frames(pixels, spec["region"], spec["effects_ahead"], spec["splits"])]
        frames[anim] = [found[i] for i in spec["frames"]] if spec["frames"] else found

    # Body height: the tallest running frame, the game scales it to the player's hitbox
    body = max(f.height for f in frames["run"])

    # Cell big enough for every frame once anchored
    placed = {}
    half_w, top, bottom = 0, 0, 0
    for anim, spec in animations.items():
        for f in frames[anim]:
            cx, cy = body_centre(f)
            # anchor: x of the body centre, y of the feet (ground) or the body centre (air)
            ay = f.height if spec["ground"] else cy + body / 2
            placed.setdefault(anim, []).append((f, cx, ay))
            half_w = max(half_w, cx, f.width - cx)
            top = max(top, ay)
            bottom = max(bottom, f.height - ay)

    cell_w = int(2 * half_w) + 2 * PAD
    cell_h = int(top + bottom) + 2 * PAD
    feet_y = int(top) + PAD
    columns = max(len(v) for v in frames.values())
    grid = Image.new("RGBA", (cell_w * columns, cell_h * len(animations)))

    for row, anim in enumerate(animations):
        for col, (f, cx, ay) in enumerate(placed[anim]):
            grid.alpha_composite(f, (int(col * cell_w + cell_w / 2 - cx), int(row * cell_h + feet_y - ay)))

    grid.save(out / "sheet.png")
    if portrait_box:
        sheet.crop(portrait_box).save(out / "portrait.png")
    else:
        first = frames["run"][0]
        first.crop((0, 0, first.width, first.width)).save(out / "portrait.png")

    lines = [f"name {DISPLAY_NAMES.get(name, name.upper())}", "sheet sheet.png", "portrait portrait.png",
             f"frame {cell_w} {cell_h}", f"feet {feet_y}", f"body {body}"]
    if name in EXHAUST:
        lines.append(f"exhaust {EXHAUST[name]}")
    for row, (anim, spec) in enumerate(animations.items()):
        lines.append(f"{anim} {row} {len(frames[anim])} {spec['fps']} {spec['mode']}")
    (out / "meta.txt").write_text("\n".join(lines) + "\n")
    print(f"{name}: cell {cell_w}x{cell_h}, body {body}px,",
          ", ".join(f"{a} {len(v)}" for a, v in frames.items()))

    if debug_dir:
        overlay = sheet.copy()
        draw = ImageDraw.Draw(overlay)
        for anim, spec in animations.items():
            draw.rectangle(spec["region"], outline=(255, 255, 0, 255))
            for box, _ in split_frames(pixels, spec["region"], spec["effects_ahead"], spec["splits"]):
                draw.rectangle(box, outline=(255, 0, 60, 255))
        overlay.save(Path(debug_dir) / f"{name}_regions.png")
        preview = Image.new("RGBA", grid.size, (40, 36, 56, 255))
        preview.alpha_composite(grid)
        draw = ImageDraw.Draw(preview)
        for row in range(len(animations)):
            draw.line((0, row * cell_h + feet_y, grid.width, row * cell_h + feet_y), fill=(0, 240, 255, 255))
        preview.save(Path(debug_dir) / f"{name}_grid.png")


if __name__ == "__main__":
    debug = sys.argv[1] if len(sys.argv) > 1 else None
    for name, (sheet, portrait, anims) in CHARACTERS.items():
        slice_character(name, sheet, portrait, anims, debug)
