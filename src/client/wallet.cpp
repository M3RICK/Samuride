// Eddies earned over every run, spent on unlocking the characters one after the other

#include "wallet.hpp"
#include "../common/platform.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

static std::filesystem::path walletPath()
{
    return Platform::dataDirectory() / "wallet";
}

// "key value" lines, like the settings
Wallet Wallet::load()
{
    Wallet wallet;
    std::ifstream file(walletPath());
    std::string key;
    int value;

    while (file >> key >> value) {
        if (key == "eddies")
            wallet.eddies = std::max(0, value);
        else if (key == "unlocked")
            wallet.unlocked = std::clamp(value, 1, static_cast<int>(CHARACTER_PRICES.size()));
    }
    return wallet;
}

void Wallet::save() const
{
    std::error_code ec;

    std::filesystem::create_directories(walletPath().parent_path(), ec);
    std::ofstream(walletPath()) << "eddies " << eddies << "\nunlocked " << unlocked << "\n";
}

void Wallet::earn(int amount)
{
    Wallet wallet = load();

    wallet.eddies += amount;
    wallet.save();
}

bool Wallet::buy(int character)
{
    if (character != unlocked || character >= static_cast<int>(CHARACTER_PRICES.size()) ||
        eddies < CHARACTER_PRICES[character])
        return false;

    eddies -= CHARACTER_PRICES[character];
    unlocked++;
    return true;
}
