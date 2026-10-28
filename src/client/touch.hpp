// Touch controls, for phones: any finger on the screen is the jetpack, but for two round buttons at the
// bottom right, attack and ultimate. Shared by the inputs (what was touched) and the HUD (drawing them).

#ifndef TOUCH_HPP
    #define TOUCH_HPP

#include <SFML/Window/Event.hpp>
#include <cmath>

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

    // Once a run is over (or the connection lost): the way back to the menu, at the bottom.
    // The phone's back gesture doesn't always reach the game.
    constexpr sf::Vector2f MENU_BUTTON{1920 / 2, 1080 - 110};
    constexpr sf::Vector2f MENU_BUTTON_SIZE{380, 100};

    // The Trauma Team's question, in the middle: call it, or let me die
    constexpr sf::Vector2f TRAUMA_CALL{1920 / 2 - 240, 1080 / 2 + 170};
    constexpr sf::Vector2f TRAUMA_DECLINE{1920 / 2 + 240, 1080 / 2 + 170};
    constexpr sf::Vector2f TRAUMA_BUTTON_SIZE{420, 110};

    inline bool onBox(sf::Vector2f center, sf::Vector2f size, sf::Vector2f pos)
    {
        sf::Vector2f offset = pos - center;
        return std::abs(offset.x) < size.x / 2 && std::abs(offset.y) < size.y / 2;
    }

    inline bool onMenuButton(sf::Vector2f pos)
    {
        return onBox(MENU_BUTTON, MENU_BUTTON_SIZE, pos);
    }

    // A help line: the keys on desktop, the gestures on phones (where the back button is Escape)
    inline const char *hint(const char *keyboard, const char *touch)
    {
        return ENABLED ? touch : keyboard;
    }

    // SFML reports fingers only as touch events: for the menus, which work with the mouse, a finger
    // going down is a click
    inline sf::Event asClick(const sf::Event &event)
    {
        if (const auto *touch = event.getIf<sf::Event::TouchBegan>())
            return sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, touch->position};
        return event;
    }

    // A little more than the drawn circle: fingers are fat
    inline bool on(sf::Vector2f button, sf::Vector2f pos)
    {
        return (pos - button).lengthSquared() < BUTTON_RADIUS * BUTTON_RADIUS * 1.4f;
    }
}

#endif
