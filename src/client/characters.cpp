// Loads a character from the meta.txt written by tools/slice_sheets.py

#include "characters.hpp"
#include "../common/debug.hpp"
#include <algorithm>
#include <fstream>
#include <map>

int Animation::frameAt(float seconds) const
{
    int index = static_cast<int>(seconds * fps);

    if (frames == 0)
        return -1;
    if (mode == Mode::Loop)
        return index % frames;
    if (mode == Mode::Hide && index >= frames)
        return -1;
    return std::min(index, frames - 1);
}

// meta.txt: one "key values..." per line, written by tools/slice_sheets.py
bool Character::load(const std::string &dir)
{
    std::ifstream meta(dir + "/meta.txt");
    std::map<std::string, Animation *> animations = {
        {"run", &run}, {"takeoff", &takeoff}, {"rise", &rise}, {"hover", &hover}, {"fall", &fall},
        {"death", &death}, {"attack", &attack}, {"smoke", &smoke}, {"emerge", &emerge}
    };
    std::string key;

    while (meta >> key) {
        if (key == "name") {
            meta >> name;
        } else if (key == "sheet" || key == "portrait") {
            std::string file;
            meta >> file;
            sf::Texture &texture = key == "sheet" ? sheet : portrait;
            if (!texture.loadFromFile(dir + "/" + file))
                DEBUG_LOG("Failed to load " + dir + "/" + file);
        } else if (key == "frame") {
            meta >> frame_size.x >> frame_size.y;
        } else if (key == "feet") {
            meta >> feet;
        } else if (key == "body") {
            meta >> body;
        } else if (key == "exhaust") {
            int r, g, b;
            meta >> r >> g >> b;
            exhaust = sf::Color(r, g, b);
        } else if (animations.count(key)) {
            Animation &anim = *animations[key];
            std::string mode;
            meta >> anim.row >> anim.frames >> anim.fps >> mode;
            anim.mode = mode == "once" ? Animation::Mode::Once : mode == "hide" ? Animation::Mode::Hide : Animation::Mode::Loop;
        } else {
            std::getline(meta, key);
        }
    }
    return sheet.getSize().x > 0 && run.frames > 0;
}

sf::IntRect Character::frameRect(const Animation &anim, int index) const
{
    return sf::IntRect({index * frame_size.x, anim.row * frame_size.y}, frame_size);
}
