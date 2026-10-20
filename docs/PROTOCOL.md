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

### `MSG_CONNECT`

| Bytes | Field |
| --- | --- |
| 1, optional | Character: 0 Johnny, 1 David, 2 Jinx, 3 Mikasa |
| 1, optional | District: 0 The Corridor, 1 Worlds Collide, 2 Night City, 3 The Walls, 4 Zaun, 5 Both Worlds |
| 1, optional | Mode: 0 CLASSIC, 1 CYBERPSYCHO |

District and mode are only taken from the first player in the lobby, the host; whoever joins
sends only a character. A game only starts once every player in the lobby sent theirs, and
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

An 11-byte record per player, 60 times per second:

| Bytes | Field |
| --- | --- |
| 1 | Player number |
| 4 | X, also the distance run |
| 2 | Y: top of the 2-tile-tall body, 0 is the top row |
| 2 | Score |
| 1 | Flags: bit 0 jetpack, bits 1-2 form (0 normal, 1 shifting, 2 Attack Titan, 3 emerging), bit 3 shield, bit 4 Sandevistan, bits 5-6 Lucy's RAM charges, bit 7 Berserk |
| 1 | Ultimate gauge, in percent |

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
| `k` | Titan taken down (Mikasa's blade, the Attack Titan or Johnny's fire wave), +20 |
| `p` | Attack Titan orb taken: shifting starts |
| `a` | Shield taken |
| `v` | Sandevistan taken: the sprint starts |
| `r` | Berserk taken: 8 s of attacking on your own, twice as often |
| `h` | Lucy hacked the zapper with a node on that tile: all its nodes are off for 1.2 s |
| `b` | A shield took a deadly hit (zapper, drone or titan) instead of the player, who gets 1 s of invincibility |

### `MSG_GAME_END`

| Bytes | Field |
| --- | --- |
| 1 | Winner's player number, or `0xFF` for none |

The game ends at the first death (zapper, drone or titan, unless a shield takes it): the other player wins, nobody if both
died on the same tick. If a player disconnects, the remaining one wins. In solo the winner is
always `0xFF` and the score is the distance run.

### `MSG_READY`

Empty. Sent after `MSG_GAME_END` to ask for a rematch. Once every connected player is ready, the
server sends a fresh `MSG_MAP_DATA` (offset 0) and `MSG_GAME_START` to everyone.

### `MSG_ATTACK`

Empty, CYBERPSYCHO only (ignored otherwise). The server enforces the weapon cooldown. Johnny and
David shoot guns, Jinx rockets: a projectile flies ahead at run speed plus the weapon's speed, for
up to 22 tiles. Mikasa's blade destroys every drone and titan within 2.5 tiles ahead. Lucy hacks
the first drone or zapper in her way (7 tiles ahead, from a tile above her head to a tile below her
feet), for one of her 2 RAM charges; nothing in her way, nothing spent.

### `MSG_SHOT`

| Bytes | Field |
| --- | --- |
| 1 | Player number |
| 1 | Weapon: 0 gun, 1 rockets, 2 blade, 3 hack |

### `MSG_ULTIMATE`

From the client: empty, CYBERPSYCHO only, ignored unless the gauge is full (100 eddies earned since
the last one) and the runner has an ultimate. From the server, to everyone:

| Bytes | Field |
| --- | --- |
| 1 | Player number |
| 1 | Ultimate: 1 fire wave (Johnny) |
| 4 | X where it starts |

The fire wave sweeps 25 tiles ahead at 45 tiles per second: every column it crosses burns from
floor to ceiling. Drones go down (`d`), zappers go out for the rest of the run, titans die (`k`).
Clients burn the zappers themselves from the start and the speed.

### `MSG_TITANS`

| Bytes | Field |
| --- | --- |
| 1 | Count |
| 5 per titan | Type (0 Female, 4.5 tiles tall and fast; 1 Armored, 4 tiles tall and slower), X of its left edge |

Titans stand on the floor and run at the players. The first one comes past 300 tiles, then one
every 7 to 14 seconds, appearing 40 tiles ahead of the leader.

## A game, step by step

1. The client connects and sends `MSG_CONNECT`.
2. Once the lobby is full (extra clients are refused) and everyone sent `MSG_CONNECT`, the server
   sends the map (`MSG_MAP_DATA`) and `MSG_GAME_START` to each client, then `MSG_COUNTDOWN` once
   per second: 3, 2, 1, 0.
3. During the run the server ticks 60 times per second: players move forward, faster and faster,
   gravity or thrust applies, new map chunks are generated ahead of the leader, collisions are
   checked. It sends `MSG_GAME_STATE` and `MSG_TITANS` every tick, `MSG_COLLISION` and `MSG_SHOT`
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
