Samuride - Multiplayer Jetpack Game
<p align="center"> <img src="https://github.com/user-attachments/assets/cbadfc9f-8b40-41c6-b83a-08a557ad397a" alt="Samuride Game" width="600px"> </p> <p align="center"> <em>A multiplayer Jetpack Joyride-inspired game with network play.</em> </p>
## Multiplayer gameplay — Race against your friends in real-time

Client-server architecture with efficient binary TCP protocol
Coin collection and competitive scoring
Hazard avoidance — Don't get electrocuted!
Game audio and visual effects
Real-time physics with jetpack mechanics

# 🚧 Development Status
Note: This project is currently in development and not fully completed.

Working on:

Animations refinement
Visual polish
Additional sound effects
UI improvements

# Building the Game
## Prerequisites
C++20 compiler (g++ recommended)
SFML library (graphics, window, audio)
pthread library

# Compilation

## Build both server and client
make

### Build only the server
make server

## Build only the client
make client

## Clean object files
make clean

## Clean everything (objects and executables)
make fclean

## Rebuild everything
make re

# 🎮 Running the Game

## Server
./jetpack_server -p <port> -m <map_file> [-d]

Options:

-p <port> — Port to listen on
-m <map_file> — Path to map file
-d — Enable debug mode (optional)

Example:

./jetpack_server -p 4242 -m maps/small_good.txt -d

## Client
./jetpack_client -h <ip> -p <port> [-d]

Options:

-h <ip> — Server IP address

-p <port> — Server port

-d — Enable debug mode (optional)

Example:
./jetpack_client -h 127.0.0.1 -p 4242 -d

# Game Controls
Press Space — Activate jetpack

Press Escape — Exit game

# Protocol Information
Samuride uses a custom binary protocol for client-server communication:

Standard message format: 4-byte header (1 byte message type + 3 bytes payload size)

## Message types:

Connection establishment

Map data transmission

Game state updates

Player input

Collision notifications

Game end conditions
For complete protocol details, see the doc.txt file.

# Future Improvements
Complete and polish all character animations

Add more varied map elements and obstacles

Implement power-ups and special abilities

Add more sound effects and music options

Improve the UI and add more in-game feedback

Implement a proper lobby system for waiting players

Add player customization options

📝 License
This project is licensed under the MIT License.

<p align="center"> <sub>Wake the fuck up, Samurai. We've got a city to burn.</sub> </p>
