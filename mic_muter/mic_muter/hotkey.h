#pragma once

namespace chs::mic_muter
{
    struct hotkey_t
    {
        byte key_code;
        char const *name;
    };

    extern hotkey_t hotkeys[];
    extern size_t const num_hotkeys;

    int get_hotkey_index(byte keycode);
    std::string get_hotkey_name(byte keycode, byte modifiers);
}