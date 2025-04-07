#ifndef MAP_HPP
    #define MAP_HPP

#include <string>
#include <vector>
#include <cstdint>

class Map {
private:
    std::vector<std::string> mapData;
    size_t width;
    size_t height;

public:
    Map();

    bool loadFromFile(const std::string& filename);

    bool loadFromData(const std::vector<uint8_t>& data);

    std::vector<uint8_t> serialize() const;

    char getTile(size_t x, size_t y) const;
    size_t getWidth() const { return width; }
    size_t getHeight() const { return height; }

    // Debug
    void printMap() const;
};

#endif
