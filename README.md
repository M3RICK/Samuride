<p align="center">
  <img src="https://github.com/user-attachments/assets/cbadfc9f-8b40-41c6-b83a-08a557ad397a" alt="Samuride" width="500px">
</p>

<h3 align="center"><em>Wake the fuck up, Samurai. We have a city to burn.</em></h3>

<p align="center">
  Endless online Jetpack Joyride, rewired with a Cyberpunk 2077 soul.<br>
  C++20 · SFML 3 · custom binary TCP protocol · authoritative 60 Hz server
</p>

---

<p align="center"><img src="docs/gameplay.webp" alt="Gameplay: Mikasa and Jinx in CYBERPSYCHO" width="720px"></p>

<p align="center"><img src="docs/versus.png" alt="Two players: Jinx and Mikasa in CYBERPSYCHO" width="720px"><br>
<em>Two runners, one corridor: Jinx and Mikasa in CYBERPSYCHO.</em></p>

## The run

You sprint forward on your own, the jetpack is the only thing you control. The corridor never ends: it is generated on the fly, faster and nastier the further you go.

- **Solo**: run as far as you can. Your best distance is saved.
- **Versus**: two runners, one corridor. First one to get zapped loses.
- **Eddies** (coins) come in blocks and waves, collected per player: what your choom grabs is still there for you.
- **Zappers** come as vertical, horizontal or diagonal beams. One touch and you flatline.
- **Titans** charge along the floor past 150 m: a blinking `!` warns you, then fly over them or get stomped. In CYBERPSYCHO, Mikasa can take their nape for +20.
- **Two modes**: CLASSIC, the original run, or **CYBERPSYCHO**: Arasaka drones block the corridor and you gun them down for +5 eddies each. Johnny and David shoot, Jinx fires her shark rockets, Mikasa slices with her blades.
- **Six districts**: The Corridor; Night City, The Walls and Zaun, three panoramas from sunset to night; Worlds Collide, running through all three back to back; and Both Worlds, switching between the corridor and Worlds Collide every 250 m like day and night. The host picks.
- **Four runners**: Johnny, David, Jinx on her shark rocket and Mikasa, each with their own run, take-off, flight and death animations.
- After a game, hit `Space` for an instant rematch.

| Key | Action |
| --- | --- |
| `Space` (hold) | Jetpack (default) |
| `F` | Attack in CYBERPSYCHO (default) |
| `Esc` | Back to menu |

Keys and volumes can be changed in **Options**; they are kept in `~/.local/share/samuride/settings`.
| Arrows / mouse + `Enter` | Menu |

<p align="center"><img src="docs/menu.png" alt="Menu" width="600px"></p>
<p align="center"><img src="docs/characters.png" alt="Character select" width="600px"></p>
<p align="center"><img src="docs/districts.png" alt="District select" width="600px"></p>

## Download

Grab `samuride-<version>-linux-x86_64.tar.gz` from the [Releases](../../releases) page, then:

```sh
tar xzf samuride-*-linux-x86_64.tar.gz
cd samuride-*-linux-x86_64
./jetpack_client
```

Everything it needs is in the archive. Runs on x86_64 Linux with glibc 2.39 or newer (Ubuntu 24.04+, Fedora 40+, Arch...).

## Build

Linux, a C++20 compiler and **SFML 3**.

```sh
sudo pacman -S sfml      # Arch / CachyOS
make                     # jetpack_server + jetpack_client
make tests               # protocol, map streaming & generator self-checks
```

SFML 3 installed somewhere else? Point the Makefile at it: `make SFML_DIR=/path/to/sfml`.
`tools/package_release.sh` builds the same self-contained archive as the Releases page.

## Play

```sh
./jetpack_client
```

That's it: **Solo** and **Host** start a server in the background for you. To play with a friend on your network, one picks **Host a run**, the other **Join a run** and types the address shown on the host's screen.

A dedicated server and direct connection are available too:

```sh
./jetpack_server -p 4242 [-m maps/small_good.txt] [-s] [-d]
./jetpack_client -h 127.0.0.1 -p 4242 [-c 2] [-d]
```

`-c` picks the character (0 Johnny, 1 David, 2 Jinx, 3 Mikasa), `-m` starts the run on a hand-made map before going procedural, `-s` is solo mode, `-d` prints debug logs (packets, inputs).

## Under the hood

- **Server**: single `poll()` loop with a fixed 60 Hz tick. It owns physics, collisions, scoring and map generation; clients only send "jetpack on/off".
- **Endless map**: generated in 40-column chunks ahead of the leader and streamed to clients as the run goes on. Patterns get denser over time, and every column always leaves room to get through.
- **Client**: a network thread fills the game state, the main thread renders it at 60 FPS, smoothing positions between server ticks.
- **Characters**: `tools/slice_sheets.py` turns hand-laid sprite sheets (labels, uneven spacing, trails overlapping the next frame) into aligned grids plus a `meta.txt` per character. Frames are found from the characters' bodies, every blob is assigned to its owner, and each frame is re-anchored on the feet or the body centre so animations don't wobble.
- **Protocol**: 4-byte header (1 byte type + 3 bytes payload size), packets reassembled from the TCP stream. Full spec in [`doc.txt`](doc.txt).

### Hand-made maps

Plain text, one character per tile, all lines the same length:

| Char | Tile |
| --- | --- |
| `_` | empty |
| `c` | eddie (coin) |
| `e` | zapper |
| `d` | drone (CYBERPSYCHO) |

## Crew

Aymeric — started as an Epitech project (B-NWP jetpack).

Rajdhani font by Indian Type Foundry, under the SIL Open Font License.

Non-commercial fan project. Johnny Silverhand and David Martinez belong to CD Projekt, Jinx to Riot Games, Mikasa Ackerman to Hajime Isayama / Kodansha. Not affiliated with any of them.
