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

## Runners that play differently (proposed 2026-10-29)

Everyone flies a jetpack and attacks ahead today: these change how you move, or what dying means.

- **Miles Morales** (Spider-Man): no jetpack. He webs the ceiling and swings like a pendulum; letting
  go at the right moment flings him. The most different of all.
- **Kat** (Gravity Rush): flips gravity, runs on the ceiling, falls back down on the next flip. The
  top rows become her floor.
- **Corvo** (Dishonored): Blink, a teleport 4 tiles ahead, up or down, through a wall of zappers.
  Timing, with a cooldown.
- **Ekko** (Arcane): rewinds 3 s when he dies, once every 400 m. The risk taker.
- **Geralt** (The Witcher): his key alternates two Signs, Aard (a blast that kills drones and pushes
  a titan back) then Quen (a shield for one hit). A rhythm to manage.
- **Scorpion** (Mortal Kombat): his spear pulls a drone, eddies or a power-up to him ("GET OVER HERE").

## Gameplay we don't have yet (proposed 2026-10-29)

1. **Boons, Hades style**: at 250 m and 750 m the run freezes and you pick 1 perk out of 3 for the
   rest of the run (exploding drones, a longer Sandevistan, eddies x1.5 but one hit kills...). A
   small roguelite layer, no art needed.
2. **Close calls**: passing within 0.6 tile of a zapper without dying fills the ultimate gauge and
   shows "CLOSE CALL". Rewards risk; the server already measures that distance.
3. **Daily run**: the same map for everyone that day, with its own best. A reason to come back, and
   friends compare on the same run.
4. **Missions**: 3 goals at a time ("kill 30 drones", "pass Smasher untouched", "800 m with Lucy"),
   paid in eddies.
5. **Events during a run**: a blackout (only the zappers light up), a drone swarm, a stretch where
   gravity flips.
6. **Parry**: the attack key right as one of Smasher's rockets arrives sends it back at him. A skill
   moment during the boss.
