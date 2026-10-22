# Ideas

Where Samuride goes next. The rule: keep Jetpack Joyride's feel (fair deaths, warned threats, a run
that gets harder slowly), but not its looks. Every obstacle comes from our universes: Cyberpunk,
Arcane, Attack on Titan.

## Balance, from Jetpack Joyride

- **Difficulty comes from variety, not speed.** Speed climbs gently and caps early (top speed around
  1500 m, obstacles at their hardest around 1200 m). What kills is the mix of obstacles.
- **Every death must feel avoidable.** No wall you can't pass, a warning before anything fast.
- **Long zappers stay rare**: the shorter of two rolls, 4 tiles only past ~800 m.
- **Second chances** keep long runs alive: shield, Sandevistan, the Attack Titan.

## Obstacles

- **Arasaka sniper** (v2.9). A red laser sight crosses the screen and follows your height, freezes,
  then a railgun shot sweeps the whole row. Same reflex as Jetpack Joyride's missile, nothing flies
  at you.
- ~~Security gates~~: dropped. A barrier switching on and off on a timer is luck in a runner: you
  can't slow down to wait for it. Any timed obstacle must be readable from where you dodge it.
- **One obstacle per district**, so the scenery changes the run, not just its colours:
  - Piltover: hextech lines
  - Zaun: chemical gas rising from the floor
  - Night City, The Walls: to find

## The Hunter (Bloodborne): dropped

Dropped on 2026-10-23: no sprite sheet looked good enough. Everything about him is on the branch
`feature/hunter`. What was liked: the ultimate, the Old Hunter's bell (a phantom of him flying ahead
and cutting drones), to reuse for Dante's Doppelganger.

## Easy to change, for friends testing the balance

The goal: friends who never coded change the game and send real feedback. In English.

- **Done (v2.9): `assets/tuning.txt`.** Every balance value, explained, read at startup by the game
  and the server. CUSTOM TUNING on the HUD and no solo records while a value differs. The values live
  in `src/common/tuning.cpp`; after changing a default in the code, `./Samuride_server -t >
  assets/tuning.txt` (a test fails until then).
- **Next, in this order:**
  1. **Runners in a data file** (`assets/runners.txt`): folder, price, weapon, ultimate, charge. A new
     runner is a folder and a line; any runner can take any existing weapon or ultimate. A new
     mechanic (Dante's style meter) still needs code.
  2. **A web page to slice sprite sheets**: drop the sheet, draw a box per animation, pick its role,
     watch it play, set fps and scale, export what `tools/slice_sheets.py` reads. Replaces the pixel
     coordinates typed by hand, the slowest part of adding a runner.
  3. **Reload `tuning.txt` without restarting** (a key in a run), and **the host sending its tuning**
     to whoever joins, so both draw the same game in versus.
  4. **A modding guide** with recipes: change a price, make the game easier, add a runner from a
     ChatGPT sheet, make a map (`maps/` are already text files).
- Not planned: a scripting language for new weapons. A lot of work and bugs for friends who mostly
  want to change numbers and add runners.

## Where we stopped

See `docs/NEXT.md`.

## Later

- **A mount per runner**, in place of the Attack Titan power-up for everyone: Mikasa rides Eren's
  titan, Jinx Warwick, Johnny his Porsche 911, David goes cyberpsycho. Alex's sheets are in
  `Samuride Files/Animation files/` (JPG with backgrounds and labels: ask for transparent PNGs).
- Bosses at distance milestones (Adam Smasher before 1000 m).
- The ultimates take snipers down too (Johnny's fire wave, Fishbones).
- Real sprites and sounds for the sniper: plain shapes for now.
