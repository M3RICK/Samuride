// Eddies earned over every run, spent on unlocking the runners one after the other (their prices are in
// common/runners.hpp), and the districts

#ifndef WALLET_HPP
    #define WALLET_HPP

#include <vector>

// Price of each district, in MenuLayout order: The Corridor, Worlds Collide, Night City, The Walls, Zaun,
// Both Worlds. 0 is free; the others are bought in any order.
inline const std::vector<int> DISTRICT_PRICES = {0, 500, 0, 150, 300, 750};

struct Wallet {
    int eddies = 0;
    int unlocked = 1;   // the first `unlocked` characters are playable
    int districts = 0;  // bit i: district i was bought

    // Kept in the platform's data directory, next to the settings
    static Wallet load();
    void save() const;
    // Adds a run's eddies to the saved wallet
    static void earn(int amount);

    bool isUnlocked(int character) const { return character < unlocked; }
    // Only the next locked character can be bought; returns whether it was
    bool buy(int character);

    bool hasDistrict(int district) const;
    // Returns whether it was bought
    bool buyDistrict(int district);
};

#endif
