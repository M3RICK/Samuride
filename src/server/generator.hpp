/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef GENERATOR_HPP
    #define GENERATOR_HPP

#include <random>
#include <string>
#include <vector>

// Random Jetpack Joyride patterns: zapper beams, coin blocks and coin waves.
// difficulty goes from 0 to 1: longer and more frequent zappers, shorter breathers.
std::vector<std::string> generateChunk(std::mt19937 &rng, size_t width, size_t height, float difficulty);

#endif
