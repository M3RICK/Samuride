# Samuride protocol

Samuride is a two-player Jetpack Joyride over TCP. The server is authoritative: it runs the physics,
collisions, scoring and map generation, and clients only send their inputs. The run is endless and
ends at the first death.

## Packets

Every packet is a 4-byte header followed by its payload:

| Bytes | Field |
| --- | --- |
| 1 | Message type |
| 3 | Payload size, big endian |

TCP is a stream: a single `recv` can hold part of a packet or several packets, so both sides
reassemble packets from a buffer before reading them. All multi-byte numbers are big endian.
Positions travel in hundredths of a tile.

## Message types

| Type | Name | Direction | Purpose |
| --- | --- | --- | --- |
| 1 | `MSG_CONNECT` | client → server | First message after connecting |
| 2 | `MSG_MAP_DATA` | server → client | Map columns |
| 3 | `MSG_GAME_START` | server → client | A game is about to start |
| 4 | `MSG_PLAYER_INPUT` | client → server | Jetpack on or off |
| 5 | `MSG_GAME_STATE` | server → client | Players, every tick |
| 6 | `MSG_COLLISION` | server → client | Something was hit or collected |
| 7 | `MSG_GAME_END` | server → client | The game is over |
| 8 | `MSG_COUNTDOWN` | server → client | Countdown before the run |
| 9 | `MSG_READY` | client → server | Rematch request |
| 10 | `MSG_ATTACK` | client → server | CYBERPSYCHO attack |
| 11 | `MSG_SHOT` | server → client | Someone attacked, for the animation |
| 12 | `MSG_TITANS` | server → client | Titans, every tick |
| 13 | `MSG_ULTIMATE` | both | Client: fire my ultimate. Server: someone did |
| 14 | `MSG_SNIPERS` | server → client | Arasaka snipers aiming and firing, every tick |
| 15 | `MSG_BOSS` | server → client | Adam Smasher: where he is, his health, his attack, every tick |
| 16 | `MSG_VERSION` | both | Client: my protocol version, before `MSG_CONNECT`. Server: not mine, which is this |
| 17 | `MSG_TRAUMA` | client → server | My answer to the Trauma Team's offer (`O`): 1 call it, 0 let me die |
| 18 | `MSG_RIPPERDOC` | client → server | The implant I buy at the Ripperdoc (`R`): 1 to 3, 0 none. One I can't afford is ignored, I'm still choosing |

### `MSG_VERSION`

| Bytes | Field |
| --- | --- |
| 1 | `PROTOCOL_VERSION` (`src/common/protocol.hpp`), bumped whenever a packet changes |

The client sends it right after connecting, before `MSG_CONNECT`. When it doesn't match the
server's, or never came (a build from before it existed), the server answers its own version to
`MSG_CONNECT` and hangs up: the client tells the player which side has to update.

### `MSG_CONNECT`

| Bytes | Field |
| --- | --- |
| 1 | Runner: 0 Johnny, 1 David, 2 Jinx, 3 Mikasa, 4 Lucy, 5 Dante (the order of `src/common/runners.hpp`) |
| 1 | District: 0 The Corridor, 1 Worlds Collide, 2 Night City, 3 The Walls, 4 Zaun, 5 Both Worlds; `0xFF` when joining |
| 1 | Mode: 0 CLASSIC, 1 CYBERPSYCHO; `0xFF` when joining |
| 1 per upgrade | Upgrade levels, in the order of `src/common/upgrades.hpp`: Sandevistan, eddie magnet, subdermal armor, Trauma Team, Lucky Chip, Titan Serum, Drone Pinata |

District and mode are only taken from the host: the first player in the lobby, whoever their
`MSG_CONNECT` came after, or the one who stayed when the host left. Whoever joins sends `0xFF` for both. The server caps each upgrade level at what the shop sells. A game only starts once every player in the lobby sent theirs, and
`MSG_CONNECT` is ignored outside the lobby.

### `MSG_MAP_DATA`

| Bytes | Field |
| --- | --- |
| 4 | Offset: first column of this chunk |
| 4 | Chunk width |
| 4 | Map height |
| width × height | Tiles, one byte each, row by row from the top |

An offset of 0 replaces the client's map; any other offset must equal the client's current width,
and the chunk is appended. The whole map is sent right before each `MSG_GAME_START`, then new
chunks are streamed while the players advance.

Tiles:

| Byte | Tile |
| --- | --- |
| `_` | Empty |
| `c` | Coin (eddie), +1 |
| `e` | Zapper, deadly |
| `d` | Drone (CYBERPSYCHO), deadly, +5 when destroyed |
| `p` | Attack Titan orb |
| `a` | Shield: takes one deadly hit |
| `v` | Sandevistan: 3 s of sprint at 1.7× the run speed, invincible |

A destroyed drone's tile can hold a shield or a Sandevistan: one in three drops one, picked by a hash
of its position (`droneDrop` in `src/common/powerups.hpp`), so clients know without being told.

Collected coins, orbs and power-ups stay in the map: the server tracks who took what, so both players can
take the same one.

### `MSG_GAME_START`

| Bytes | Field |
| --- | --- |
| 1 | Your player number |
| 1 | Player count |
| 2 per player | Player number, character |
| 1 | District |
| 1 | Mode |

Sent to each client once the lobby is full (two players, one in solo), and again on a rematch.

### `MSG_COUNTDOWN`

| Bytes | Field |
| --- | --- |
| 1 | Seconds left: 3, 2, 1, then 0 when the run starts |

### `MSG_PLAYER_INPUT`

| Bytes | Field |
| --- | --- |
| 1 | 1 jetpack on, 0 off |

Sent when the input changes, not every frame.

### `MSG_GAME_STATE`

A 13-byte record per player, 60 times per second:

| Bytes | Field |
| --- | --- |
| 1 | Player number |
| 4 | X, also the distance run |
| 2 | Y: top of the 2-tile-tall body, 0 is the top row |
| 2 | Score |
| 1 | Flags: bit 0 jetpack, bits 1-2 form (0 normal, 1 shifting, 2 Attack Titan, 3 emerging), bit 3 shield, bit 4 Sandevistan, bits 5-6 the weapon's charges (Lucy's RAM, Wraith's Blinks), bit 7 Berserk |
| 1 | Ultimate gauge, in percent |
| 1 | Mikasa's thunder spears left |
| 1 | Flags: bit 0 grace, invincible for a second after a broken shield, the Sandevistan or the titan; bits 1-3 Dante's style rank, 0 (D) to 6 (SSS); bits 4-6 the NCPD wanted level, 0 to 5 stars |

### `MSG_COLLISION`

| Bytes | Field |
| --- | --- |
| 1 | Type, see below |
| 1 | Player number |
| 4 | Tile column |
| 2 | Tile row |

| Type | Meaning |
| --- | --- |
| `c` | Coin collected |
| `e` | Zapper or drone contact: death |
| `d` | Drone destroyed, +5 |
| `t` | Run down by a titan: death |
| `s` | Hit by an Arasaka sniper's railgun: death |
| `k` | Titan taken down (Mikasa's blade, the Attack Titan or Johnny's fire wave), +20 |
| `p` | Attack Titan orb taken: shifting starts |
| `a` | Shield taken |
| `v` | Sandevistan taken: the sprint starts |
| `r` | Berserk taken: 8 s of attacking on your own, twice as often |
| `l` | One of Mikasa's thunder spears blew up at that column: everything 2 tiles around it, floor to ceiling, goes down |
| `h` | Lucy hacked the zapper with a node on that tile: all its nodes are off for 2 s |
| `j` | Lucy's hack jumped from the drone she hacked (the last `d` of that player) to the drone on that tile; that drone's `d` follows |
| `b` | A shield took a deadly hit (zapper, drone, titan or sniper) instead of the player, who gets 1 s of invincibility |
| `O` | A deadly hit on a player who has the Trauma Team (upgrade): the run waits up to 5 s for their `MSG_TRAUMA`, then counts it a yes. No: the hit's own death type follows |
| `R` | CYBERPSYCHO, the leader past Smasher's arena: the Ripperdoc opens for every player, the run waits up to 10 s for their `MSG_RIPPERDOC` |
| `I` | A player left the Ripperdoc: x is the implant bought, 1 to 3, or 0 for none |
| `T` | The Trauma Team (upgrade) picked the player up instead of a death: the run waits 3.5 s while it works, then 2 s of invincibility, half the run's eddies billed |
| `J` | The eddy just taken on that tile hit the Lucky Chip's jackpot (upgrade): +10 in all |
| `P` | The drone on that tile, just destroyed, drops a power-up thanks to the Drone Pinata (upgrade): `droneDrop(x, y, true)` |

### `MSG_GAME_END`

| Bytes | Field |
| --- | --- |
| 1 | Winner's player number, or `0xFF` for none |

The game ends at the first death (zapper, drone, titan or sniper, unless a shield takes it): the other player wins, nobody if both
died on the same tick. If a player disconnects, the remaining one wins. In solo the winner is
always `0xFF` and the score is the distance run.

### `MSG_READY`

Empty. Sent after `MSG_GAME_END` to ask for a rematch. Once every connected player is ready, the
server sends a fresh `MSG_MAP_DATA` (offset 0) and `MSG_GAME_START` to everyone.

### `MSG_ATTACK`

One byte: 1 when the attack key goes down (also when empty), 0 when it comes back up, which the
server ignores. CYBERPSYCHO only (ignored otherwise). The server enforces the weapon cooldown. Johnny
and David shoot guns, Jinx her minigun in bursts of 3 (a steady spray in Berserk): a projectile flies ahead at run speed plus the weapon's speed, for
up to 22 tiles. Mikasa's blade destroys every drone and titan within 2.5 tiles ahead. Lucy hacks
the first drone or zapper in her way (7 tiles ahead, from a tile above her head to a tile below her
feet), for one of her 2 RAM charges; nothing in her way, nothing spent. Wraith blinks, for one of his 2
charges: 4 tiles ahead; the drones his body crosses go down (`d`), Adam Smasher's rockets it crosses
turn back at him, and he takes a stab if it reaches him. In the last 0.3 s before Smasher's slam or
cannon hits Wraith, the Blink is perfect (`f`): no charge spent, untouchable until that strike is over.
Berserk never blinks on its own.

### `MSG_SHOT`

| Bytes | Field |
| --- | --- |
| 1 | Player number |
| 1 | Weapon: 0 gun, 1 minigun, 2 blade, 3 hack, 4 thunder spear, 6 blink, sent before he moves (Dante's attacks go out as 0 in the air, 2 on the ground) |

### `MSG_ULTIMATE`

From the client: empty, CYBERPSYCHO only, ignored unless the gauge is full (100 eddies earned since
the last one) and the runner has an ultimate. From the server, to everyone:

| Bytes | Field |
| --- | --- |
| 1 | Player number |
| 1 | Ultimate: 1 fire wave (Johnny), 2 nuke (David), 3 breach (Lucy), 4 thunder spears (Mikasa), 5 Fishbones (Jinx), 6 Doppelganger (Dante), 7 Stillness (Wraith) |
| 4 | X where it starts |
| 2 | Y of its line: the middle of the runner's body |

The fire wave sweeps 25 tiles ahead at 45 tiles per second: every column it crosses burns from
floor to ceiling. Drones go down (`d`), zappers go out for the rest of the run, titans die (`k`).
Clients burn the zappers themselves from the start and the speed.

The nuke lands 0.5 s after it is fired, 10 tiles ahead of X, and razes 8 tiles on each side the same
way, all at once.

The breach covers 2 tiles behind X to 15 tiles ahead, at once: drones go down (`d`), zappers go off
for 3 s (burnt ones stay out). Clients turn the zappers off themselves from X.

The thunder spears replace Mikasa's next 3 attacks: each flies like a projectile (run speed + 28) until
it sticks in a drone or a titan, or reaches 22 tiles, then blows up (`l`).

Fishbones sweeps 30 tiles ahead at 40 tiles per second like the fire wave, but only the rows within
2.5 tiles of Y: a zapper touching them goes out whole, a titan dies if the band reaches its height.

The Doppelganger sends nothing more: for 8 seconds each of Dante's attacks is also made from the
opposite height (the corridor's height minus his body's, minus his y), and the clients draw his double
there and its shots themselves. Its kills come as his.

Stillness sends nothing more: for 4 seconds the server stops the titans, the snipers and Adam Smasher
(his x and his rockets stay where they are). Their states keep coming, standing still.

### `MSG_TITANS`

| Bytes | Field |
| --- | --- |
| 1 | Count |
| 5 per titan | Type (0 Female, 4.5 tiles tall and fast; 1 Armored, 4 tiles tall and slower), X of its left edge |

Titans stand on the floor and run at the players. The first one comes past 300 tiles, then one
every 7 to 14 seconds, appearing 40 tiles ahead of the leader.

### `MSG_SNIPERS`

| Bytes | Field |
| --- | --- |
| 1 | Count |
| 3 per sniper | Phase (0 tracking, 1 aiming, 2 firing), Y of its line (2 bytes) |

Arasaka snipers shoot in both modes. The first one aims past 150 tiles, then one every 5 to 10
seconds, at a random player: for 1.2 s its line follows the middle of that player, then it stays
still for 0.6 s, then the railgun fires for 0.25 s along the whole row, everyone in it included.

### `MSG_BOSS`

| Bytes | Field |
| --- | --- |
| 1 | 1 while he fights, 0 before his arena and once he is gone |
| 1 | Health, in percent |
| 1 | Attack: 0 none, 1 rockets, 2 slam, 3 arm cannon |
| 1 | Phase: 0 warning, 1 aim (the cannon frozen), 2 strike (it hurts) |
| 4 | His X, in 1/100 tile |
| 4 | The rockets' X, in 1/100 tile |
| 2 | Y: the top of the rockets' gap, or the cannon's line |
| 2 | Seconds into the attack, in 1/100 |
| 1 | 1 under his Sandevistan: low on health or past half his arena, his strikes and gaps sped up |
| 2 | The rows of the rocket wall Wraith blinked through, bit 0 the top row: they fly back at him (4 damage each) and hit nobody |

Adam Smasher steps in at 500 tiles, 12 tiles ahead of the leader, and fights for the next 400 (the
map is empty there). Rockets: a wall along every row but a 4-tile gap. Slam: the 3 bottom rows.
Arm cannon: like a sniper's railgun, thicker. In CYBERPSYCHO the players' shots, Lucy's hack and the
ultimates hurt him; `BOSS_KILL` ('K') in `MSG_COLLISION` when he goes down, `BOSS_HIT` ('m') when he
kills a player. Every number is in `src/common/boss.hpp` and `assets/tuning.txt`.

## A game, step by step

1. The client connects and sends `MSG_VERSION`, then `MSG_CONNECT`.
2. Once the lobby is full (extra clients are refused) and everyone sent `MSG_CONNECT`, the server
   sends the map (`MSG_MAP_DATA`) and `MSG_GAME_START` to each client, then `MSG_COUNTDOWN` once
   per second: 3, 2, 1, 0.
3. During the run the server ticks 60 times per second: players move forward, faster and faster,
   gravity or thrust applies, new map chunks are generated ahead of the leader, collisions are
   checked. It sends `MSG_GAME_STATE`, `MSG_TITANS` and `MSG_SNIPERS` every tick, `MSG_COLLISION` and `MSG_SHOT`
   when they happen, and `MSG_MAP_DATA` for each new chunk. Clients send `MSG_PLAYER_INPUT` and
   `MSG_ATTACK`.
4. At the first death the server sends `MSG_GAME_END`. Clients answer `MSG_READY` for a rematch,
   and the game starts again from step 2 with a new map. If someone leaves instead, the lobby
   reopens: whoever stays plays the next choom who joins.

## Security

The protocol is meant for friends on a local network or a VPN. There is no authentication or
encryption. The server never trusts clients with the game itself: they only send inputs, and it
drops any client sending more than 1 KB of unparsable data, or no longer reading what it sends.
What a slow connection can't take right away is queued, never cut, so packets always arrive whole.
