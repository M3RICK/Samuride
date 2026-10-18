// The run's tile map: loaded from a file, generated in chunks, streamed to clients

#ifndef MAP_HPP
    #define MAP_HPP

#include <string>
#include <vector>
#include <cstdint>

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

#endif
