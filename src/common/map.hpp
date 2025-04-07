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

    // Load map from file
    bool loadFromFile(const std::string& filename);

    // Create map from serialized data
    bool loadFromData(const std::vector<uint8_t>& data);

    // Serialize map for network transmission
    std::vector<uint8_t> serialize() const;

    // Map accessors
    char getTile(size_t x, size_t y) const;
    size_t getWidth() const { return width; }
    size_t getHeight() const { return height; }

    // Debug
    void printMap() const;
};

#endif
