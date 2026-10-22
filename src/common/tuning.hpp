// Balance values anyone can change without compiling: assets/tuning.txt, read when the game or the
// server starts, sets them over the defaults written in the code. Every value is listed in tuning.cpp.

#ifndef TUNING_HPP
    #define TUNING_HPP

#include <istream>
#include <string>

namespace Tuning {
    // "NAME value" lines, # starts a comment. Unknown names and values out of range are reported on
    // stderr and skipped. Returns how many values were read; a missing file reads none.
    int load(const std::string &path);
    int apply(std::istream &in, const std::string &source);
    // The whole file, every value at its default and explained (./Samuride_server -t)
    std::string defaults();
    // Some value isn't at its default: the HUD says so, so feedback says what was played
    bool custom();
}

#endif
