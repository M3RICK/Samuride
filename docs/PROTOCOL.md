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

### `MSG_CONNECT`

| Bytes | Field |
| --- | --- |
| 1, optional | Character: 0 Johnny, 1 David, 2 Jinx, 3 Mikasa |
| 1, optional | District: 0 The Corridor, 1 Worlds Collide, 2 Night City, 3 The Walls, 4 Zaun, 5 Both Worlds |
| 1, optional | Mode: 0 CLASSIC, 1 CYBERPSYCHO |

District and mode are only taken from the first player in the lobby, the host; whoever joins
sends only a character. When the mode differs from the map already sent, the server regenerates
the map (with drones for CYBERPSYCHO) and sends it again.

### `MSG_MAP_DATA`

| Bytes | Field |
| --- | --- |
| 4 | Offset: first column of this chunk |
| 4 | Chunk width |
| 4 | Map height |
| width × height | Tiles, one byte each, row by row from the top |

An offset of 0 replaces the client's map; any other offset must equal the client's current width,
and the chunk is appended. The whole map is sent on connection and before each game, then new
chunks are streamed while the players advance.

Tiles:

| Byte | Tile |
| --- | --- |
| `_` | Empty |
| `c` | Coin (eddie), +1 |
| `e` | Zapper, deadly |
| `d` | Drone (CYBERPSYCHO), deadly, +5 when destroyed |
| `p` | Attack Titan orb |

Collected coins and orbs stay in the map: the server tracks who took what, so both players can
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

A 10-byte record per player, 60 times per second:

| Bytes | Field |
| --- | --- |
| 1 | Player number |
| 4 | X, also the distance run |
| 2 | Y: top of the 2-tile-tall body, 0 is the top row |
| 2 | Score |
| 1 | Flags: bit 0 jetpack, bits 1-2 form (0 normal, 1 shifting, 2 Attack Titan, 3 emerging) |

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
| `k` | Titan taken down (Mikasa's blade or the Attack Titan), +20 |
| `p` | Attack Titan orb taken: shifting starts |

### `MSG_GAME_END`

| Bytes | Field |
| --- | --- |
| 1 | Winner's player number, or `0xFF` for none |

The game ends at the first death (zapper, drone or titan): the other player wins, nobody if both
died on the same tick. If a player disconnects, the remaining one wins. In solo the winner is
always `0xFF` and the score is the distance run.

### `MSG_READY`

Empty. Sent after `MSG_GAME_END` to ask for a rematch. Once every connected player is ready, the
server sends a fresh `MSG_MAP_DATA` (offset 0) and `MSG_GAME_START` to everyone.

### `MSG_ATTACK`

Empty, CYBERPSYCHO only (ignored otherwise). The server enforces the weapon cooldown. Johnny and
David shoot guns, Jinx rockets: a projectile flies ahead at run speed plus the weapon's speed, for
up to 22 tiles. Mikasa's blade destroys every drone and titan within 2.5 tiles ahead.

### `MSG_SHOT`

| Bytes | Field |
| --- | --- |
| 1 | Player number |
| 1 | Weapon: 0 gun, 1 rockets, 2 blade |

### `MSG_TITANS`

| Bytes | Field |
| --- | --- |
| 1 | Count |
| 5 per titan | Type (0 Female, 4.5 tiles tall and fast; 1 Armored, 4 tiles tall and slower), X of its left edge |

Titans stand on the floor and run at the players. The first one comes past 150 tiles, then one
every 7 to 14 seconds, appearing 40 tiles ahead of the leader.

## A game, step by step

1. The client connects and sends `MSG_CONNECT`; the server answers with `MSG_MAP_DATA`.
2. Once the lobby is full (extra clients are refused), the server sends `MSG_GAME_START` to each
   client, then `MSG_COUNTDOWN` once per second: 3, 2, 1, 0.
3. During the run the server ticks 60 times per second: players move forward, faster and faster,
   gravity or thrust applies, new map chunks are generated ahead of the leader, collisions are
   checked. It sends `MSG_GAME_STATE` and `MSG_TITANS` every tick, `MSG_COLLISION` and `MSG_SHOT`
   when they happen, and `MSG_MAP_DATA` for each new chunk. Clients send `MSG_PLAYER_INPUT` and
   `MSG_ATTACK`.
4. At the first death the server sends `MSG_GAME_END`. Clients answer `MSG_READY` for a rematch,
   and the game starts again from step 2 with a new map.

## Security

The protocol is meant for friends on a local network or a VPN. There is no authentication or
encryption. The server never trusts clients with the game itself: they only send inputs, and it
drops any client sending more than 1 KB of unparsable data.
