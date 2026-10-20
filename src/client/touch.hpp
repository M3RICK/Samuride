// Touch controls, for phones: any finger on the screen is the jetpack, but for two round buttons at the
// bottom right, attack and ultimate. Shared by the inputs (what was touched) and the HUD (drawing them).

#ifndef TOUCH_HPP
    #define TOUCH_HPP

#include <SFML/System/Vector2.hpp>

namespace Touch {
#ifdef __ANDROID__
    constexpr bool ENABLED = true;
#else
    constexpr bool ENABLED = false;
#endif

    // In screen coordinates (1920x1080), within the right thumb's reach
    constexpr float BUTTON_RADIUS = 95;
    constexpr sf::Vector2f ATTACK{1920 - 160, 1080 - 170};
    constexpr sf::Vector2f ULTIMATE{1920 - 160, 1080 - 400};

    // A little more than the drawn circle: fingers are fat
    inline bool on(sf::Vector2f button, sf::Vector2f pos)
    {
        return (pos - button).lengthSquared() < BUTTON_RADIUS * BUTTON_RADIUS * 1.4f;
    }
}

#endif
