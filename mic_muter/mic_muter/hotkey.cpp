
#include "framework.h"

namespace chs::mic_muter
{
    chs::mic_muter::hotkey_t hotkeys[] = { { 0x08, "Backspace" },
                                           { 0x09, "Tab" },
                                           { 0x0C, "Clear" },
                                           { 0x0D, "Enter" },
                                           { 0x10, "Shift" },
                                           { 0x11, "Ctrl" },
                                           { 0x12, "Alt" },
                                           { 0x13, "Pause" },
                                           { 0x14, "Caps Lock" },
                                           { 0x1B, "Esc" },
                                           { 0x20, "Spacebar" },
                                           { 0x21, "Page Up" },
                                           { 0x22, "Page Down" },
                                           { 0x23, "End" },
                                           { 0x24, "Home" },
                                           { 0x25, "Left Arrow" },
                                           { 0x26, "Up Arrow" },
                                           { 0x27, "Right Arrow" },
                                           { 0x28, "Down Arrow" },
                                           { 0x29, "Select" },
                                           { 0x2A, "Print" },
                                           { 0x2B, "Execute" },
                                           { 0x2C, "Print Screen" },
                                           { 0x2D, "Insert" },
                                           { 0x2E, "Delete" },
                                           { 0x2F, "Help" },
                                           { 0x30, "0" },
                                           { 0x31, "1" },
                                           { 0x32, "2" },
                                           { 0x33, "3" },
                                           { 0x34, "4" },
                                           { 0x35, "5" },
                                           { 0x36, "6" },
                                           { 0x37, "7" },
                                           { 0x38, "8" },
                                           { 0x39, "9" },
                                           { 0x41, "A" },
                                           { 0x42, "B" },
                                           { 0x43, "C" },
                                           { 0x44, "D" },
                                           { 0x45, "E" },
                                           { 0x46, "F" },
                                           { 0x47, "G" },
                                           { 0x48, "H" },
                                           { 0x49, "I" },
                                           { 0x4A, "J" },
                                           { 0x4B, "K" },
                                           { 0x4C, "L" },
                                           { 0x4D, "M" },
                                           { 0x4E, "N" },
                                           { 0x4F, "O" },
                                           { 0x50, "P" },
                                           { 0x51, "Q" },
                                           { 0x52, "R" },
                                           { 0x53, "S" },
                                           { 0x54, "T" },
                                           { 0x55, "U" },
                                           { 0x56, "V" },
                                           { 0x57, "W" },
                                           { 0x58, "X" },
                                           { 0x59, "Y" },
                                           { 0x5A, "Z" },
                                           { 0x5D, "Applications" },
                                           { 0x5F, "Sleep" },
                                           { 0x60, "Numpad 0" },
                                           { 0x61, "Numpad 1" },
                                           { 0x62, "Numpad 2" },
                                           { 0x63, "Numpad 3" },
                                           { 0x64, "Numpad 4" },
                                           { 0x65, "Numpad 5" },
                                           { 0x66, "Numpad 6" },
                                           { 0x67, "Numpad 7" },
                                           { 0x68, "Numpad 8" },
                                           { 0x69, "Numpad 9" },
                                           { 0x6A, "Numpad Multiply" },
                                           { 0x6B, "Numpad Add" },
                                           { 0x6C, "Numpad Decimal Point" },
                                           { 0x6D, "Numpad Subtract" },
                                           { 0x6E, "Numpad Decimal" },
                                           { 0x6F, "Numpad Divide" },
                                           { 0x70, "F1" },
                                           { 0x71, "F2" },
                                           { 0x72, "F3" },
                                           { 0x73, "F4" },
                                           { 0x74, "F5" },
                                           { 0x75, "F6" },
                                           { 0x76, "F7" },
                                           { 0x77, "F8" },
                                           { 0x78, "F9" },
                                           { 0x79, "F10" },
                                           { 0x7A, "F11" },
                                           { 0x7B, "F12" },
                                           { 0x7C, "F13" },
                                           { 0x7D, "F14" },
                                           { 0x7E, "F15" },
                                           { 0x7F, "F16" },
                                           { 0x80, "F17" },
                                           { 0x81, "F18" },
                                           { 0x82, "F19" },
                                           { 0x83, "F20" },
                                           { 0x84, "F21" },
                                           { 0x85, "F22" },
                                           { 0x86, "F23" },
                                           { 0x87, "F24" },
                                           { 0x90, "Num Lock" },
                                           { 0x91, "Scroll Lock" },
                                           { 0xA6, "Browser Back" },
                                           { 0xA7, "Browser Forward" },
                                           { 0xA8, "Browser Refresh" },
                                           { 0xA9, "Browser Stop" },
                                           { 0xAA, "Browser Search" },
                                           { 0xAB, "Browser Favorites" },
                                           { 0xAC, "Browser Start and Home" },
                                           { 0xAD, "Volume Mute" },
                                           { 0xAE, "Volume Down" },
                                           { 0xAF, "Volume Up" },
                                           { 0xB0, "Next Track" },
                                           { 0xB1, "Previous Track" },
                                           { 0xB2, "Stop Media" },
                                           { 0xB3, "Play/Pause Media" },
                                           { 0xB4, "Start Mail" },
                                           { 0xB5, "Select Media" },
                                           { 0xB6, "Start App1" },
                                           { 0xB7, "Start App2" },
                                           { 0xBA, "; (Semicolon)" },
                                           { 0xBB, "+ (Plus)" },
                                           { 0xBC, ", (Comma)" },
                                           { 0xBD, "- (Minus)" },
                                           { 0xBE, ". (Period)" },
                                           { 0xBF, "/ (Slash)" },
                                           { 0xC0, "` (Backtick)" },
                                           { 0xDB, "[ (Open Square Bracket)" },
                                           { 0xDC, "\\ (Backslash)" },
                                           { 0xDD, "] (Close Square Bracket)" },
                                           { 0xDE, "' (Quote)" } };

    size_t const num_hotkeys = _countof(hotkeys);

    int get_hotkey_index(byte keycode)
    {
        for(int i = 0; i < num_hotkeys; ++i) {
            if(hotkeys[i].key_code == keycode) {
                return i;
            }
        }
        return -1;
    }

    std::string get_hotkey_name(byte keycode, byte modifiers)
    {
        std::string result;
        if(modifiers & keymod_ctrl) {
            result.append("Ctrl ");
        }
        if(modifiers & keymod_alt) {
            result.append("Alt ");
        }
        if(modifiers & keymod_shift) {
            result.append("Shift ");
        }
        if(modifiers & keymod_winkey) {
            result.append("WinKey ");
        }
        int index = get_hotkey_index(keycode);
        if(index >= 0) {
            result.append(std::format("+ {}", hotkeys[index].name));
        } else {
            result.append(std::format("+ KEY_0x{:02x}", keycode));
        }
        return result;
    }
}
