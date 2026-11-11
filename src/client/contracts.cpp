// Contracts: three small goals at a time, settled at the end of every solo run

#include "contracts.hpp"
#include "wallet.hpp"
#include "../common/platform.hpp"
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>

static std::filesystem::path contractsPath()
{
    return Platform::dataDirectory() / "contracts";
}

std::string Contract::label(const std::string &runner_name) const
{
    switch (kind) {
        case Distance:       return std::format("Run {} m in one run", goal);
        case TotalDistance:  return std::format("Run {} m in total", goal);
        case Drones:         return std::format("Down {} drones in one run", goal);
        case TotalDrones:    return std::format("Down {} drones in total", goal);
        case Eddies:         return std::format("Grab {} eddies in one run", goal);
        default:             return std::format("Run {} m in one run as {}", goal, runner_name);
    }
}

// One "kind goal reward progress runner" line per contract; a line that makes no sense is an empty slot
Contracts Contracts::load()
{
    Contracts contracts;
    std::ifstream file(contractsPath());
    std::mt19937 rng(std::random_device{}());

    for (Contract &slot : contracts.slots) {
        Contract read;
        if (file >> read.kind >> read.goal >> read.reward >> read.progress >> read.runner &&
            read.kind >= 0 && read.kind < Contract::KIND_COUNT && read.goal > 0 && read.progress >= 0 &&
            read.reward >= 0 && read.reward <= REWARDS[TIERS - 1])
            slot = read;
    }
    contracts.fill(rng, Wallet::load().unlocked);
    return contracts;
}

void Contracts::save() const
{
    std::error_code ec;

    std::filesystem::create_directories(contractsPath().parent_path(), ec);
    std::ofstream file(contractsPath());
    for (const Contract &slot : slots)
        file << slot.kind << " " << slot.goal << " " << slot.reward << " " << slot.progress << " " << slot.runner << "\n";
}

void Contracts::fill(std::mt19937 &rng, int unlocked)
{
    for (Contract &slot : slots) {
        if (slot.goal > 0)
            continue;
        std::vector<int> free;
        for (int kind = 0; kind < Contract::KIND_COUNT; kind++) {
            if (std::ranges::none_of(slots, [&](const Contract &other) { return other.goal > 0 && other.kind == kind; }))
                free.push_back(kind);
        }
        int kind = free[rng() % free.size()];
        int tier = rng() % TIERS;
        int runner = kind == Contract::RunnerDistance ? static_cast<int>(rng() % std::max(1, unlocked)) : -1;
        slot = {kind, GOALS[kind][tier], REWARDS[tier], 0, runner};
    }
}

std::vector<Contract> Contracts::settle(const RunStats &run, std::mt19937 &rng, int unlocked)
{
    std::vector<Contract> finished;

    for (Contract &slot : slots) {
        switch (slot.kind) {
            case Contract::Distance:        slot.progress = std::max(slot.progress, run.distance); break;
            case Contract::TotalDistance:   slot.progress += run.distance; break;
            case Contract::Drones:          slot.progress = std::max(slot.progress, run.drones); break;
            case Contract::TotalDrones:     slot.progress += run.drones; break;
            case Contract::Eddies:          slot.progress = std::max(slot.progress, run.eddies); break;
            default:
                if (run.runner == slot.runner)
                    slot.progress = std::max(slot.progress, run.distance);
        }
        if (slot.done()) {
            finished.push_back(slot);
            slot = {};
        }
    }
    fill(rng, unlocked);
    return finished;
}
