// Eddies earned over every run, spent on unlocking the characters one after the other

#ifndef WALLET_HPP
    #define WALLET_HPP

#include <vector>

// Price of each CHARACTER_DIRS entry: Johnny is free, the others are bought in order
inline const std::vector<int> CHARACTER_PRICES = {0, 100, 250, 500};

struct Wallet {
    int eddies = 0;
    int unlocked = 1;   // the first `unlocked` characters are playable

    // Kept in the platform's data directory, next to the settings
    static Wallet load();
    void save() const;
    // Adds a run's eddies to the saved wallet
    static void earn(int amount);

    bool isUnlocked(int character) const { return character < unlocked; }
    // Only the next locked character can be bought; returns whether it was
    bool buy(int character);
};

#endif
