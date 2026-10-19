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
from PIL import Image, ImageDraw, ImageOps

ROOT = Path(__file__).resolve().parent.parent / "assets"
ALPHA = 60          # pixels above this alpha are part of a sprite
MERGE_GAP = 2       # body columns closer than this belong to the same frame
MIN_WIDTH = 30      # thinner bodies are debris, not frames
PAD = 4
SHADOW_ROWS = 12    # bottom rows of a region where ground shadows join the frames


def animation(region, fps, ground=False, mode="loop", frames=None, effects_ahead=False, splits=(), cuts=None):
    """One animation of a sheet.

    region:         box (x0, y0, x1, y1) holding the frames, labels left out
    ground:         feet on the floor (run) rather than flying
    mode:           "loop", "once" (holds the last frame) or "hide" (vanishes after the last frame)
    frames:         indices to keep, all by default
    effects_ahead:  loose effects (muzzle flash, blade arc) belong to the body on their left;
                    by default they are trails and belong to the body on their right
    splits:         sheet x positions where touching frames must be cut apart
    cuts:           for evenly laid sheets whose frames touch (ground shadows): frames are plain
                    columns between boundaries; either the boundaries' x positions, or
                    ("seams", frame centres) to cut at the emptiest column between two centres
    """
    return {"region": region, "fps": fps, "ground": ground, "mode": mode,
            "frames": frames, "effects_ahead": effects_ahead, "splits": splits, "cuts": cuts}


# Titan sheet: frames sit above their numbered labels, cut at the emptiest column between two
TITAN_CUTS = ("seams", [200, 310, 420, 535, 645, 757, 867, 978, 1094, 1207, 1323, 1447])

# Sheet options:
#   key_background: the "transparent" checkerboard is drawn in the picture, light greys become transparent
#   mirror:         frames face left in the game (they run toward the player)
#   effects:        tell flames and smoke apart from the body; off when the body itself has those colours
#   erase:          boxes cleared before slicing, for labels drawn over the rows
#   crops:          {file: box} extra pictures cut as they are, trimmed to their opaque pixels
TITAN_SHEET = {"key_background": True, "mirror": True, "effects": False}

# Eren's transformation: smoke column, titan emerging from it, titan running.
# The smoke clouds touch each other, so that row is cut into even columns.
SHIFT_SHEET = {"effects": False, "erase": [(0, 4, 170, 20), (0, 118, 170, 132), (0, 282, 170, 296)]}
SHIFT_SMOKE_CUTS = [0, 37, 72, 115, 159, 205, 255, 305, 357, 410, 472, 533, 590, 679]

# name: (source sheet, portrait box or None for the head of the first run frame,
#        {animation: animation(...)}[, sheet options])
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
        "rise":    animation((0, 440, 765, 592), 12),
        "hover":   animation((0, 345, 1536, 420), 14),
        "fall":    animation((770, 476, 1536, 592), 12),
        "death":   animation((680, 779, 1480, 874), 10, mode="hide"),
        "attack":  animation((780, 640, 1255, 732), 16, mode="once", effects_ahead=True),
    # The rising heads reach past the "MONTEE" label's top
    }, {"erase": [(10, 446, 98, 477)]}),
    "mikasa": ("sources/mikasa.png", (1140, 925, 1212, 1000), {
        "run":     animation((655, 40, 1240, 144), 12, ground=True),
        "takeoff": animation((0, 182, 640, 308), 16, mode="once"),
        "rise":    animation((650, 185, 1536, 308), 12),
        "hover":   animation((0, 351, 860, 442), 12),
        "fall":    animation((870, 346, 1536, 472), 12),
        "death":   animation((750, 795, 1536, 877), 10, mode="once"),
        "attack":  animation((795, 505, 1536, 615), 16, mode="once", effects_ahead=True, splits=[1168]),
    }),
    # She rides a drone: no takeoff nor death of her own, the game electrocutes her
    "lucy": ("sources/lucy.png", (15, 15, 180, 180), {
        "run":    animation((640, 228, 1515, 340), 12, ground=True),
        "rise":   animation((20, 372, 815, 527), 12, frames=[1, 2, 3, 4, 5, 6]),
        "hover":  animation((20, 228, 620, 340), 10),
        "fall":   animation((835, 385, 1515, 527), 12),
        "attack": animation((20, 575, 758, 690), 16, mode="once"),
    # Her white hair would pass for smoke; a jump frame's head reaches the "JUMP" label's height
    # target.png: the glitch ring marking what her hack will hit
    }, {"effects": False, "erase": [(18, 358, 250, 382)], "crops": {"target.png": (24, 880, 132, 990)}}),
    # Titans charge at the player; Eren's titan is also the transformation power-up
    "titan_female": ("sources/titans.png", None, {
        "run": animation((140, 350, 1536, 520), 14, ground=True, cuts=TITAN_CUTS),
    }, TITAN_SHEET),
    "titan_armored": ("sources/titans.png", None, {
        "run": animation((140, 592, 1536, 756), 12, ground=True, cuts=TITAN_CUTS),
    }, TITAN_SHEET),
    "titan_eren": ("sources/titans.png", None, {
        "run": animation((140, 808, 1536, 970), 14, ground=True, cuts=TITAN_CUTS),
    }, TITAN_SHEET),
    "eren_shift": ("sources/transformation.png", None, {
        "run":    animation((0, 298, 679, 363), 14, ground=True),
        "smoke":  animation((0, 6, 679, 115), 14, ground=True, mode="once", cuts=SHIFT_SMOKE_CUTS),
        "emerge": animation((0, 118, 679, 195), 14, ground=True, mode="once"),
    }, SHIFT_SHEET),
}


def core_mask(px, effects=True):
    """The character itself: opaque pixels that aren't flames or smoke."""
    r, g, b, a = (px[..., i].astype(int) for i in range(4))
    if not effects:
        return a > 200
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
EXHAUST = {"mikasa": "0 240 255", "jinx": "255 40 200", "lucy": "190 70 255"}


def split_frames(pixels, region, effects_ahead=False, splits=(), effects=True):
    """Frames in region, left to right, as (box, mask) in sheet coordinates.

    Trails link neighbouring frames, so frames are found from their bodies. Every blob
    then goes to the body it overlaps, or else to the next body on its right (trails point
    backwards) or on its left (effects_ahead); the mask keeps a neighbour's trail out.
    """
    x0, y0, x1, y1 = region
    area = pixels[y0:y1, x0:x1]
    columns = core_mask(area, effects).any(axis=0)

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


def column_frames(pixels, region, cuts):
    """Frames as the columns between `cuts`, each trimmed to its opaque pixels.

    A neighbour's hand or hair can poke across a boundary: only blobs at least a
    tenth the size of the biggest one (the frame's own body) are kept.
    """
    _, y0, _, y1 = region
    frames = []
    for left, right in zip(cuts, cuts[1:]):
        labels, count = label_blobs(pixels[y0:y1, left:right, 3] > ALPHA)
        sizes = np.bincount(labels.ravel(), minlength=count + 1)
        sizes[0] = 0
        mask = np.isin(labels, np.nonzero(sizes >= sizes.max() / 10)[0])
        ys, xs = np.nonzero(mask)
        box = (xs.min(), ys.min(), xs.max() + 1, ys.max() + 1)
        frames.append(((left + box[0], y0 + box[1], left + box[2], y0 + box[3]),
                       mask[box[1]:box[3], box[0]:box[2]]))
    return frames


def seam_cuts(pixels, region, centres):
    """Boundaries at the emptiest column between consecutive frame centres, ground shadows ignored."""
    x0, y0, x1, y1 = region
    opaque = (pixels[y0:y1 - SHADOW_ROWS, :, 3] > ALPHA).sum(axis=0)
    cuts = [x0]
    for left, right in zip(centres, centres[1:]):
        cuts.append(left + int(np.argmin(opaque[left:right])))
    return cuts + [x1]


def find_frames(pixels, spec, effects):
    cuts = spec["cuts"]
    if cuts and cuts[0] == "seams":
        cuts = seam_cuts(pixels, spec["region"], cuts[1])
    if cuts:
        return column_frames(pixels, spec["region"], cuts)
    return split_frames(pixels, spec["region"], spec["effects_ahead"], spec["splits"], effects)


def crop_frame(sheet, box, mask):
    frame = np.array(sheet.crop(box))
    frame[~mask] = 0
    return Image.fromarray(frame)


def key_background(sheet):
    """Light, colourless pixels (a drawn checkerboard) become transparent."""
    px = np.array(sheet)
    rgb = px[..., :3].astype(int)
    grey = (rgb.max(axis=2) - rgb.min(axis=2) <= 16) & (rgb.min(axis=2) >= 185)
    px[grey, 3] = 0
    return Image.fromarray(px)


def body_centre(frame, effects=True):
    core = core_mask(np.array(frame), effects)
    if not core.any():
        core = np.array(frame)[..., 3] > ALPHA
    ys, xs = np.nonzero(core)
    return xs.mean(), ys.mean()


def slice_character(name, sheet_name, portrait_box, animations, options, debug_dir):
    sheet = Image.open(ROOT / sheet_name).convert("RGBA")
    effects = options.get("effects", True)
    if options.get("key_background"):
        sheet = key_background(sheet)
    for box in options.get("erase", []):
        sheet.paste((0, 0, 0, 0), box)
    pixels = np.array(sheet)
    out = ROOT / name
    out.mkdir(exist_ok=True)

    frames = {}
    for anim, spec in animations.items():
        found = [crop_frame(sheet, box, mask) for box, mask in find_frames(pixels, spec, effects)]
        if options.get("mirror"):
            found = [ImageOps.mirror(frame) for frame in found]
        frames[anim] = [found[i] for i in spec["frames"]] if spec["frames"] else found

    # Body height: the tallest running frame, the game scales it to the player's hitbox
    body = max(f.height for f in frames["run"])

    # Cell big enough for every frame once anchored
    placed = {}
    half_w, top, bottom = 0, 0, 0
    for anim, spec in animations.items():
        for f in frames[anim]:
            cx, cy = body_centre(f, effects)
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
    for file, box in options.get("crops", {}).items():
        picture = sheet.crop(box)
        alpha = picture.getchannel("A").point(lambda a: 255 if a > ALPHA else 0)
        picture.crop(alpha.getbbox()).save(out / file)
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
            for box, _ in find_frames(pixels, spec, effects):
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
    for name, (sheet, portrait, anims, *options) in CHARACTERS.items():
        slice_character(name, sheet, portrait, anims, options[0] if options else {}, debug)
