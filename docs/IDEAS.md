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

## The Hunter (Bloodborne), on the branch feature/hunter

Hidden from the game on 2.9 (his line out of `RUNNERS`, his test out): his code stays in.

He makes you rush drones instead of dodging them. Played once (v2.9), not good enough yet: ugly
animations, flat gameplay. What to improve:

**Looks**
- New sheet in (the platformer one): he flies on its glide. His mist is to
  redo: square particles were invisible, soft puffs and two trailing ghosts looked bad (both out). It
  has no flight of its own: a row of him floating forward, coat
  dissolving into mist, would beat the glide. Its attack and shoot rows are drawn bigger, shrunk to fit.
- His faces sheet, for the HUD's living face (only the portrait shows).
- No take-off: the sheet's jump looked wrong. Its attack row cuts twice, only the second is played.
- Unused rows that could serve: Hit/Stun (when his shield breaks), Shoot + Move.
- A staggered titan shows nothing but a puff of silver: it should kneel or flash, so you know it's
  open for the cut.

**Feel**
- Cleaver made heavier (2.2 tiles, its own rasp, a jolt on every swing, a red cut and blood with a
  double hit-stop on a kill), and the visceral playable (lunges 5 tiles, the extended cut, a long
  freeze, the screen flashing red): to judge in a run.
- Bullets: check whether 3, loaded only by cleaver kills, is too stingy to ever use the pistol on
  drones.
- He must feel unlike Mikasa: the other melee runner, with spears for her ultimate.
- The Moonlight Blade is a Fishbones without the recoil. The Old Hunter's bell (a phantom hunter
  fighting at his side for a few seconds) would be his own.

**His kit, as built**
- **Tap F**: Saw Cleaver, very fast, very short.
- **Hold F ~0.3 s, release**: Hunter Pistol shot, if a quicksilver bullet is left. Max 3, and only
  melee kills refill them. A shot staggers a titan (next slash is a visceral: one-shot, bonus
  eddies) and kills a sniper while its sight is frozen (hold during the tracking, let go on the freeze; the
  sight turns silver as the cue). Any time the sight is on him was too easy. Berserk keeps the auto-cleaver.
- **Rally**: when his shield breaks, kill a drone within ~2 s to get it back.
- **Ultimate (E)**: the Moonlight Blade, a wave of blue light razing a band at his height.
- **Flight**: chained quicksteps in the air, a trail of grey mist (the Old Hunter Bone).

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
     coordinates typed by hand, the slowest part of adding the Hunter.
  3. **Reload `tuning.txt` without restarting** (a key in a run), and **the host sending its tuning**
     to whoever joins, so both draw the same game in versus.
  4. **A modding guide** with recipes: change a price, make the game easier, add a runner from a
     ChatGPT sheet, make a map (`maps/` are already text files).
- Not planned: a scripting language for new weapons. A lot of work and bugs for friends who mostly
  want to change numbers and add runners.

## Where we stopped (2026-10-22)

Branch `2.9`, nothing pushed. The Hunter is in and played (new Bloodborne sheet, tap/hold F, parry
coached on the reticle, heavier cleaver, visceral), his list above is what's left on him. The tuning
file is done. Next session: pick from the list just above, then Dante (needs his sprite sheet), then
Adam Smasher.

## Later

- **A mount per runner**, in place of the Attack Titan power-up for everyone: Mikasa rides Eren's
  titan, Jinx Warwick, Johnny his Porsche 911, David goes cyberpsycho. Alex's sheets are in
  `Samuride Files/Animation files/` (JPG with backgrounds and labels: ask for transparent PNGs).
- Bosses at distance milestones (Adam Smasher before 1000 m).
- The ultimates take snipers down too (Johnny's fire wave, Fishbones).
- Real sprites and sounds for the sniper: plain shapes for now.
