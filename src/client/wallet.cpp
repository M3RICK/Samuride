// Eddies earned over every run, spent on the runners (one after the other) and the districts

#include "wallet.hpp"
#include "../common/runners.hpp"
#include "../common/platform.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
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
            wallet.unlocked = std::clamp(value, 1, RUNNER_COUNT);
        else if (key == "districts")
            wallet.districts = value;
        // "upgrade.<n> <level>"
        for (int i = 0; i < UPGRADE_COUNT; i++) {
            if (key == "upgrade." + std::to_string(i))
                wallet.upgrades[i] = std::clamp(value, 0, UPGRADES[i].max_level);
        }
        // Up to v2.85, "upgrade_<n>" with the ultimate upgrade second: it's gone, its eddies come back
        static constexpr int LEGACY[] = {UPGRADE_SANDEVISTAN, -1, UPGRADE_MAGNET, UPGRADE_SHIELD};
        static constexpr int ULTIMATE_PRICES[] = {1000, 2500, 5000};
        for (int i = 0; i < static_cast<int>(std::size(LEGACY)); i++) {
            if (key != "upgrade_" + std::to_string(i))
                continue;
            if (LEGACY[i] >= 0)
                wallet.upgrades[LEGACY[i]] = std::clamp(value, 0, UPGRADES[LEGACY[i]].max_level);
            for (int level = 0; LEGACY[i] < 0 && level < std::clamp(value, 0, 3); level++)
                wallet.eddies += ULTIMATE_PRICES[level];
        }
    }
    return wallet;
}

void Wallet::save() const
{
    std::error_code ec;

    std::filesystem::create_directories(walletPath().parent_path(), ec);
    std::ofstream file(walletPath());

    file << "eddies " << eddies << "\nunlocked " << unlocked << "\ndistricts " << districts << "\n";
    for (int i = 0; i < UPGRADE_COUNT; i++)
        file << "upgrade." << i << " " << upgrades[i] << "\n";
}

void Wallet::earn(int amount)
{
    Wallet wallet = load();

    wallet.eddies += amount;
    wallet.save();
}

bool Wallet::buy(int character)
{
    if (character != unlocked || character >= RUNNER_COUNT || eddies < RUNNERS[character].price)
        return false;

    eddies -= RUNNERS[character].price;
    unlocked++;
    return true;
}

bool Wallet::hasDistrict(int district) const
{
    return district >= 0 && district < static_cast<int>(DISTRICT_PRICES.size()) &&
           (DISTRICT_PRICES[district] == 0 || (districts >> district & 1));
}

bool Wallet::buyDistrict(int district)
{
    if (district < 0 || district >= static_cast<int>(DISTRICT_PRICES.size()) || hasDistrict(district) ||
        eddies < DISTRICT_PRICES[district])
        return false;

    eddies -= DISTRICT_PRICES[district];
    districts |= 1 << district;
    return true;
}

bool Wallet::buyUpgrade(int upgrade)
{
    if (upgrade < 0 || upgrade >= UPGRADE_COUNT)
        return false;

    int &level = upgrades[upgrade];
    const UpgradeInfo &info = UPGRADES[upgrade];
    if (level >= info.max_level || eddies < info.prices[level])
        return false;

    eddies -= info.prices[level];
    level++;
    return true;
}
