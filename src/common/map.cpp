// The run's tile map: loaded from a file, generated in chunks, streamed to clients

#include "map.hpp"
#include "debug.hpp"
#include "protocol.hpp"
#include "cyberpsycho.hpp"
#include "shift.hpp"
#include <fstream>

Map::Map() : width(0), height(0)
{}

bool Map::loadFromFile(const std::string& filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
        return false;

    std::vector<std::string> rows;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty())
            rows.push_back(line);
    }

    if (rows.empty()) {
        DEBUG_LOG("Error: Map file is empty");
        return false;
    }

    for (const auto& row : rows) {
        if (row.length() != rows[0].length()) {
            DEBUG_LOG("Inconsistent line lengths");
            return false;
        }

        for (char c : row) {
            if (c != '_' && c != 'c' && c != 'e' && c != DRONE_TILE && c != SHIFT_TILE)
                return false;
        }
    }

    clear();
    return appendChunk(rows);
}

void Map::clear()
{
    mapData.clear();
    width = 0;
    height = 0;
}

bool Map::appendChunk(const std::vector<std::string> &rows)
{
    if (rows.empty() || (height != 0 && rows.size() != height))
        return false;

    if (height == 0) {
        height = rows.size();
        mapData.assign(height, "");
    }

    for (size_t y = 0; y < height; y++)
        mapData[y] += rows[y];
    width = mapData[0].length();
    return true;
}

std::vector<uint8_t> Map::serialize(size_t from_x) const
{
    std::vector<uint8_t> data;
    size_t chunk_width = width > from_x ? width - from_x : 0;

    Protocol::pushU32(data, from_x);
    Protocol::pushU32(data, chunk_width);
    Protocol::pushU32(data, height);

    for (const auto& row : mapData)
        data.insert(data.end(), row.begin() + from_x, row.begin() + from_x + chunk_width);
    return data;
}

bool Map::applyData(const std::vector<uint8_t>& data)
{
    if (data.size() < 12)
        return false;

    size_t offset = Protocol::readU32(data.data());
    size_t w = Protocol::readU32(data.data() + 4);
    size_t h = Protocol::readU32(data.data() + 8);

    if (h == 0 || data.size() != 12 + w * h || (offset != 0 && (offset != width || h != height)))
        return false;

    std::vector<std::string> rows;
    for (size_t y = 0; y < h; y++)
        rows.emplace_back(data.begin() + 12 + y * w, data.begin() + 12 + (y + 1) * w);

    if (offset == 0)
        clear();
    return appendChunk(rows);
}

char Map::getTile(size_t x, size_t y) const
{
    if (y >= height || x >= width)
        return '_';

    return mapData[y][x];
}
