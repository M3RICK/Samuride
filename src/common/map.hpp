// The run's tile map: loaded from a file, generated in chunks, streamed to clients

#ifndef MAP_HPP
    #define MAP_HPP

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>

// What a tile holds: one character each, in map files and on the network. The power-ups have their
// own (shift.hpp, powerups.hpp).
constexpr char EMPTY_TILE = '_';
constexpr char COIN_TILE = 'c';         // an eddy
constexpr char ZAPPER_TILE = 'e';       // deadly; neighbouring nodes are linked by a beam
constexpr char DRONE_TILE = 'd';        // CYBERPSYCHO: deadly too, shot down for a bounty
constexpr char GATE_TILE = 'g';         // a security gate's barrier: deadly while it's on

// Gates switch on and off on the run's clock, each column on its own beat: open GATE_OPEN_TIME
// out of every GATE_PERIOD seconds, buzzing GATE_WARNING seconds before switching back on
constexpr float GATE_PERIOD = 2.4f;
constexpr float GATE_OPEN_TIME = 1.2f;
constexpr float GATE_WARNING = 0.3f;

// Seconds into its cycle: open below GATE_OPEN_TIME
inline float gateCycle(float run_time, int x)
{
    return std::fmod(run_time + x * 0.37f, GATE_PERIOD);
}

inline bool gateOpen(float run_time, int x)
{
    return gateCycle(run_time, x) < GATE_OPEN_TIME;
}

// Grows to the right as the run goes on: the server appends chunks, clients receive them
class Map {
private:
    std::vector<std::string> mapData;
    size_t width;
    size_t height;

public:
    Map();

    bool loadFromFile(const std::string& filename);

    void clear();

    // rows.size() must match the map height (any height if the map is empty)
    bool appendChunk(const std::vector<std::string> &rows);

    // Columns from from_x to the end, as a MSG_MAP_DATA payload
    std::vector<uint8_t> serialize(size_t from_x = 0) const;

    // Offset 0 replaces the map, offset == width appends to it
    bool applyData(const std::vector<uint8_t>& data);

    char getTile(size_t x, size_t y) const;
    size_t getWidth() const { return width; }
    size_t getHeight() const { return height; }
};

// Every node of the zapper that has one at (x, y): the nodes linked to it, the way its beams are drawn
inline std::vector<std::pair<int, int>> zapperNodes(const Map &map, int x, int y)
{
    std::vector<std::pair<int, int>> nodes = {{x, y}};

    for (size_t i = 0; i < nodes.size(); i++) {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                std::pair<int, int> next = {nodes[i].first + dx, nodes[i].second + dy};
                if (next.first >= 0 && next.second >= 0 && map.getTile(next.first, next.second) == ZAPPER_TILE &&
                    std::find(nodes.begin(), nodes.end(), next) == nodes.end())
                    nodes.push_back(next);
            }
        }
    }
    return nodes;
}

#endif
