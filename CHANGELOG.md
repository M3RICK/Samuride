# Patch notes

Each release on the [Releases page](https://github.com/M3RICK/Samuride/releases) shows its own notes,
taken from this file. Players on different versions can't play together: update both sides.

## v2.9

### Balance
- **Easier runs, closer to Jetpack Joyride.** The run starts a bit slower and speeds up far more gently:
  top speed is lower and only reached around 2300 m instead of 800 m. Obstacles get harder over ~2000 m
  instead of ~500 m.
- **Fewer tall zappers.** 4-tile zappers only show up past ~1300 m and stay rare; most beams are 2 or 3 tiles.

## v2.85

### New
- **Voice lines.** Johnny's gun now fires with his real Malorian shot, and he speaks when he fires his
  ultimate. Jinx throws one of six lines ("Bye bye!", "See ya!", "Watch this!"...) when she launches Fishbones.

## v2.8

### New
- **A personal best per mode**: CLASSIC and CYBERPSYCHO each keep their own best distance, and CYBERPSYCHO
  also keeps your most drones down in one run. Your old best carries over as the CLASSIC one.
- **Mikasa's face comes alive in the HUD**, Doom style: she turns her head to look ahead, frowns on a kill and
  cries out after three in a row, smiles at a power-up, flinches when her shield breaks, glares at an
  incoming titan, shouts her ultimate and bleeds when she falls.
- **Jinx's face too**: a grin on every kill, a mad cackle on a streak, a wink at a power-up, a laugh as she
  fires her ultimate.
- **A round portrait at the bottom centre**, with your ultimate's gauge filling a ring around it (a notch per
  thunder spear while Mikasa holds them).

### Balance
- **Mikasa's thunder spears hit harder**: 5 spears instead of 3, and each lightning column is 5 tiles wide
  instead of 3.
- **Lucy hacks more**: 3 charges of RAM instead of 2, one back every 3.5 s instead of 6, and a hacked
  zapper stays off 2 s instead of 1.2. A drone she hacks passes the hack on: the nearest drone within
  4 tiles of it goes down too.
- **The eddie magnet reaches less**: half a tile per level instead of one, all around you. At level 3 it
  used to sweep almost the whole height of the screen; now it grabs the eddies you graze.
- **A second to get clear**: you stay invincible for 1 s after the Sandevistan wears off, and 1 s more
  after bursting out of the titan.

### Fixes
- **No more haze at top speed**: past ~800 m the speed streaks were so many they blurred the screen. Half
  as many now, and fainter.

### Other
- **Lucy's RAM shows over her head**, as red bars like in Cyberpunk 2077, where you can read it without looking away from the run.
- **Lucy's hack lands with a bang**: a crackling bolt from her hand to the drone (and on to the next one when
  it jumps), the drone glitching apart in purple, and a jolt of the screen.
- **Mikasa throws her thunder spears with `E`** (the SPEAR button on phones), the key that called them: her
  blade stays on the attack key, so Berserk keeps slashing while she holds spears.
- **You blink while you can't be hurt**, for the second after a broken shield, the Sandevistan or the titan.
- **The eddie magnet pulls**: the eddies it grabs now fly into you instead of vanishing where they
  were.
||||||| parent of 1bd16eb (Johnny fires his recorded Malorian and speaks on his ultimate, Jinx shouts a line when she launches Fishbones)

## v2.7

### New
- **Upgrades**, in a new shop from the main menu. They apply to every runner, and they cost a lot on
  purpose: a reward for playing a lot. **Sandevistan** +0.5 s per level, **Ultimate** charges 10
  eddies sooner per level, **Eddie magnet** picks up eddies 1 tile further per level (3 levels each:
  1000, 2500 and 5000 eddies), and **Subdermal armor** starts every run with a shield (5000 eddies).

- **Hits you can feel.** When Mikasa's blade cuts a drone down, the world catches for a blink, the cut
  flashes across the drone and the screen jolts. Every bullet of Jinx's Pow-Pow kicks, and the screen shakes
  harder or lighter depending on the hit.

- **Fishbones is a real shark rocket now**, with the blue flame and trail of Arcane.

### Balance
- **Fishbones charges, then kicks**: Jinx stops dead in the air for 0.6 s while the rocket charges
  (nothing protects her meanwhile), then the recoil throws her back and she picks her pace up again.

### Fixes
- Taking a whole row of eddies at once no longer slows phones down: the eddies count up on a single
  "+N" instead of one "+1" each, and the sounds don't pile up.

## v2.6

### New
- **Android.** Samuride now runs on Android phones: `samuride-<version>-android.apk` on the Releases
  page. Hold anywhere on the screen for the jetpack; in CYBERPSYCHO, buttons at the bottom right
  attack and fire your ultimate. Android 8 or newer, on any phone of the last ten years.

### Fixes
- Explosions and ultimates draw much faster: fewer slowdowns when the screen fills with sparks.

## v2.5

### New
- **Ultimates** (CYBERPSYCHO). Every eddy you earn charges your ultimate, shown at the top right;
  once it's full, it lights up and `E` (set it in Options) fires it, its name across the screen. First one: **Johnny** palms
  his gun and a wall of fire sweeps the screen ahead, burning every drone, zapper and titan in its
  way. **David** fires his arm cannon at the ground ahead: half a second later a mini-nuke razes
  16 tiles from floor to ceiling. **Lucy** breaches the network: every drone on screen goes down and
  every zapper on screen goes off for 3 seconds. **Mikasa** gets 3 thunder spears, shown over her head: her next attacks
  fly ahead, stick in the first drone or titan and blow up in a lightning column from floor to
  ceiling. **Jinx** fires Fishbones: a big shark rocket razes a band of the corridor at her height,
  and she can fly through it.

### Balance
- **Jinx swaps her rockets for Pow-Pow**, her minigun: bursts of three small fast bullets, and a
  steady spray while Berserk. The rockets became her ultimate.
- **New music for the runs**: a 1:40 punk track instead of the short loop. That loop now plays in
  the menu, once Johnny's greeting is over.

## v2.4

### New
- **Lucy**, the netrunner, rides in on her drone (750 eddies, after Mikasa). In CYBERPSYCHO she
  doesn't shoot, she hacks what's in her way: the first zapper ahead of her goes off for
  1.2 seconds, or the first drone ahead of her goes down. Each hack takes one of her 2 charges of RAM, and a charge comes back every
  6 seconds: her RAM shows next to her eddies, and a glitch ring marks what her next hack will hit.
- **Districts are bought with eddies too.** The Corridor and Night City stay free; The Walls (150),
  Zaun (300), Worlds Collide (500) and Both Worlds (750) are bought from the district select, in
  any order. Only the host needs to own the district: whoever joins plays it anyway.

- **Berserk**, a new CYBERPSYCHO power-up (gold crossed blades): for 8 seconds you attack on your
  own, twice as often, and Lucy's RAM comes back three times faster. Drones drop it too.

### Other
- The shield is now green, so it no longer looks like a zapper, and the Sandevistan is now magenta.

## v2.3

### New
- **Eddies are money.** Every run's eddies go to your wallet. Johnny is free, then unlock David
  (100), Jinx (250) and Mikasa (500), in that order, from the character select.
- **Fullscreen**, in Options. Black bars keep the picture whole on screens that aren't 16:9.
- The titan warning is now a red column as tall as the titan coming, instead of a small `!`.

### Balance
- No titan before 300 m (was 150 m).
- Jinx's rockets are bigger, and rockets and bullets hit drones slightly above or below their line:
  they hit what they look like they hit.

### Fixes
- Jinx's head vanished while she was climbing.
- A shot kept flying through a drone it had just destroyed.
- Johnny's greeting looped in the menu and the lobby: it now plays once, when the main menu opens.
- With the mouse, the volume could only go up. Click the bar to set it, left of it to mute, or use
  the wheel.
- A slow connection could scramble a versus game for good.
- Joining an address where nobody answers froze the game for minutes: it now gives up after
  3 seconds.
- When your choom left after a game, you waited forever for a rematch: the next choom who joins
  now plays you.

### Other
- The game is now called `Samuride` and the dedicated server `Samuride_server`
  (were `jetpack_client` and `jetpack_server`).

## v2.2 — 2026-10-19

### New
- **Subdermal shield**: takes one deadly hit for you (zapper, drone or titan), then gives you a
  second to get clear.
- **Sandevistan**: a 3-second sprint nothing can hurt, leaving afterimages behind.
- In CYBERPSYCHO, one drone in three drops a shield or a Sandevistan when shot down.

### Fixes
- The character and district you picked were sometimes ignored (Johnny in the corridor).
- Rockets and bullets followed your jetpack up and down after being fired.

## v2.1.3 — 2026-10-19
- macOS: the game ships as `Samuride.app` and no longer opens a terminal.

## v2.1.1 — 2026-10-19
- macOS: fixed the package build.

## v2.1 — 2026-10-19

### New
- **macOS build** (Apple Silicon, macOS 13.3 or newer).

### Balance
- The Attack Titan orb is rarer and the titan form lasts 4 seconds.

## v2.0 — 2026-10-18

The first public release: a two-player Jetpack Joyride in Night City.

- Endless run, solo or versus over the network, chasing your best distance.
- Four runners: Johnny, David, Jinx and Mikasa.
- Five districts, or two of them taking turns.
- **CYBERPSYCHO** mode: gun down Arasaka drones for bounty.
- **Titans** charge along the floor, and the **Attack Titan** orb lets you run as Eren's titan.
- Options: volumes and keys.
- Linux and Windows builds.
