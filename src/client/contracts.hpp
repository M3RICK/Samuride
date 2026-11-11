// Contracts: three small goals at a time, settled at the end of every solo run. A finished one pays its
// eddies and another takes its place.

#ifndef CONTRACTS_HPP
    #define CONTRACTS_HPP

#include <array>
#include <random>
#include <string>
#include <vector>

struct Contract {
    // Kept in the save by number: only add at the end
    enum Kind { Distance, TotalDistance, Drones, TotalDrones, Eddies, RunnerDistance, KIND_COUNT };

    int kind = Distance;
    int goal = 0;           // 0: an empty slot
    int reward = 0;         // eddies
    int progress = 0;       // the best run so far, or the total over the runs for the Total kinds
    int runner = -1;        // RunnerDistance only: who has to run it

    bool done() const { return goal > 0 && progress >= goal; }
    // "Down 25 drones in one run"; `runner_name` is only read by RunnerDistance
    std::string label(const std::string &runner_name) const;
};

// What a run counts for
struct RunStats {
    int distance = 0;   // metres
    int eddies = 0;
    int drones = 0;     // 0 in CLASSIC
    int runner = 0;
};

struct Contracts {
    static constexpr int COUNT = 3;
    // Goals from the easiest to the hardest, and what each pays
    static constexpr int TIERS = 3;
    static constexpr int REWARDS[TIERS] = {60, 100, 150};
    static constexpr int GOALS[Contract::KIND_COUNT][TIERS] = {
        {400, 600, 900},        // Distance
        {1500, 2500, 4000},     // TotalDistance
        {15, 25, 40},           // Drones
        {50, 90, 150},          // TotalDrones
        {80, 130, 200},         // Eddies
        {300, 500, 700},        // RunnerDistance
    };

    std::array<Contract, COUNT> slots;

    // Kept in the platform's data directory, next to the wallet; empty slots are filled on the way
    static Contracts load();
    void save() const;
    // A new contract in every empty slot, of a kind none of the others has, for one of the first
    // `unlocked` runners
    void fill(std::mt19937 &rng, int unlocked);
    // Counts a run, then returns the contracts it finished, replaced by new ones
    std::vector<Contract> settle(const RunStats &run, std::mt19937 &rng, int unlocked);
};

#endif
