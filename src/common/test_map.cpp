#include "map.hpp"
#include "debug.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    //rajouter arg chercker if -d alors setDebugMode(true)
    g_logger.setDebugMode(true);

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <map_file>" << std::endl;
        return 1;
    }

    Map map;

    if (!map.loadFromFile(argv[1])) {
        std::cout << "Failed to load map!" << std::endl;
        return 1;
    }

    std::cout << "Map loaded successfully:" << std::endl;
    map.printMap();

    // Test serialization/deserialization
    auto serialized = map.serialize();

    Map map2;
    if (!map2.loadFromData(serialized)) {
        std::cout << "Failed to deserialize map!" << std::endl;
        return 1;
    }

    std::cout << "\nDeserialized map:" << std::endl;
    map2.printMap();

    return 0;
}
