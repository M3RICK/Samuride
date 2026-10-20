// Random map chunks: zapper beams, coin blocks and waves, drone squads

#include "generator.hpp"
#include "../common/cyberpsycho.hpp"
#include <algorithm>
#include <cmath>

using Rows = std::vector<std::string>;

static int randInt(std::mt19937 &rng, int min, int max)
{
    return std::uniform_int_distribution<int>(min, max)(rng);
}

// Patterns running past the chunk edge are just cut
static void setTile(Rows &rows, int x, int y, char tile)
{
    if (y >= 0 && y < static_cast<int>(rows.size()) && x >= 0 && x < static_cast<int>(rows[y].size()))
        rows[y][x] = tile;
}

// Returns the number of columns used
static int placeZapper(Rows &rows, std::mt19937 &rng, int x, float difficulty)
{
    int height = rows.size();
    // Always leave 4 free rows in a column: the player is 2 tiles tall
    int max_len = std::clamp(2 + static_cast<int>(4 * difficulty), 2, height - 4);
    int len = randInt(rng, 2, max_len);

    switch (randInt(rng, 0, 2)) {
        case 0: {
            int y = randInt(rng, 0, height - len);
            for (int i = 0; i < len; i++)
                setTile(rows, x, y + i, ZAPPER_TILE);
            return 1;
        }
        case 1: {
            int y = randInt(rng, 0, height - 1);
            for (int i = 0; i < len; i++)
                setTile(rows, x + i, y, ZAPPER_TILE);
            return len;
        }
        default: {
            int y = randInt(rng, 0, height - len);
            bool rising = randInt(rng, 0, 1);
            for (int i = 0; i < len; i++)
                setTile(rows, x + i, rising ? y + len - 1 - i : y + i, ZAPPER_TILE);
            return len;
        }
    }
}

// A small squad of drones stacked in one column, with room left to fly past
static int placeDrones(Rows &rows, std::mt19937 &rng, int x, float difficulty)
{
    int height = rows.size();
    int count = randInt(rng, 1, 2 + static_cast<int>(difficulty));
    int y = randInt(rng, 0, height - 2 * count);

    for (int i = 0; i < count; i++)
        setTile(rows, x, y + 2 * i, DRONE_TILE);
    return 1;
}

static int placeCoins(Rows &rows, std::mt19937 &rng, int x)
{
    int height = rows.size();

    if (randInt(rng, 0, 1)) {
        int w = randInt(rng, 3, 6);
        int h = randInt(rng, 2, 3);
        int y = randInt(rng, 0, height - h);
        for (int i = 0; i < w; i++)
            for (int j = 0; j < h; j++)
                setTile(rows, x + i, y + j, COIN_TILE);
        return w;
    }

    int w = randInt(rng, 8, 12);
    int amplitude = randInt(rng, 1, 2);
    int center = randInt(rng, amplitude, height - 1 - amplitude);
    for (int i = 0; i < w; i++)
        setTile(rows, x + i, center + static_cast<int>(std::round(std::sin(i * 0.7) * amplitude)), COIN_TILE);
    return w;
}

// Gaps after obstacles don't shrink with difficulty: at top speed 6 columns is about the
// time needed to switch lanes. Difficulty makes obstacles longer and more frequent instead.
// ponytail: fairness is statistical, add a reachability check if runs feel unfair
std::vector<std::string> generateChunk(std::mt19937 &rng, size_t width, size_t height, float difficulty,
                                       bool drones)
{
    Rows rows(height, std::string(width, EMPTY_TILE));
    std::uniform_real_distribution<float> chance(0, 1);
    int x = randInt(rng, 2, 5);

    while (x < static_cast<int>(width)) {
        float roll = chance(rng);

        if (drones && roll < 0.3f)
            x += placeDrones(rows, rng, x, difficulty) + randInt(rng, 5, 9);
        else if (roll < 0.45f + 0.3f * difficulty)
            x += placeZapper(rows, rng, x, difficulty) + randInt(rng, 6, 9);
        else
            x += placeCoins(rows, rng, x) + randInt(rng, 3, 6);
    }
    return rows;
}
