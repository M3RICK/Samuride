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

## The Hunter (Bloodborne), built in v2.9

He makes you rush drones instead of dodging them. Played once (v2.9), not good enough yet: ugly
animations, flat gameplay. What to improve:

**Looks**
- New sheet in (the platformer one): he takes off on its jump, flies on its glide, and his mist is
  drawn by the game as particles. It has no flight of its own: a row of him floating forward, coat
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

## Later

- **A mount per runner**, in place of the Attack Titan power-up for everyone: Mikasa rides Eren's
  titan, Jinx Warwick, Johnny his Porsche 911, David goes cyberpsycho. Alex's sheets are in
  `Samuride Files/Animation files/` (JPG with backgrounds and labels: ask for transparent PNGs).
- Bosses at distance milestones (Adam Smasher before 1000 m).
- The ultimates take snipers down too (Johnny's fire wave, Fishbones).
- Real sprites and sounds for the sniper: plain shapes for now.
