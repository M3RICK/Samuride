/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "map.hpp"
#include "debug.hpp"
#include <fstream>
#include <iostream>

Map::Map() : width(0), height(0)
{}

bool Map::loadFromFile(const std::string& filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
        return false;

    mapData.clear();
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty()) {
            mapData.push_back(line);
        }
    }

    if (mapData.empty()) {
        DEBUG_LOG("Error: Map file is empty");
        return false;
    }

    height = mapData.size();
    width = mapData[0].length();

    for (const auto& line : mapData) {
        if (line.length() != width) {
            DEBUG_LOG("Inconsistent line lengths");
            return false;
        }

        for (char c : line) {
            if (c != '_' && c != 'c' && c != 'e')
                return false;
        }
    }
    return true;
}

std::vector<uint8_t> Map::serialize() const
{
    std::vector<uint8_t> data;

    data.push_back((width >> 24) & 0xFF);
    data.push_back((width >> 16) & 0xFF);
    data.push_back((width >> 8) & 0xFF);
    data.push_back(width & 0xFF);

    data.push_back((height >> 24) & 0xFF);
    data.push_back((height >> 16) & 0xFF);
    data.push_back((height >> 8) & 0xFF);
    data.push_back(height & 0xFF);

    for (const auto& line : mapData) {
        for (char c : line) {
            data.push_back(static_cast<uint8_t>(c));
        }
    }

    DEBUG_LOG("Serialized map: " + std::to_string(data.size()) + " bytes");
    return data;
}

bool Map::loadFromData(const std::vector<uint8_t>& data)
{
    if (data.size() < 8) {
        DEBUG_LOG("Error: Insufficient data to deserialize map");
        return false;
    }

    size_t w = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    size_t h = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];

    if (data.size() != 8 + (w * h)) {
        DEBUG_LOG("Error: Map data size mismatch");
        return false;
    }

    mapData.clear();
    width = w;
    height = h;

    for (size_t y = 0; y < h; y++) {
        std::string line;
        for (size_t x = 0; x < w; x++) {
            size_t idx = 8 + y * w + x;
            line.push_back(static_cast<char>(data[idx]));
        }
        mapData.push_back(line);
    }

    DEBUG_LOG("Deserialized map: " + std::to_string(width) + "x" + std::to_string(height));
    return true;
}

char Map::getTile(size_t x, size_t y) const
{
    if (y >= mapData.size() || x >= mapData[y].length())
        return '_';

    return mapData[y][x];
}

void Map::printMap() const
{
    for (const auto& line : mapData) {
        std::cout << line << std::endl;
    }
}
