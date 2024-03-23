//////////////////////////////////////////////////////////////////////

const key_modifiers = {
    "Ctrl": 0x01,
    "Shift": 0x02,
    "Alt": 0x04,
};

//////////////////////////////////////////////////////////////////////

// tacky - " None" has a space at the front so it sorts to the top of the list

const hid_keys = {
    0x00: " None",
    0x04: "A",
    0x05: "B",
    0x06: "C",
    0x07: "D",
    0x08: "E",
    0x09: "F",
    0x0a: "G",
    0x0b: "H",
    0x0c: "I",
    0x0d: "J",
    0x0e: "K",
    0x0f: "L",
    0x10: "M",
    0x11: "N",
    0x12: "O",
    0x13: "P",
    0x14: "Q",
    0x15: "R",
    0x16: "S",
    0x17: "T",
    0x18: "U",
    0x19: "V",
    0x1a: "W",
    0x1b: "X",
    0x1c: "Y",
    0x1d: "Z",
    0x1e: "1",
    0x1f: "2",
    0x20: "3",
    0x21: "4",
    0x22: "5",
    0x23: "6",
    0x24: "7",
    0x25: "8",
    0x26: "9",
    0x27: "0",
    0x28: "Enter",
    0x29: "Esc",
    0x2a: "Backspace",
    0x2b: "Tab",
    0x2c: "Space",
    0x2d: "Minus",
    0x2e: "Equal",
    0x2f: "Left brace",
    0x30: "Right brace",
    0x31: "Backslash",
    0x32: "Tilde",
    0x33: "Semicolon",
    0x34: "Apostrophe",
    0x35: "Grave",
    0x36: "Comma",
    0x37: "Dot",
    0x38: "Slash",
    0x39: "Caps lock",
    0x3a: "F1",
    0x3b: "F2",
    0x3c: "F3",
    0x3d: "F4",
    0x3e: "F5",
    0x3f: "F6",
    0x40: "F7",
    0x41: "F8",
    0x42: "F9",
    0x43: "F10",
    0x44: "F11",
    0x45: "F12",
    0x46: "Sysrq",
    0x47: "Scroll lock",
    0x48: "Pause",
    0x49: "Insert",
    0x4a: "Home",
    0x4b: "Page up",
    0x4c: "Delete",
    0x4d: "End",
    0x4e: "Page down",
    0x4f: "Right",
    0x50: "Left",
    0x51: "Down",
    0x52: "Up",
    0x53: "Num lock",
    0x54: "Keypad Slash",
    0x55: "Keypad Asterisk",
    0x56: "Keypad Minus",
    0x57: "Keypad Plus",
    0x58: "Keypad Enter",
    0x59: "Keypad 1",
    0x5a: "Keypad 2",
    0x5b: "Keypad 3",
    0x5c: "Keypad 4",
    0x5d: "Keypad 5",
    0x5e: "Keypad 6",
    0x5f: "Keypad 7",
    0x60: "Keypad 8",
    0x61: "Keypad 9",
    0x62: "Keypad 0",
    0x63: "Keypad Dot",
    0x65: "Compose",
    0x66: "Power",
    0x67: "Keypad Equal",
    0x68: "F13",
    0x69: "F14",
    0x6a: "F15",
    0x6b: "F16",
    0x6c: "F17",
    0x6d: "F18",
    0x6e: "F19",
    0x6f: "F20",
    0x70: "F21",
    0x71: "F22",
    0x72: "F23",
    0x73: "F24",
    0x74: "Open",
    0x75: "Help",
    0x76: "Props",
    0x77: "Front",
    0x78: "Stop",
    0x79: "Again",
    0x7a: "Undo",
    0x7b: "Cut",
    0x7c: "Copy",
    0x7d: "Paste",
    0x7e: "Find",
    0x7f: "Mute",
    0x80: "Volume up",
    0x81: "Volume down",
    0x85: "Keypad comma",
    0x86: "Keypad equal",
    0x87: "RO",
    0x88: "Katakana Hiragana",
    0x89: "Yen",
    0x8a: "Henkan",
    0x8b: "Muhenkan",
    0x8c: "Keypad JP Comma",
    0x8d: "International 7",
    0x8e: "International 8",
    0x8f: "International 9",
    0x90: "Hangeul",
    0x91: "Hanja",
    0x92: "Katakana",
    0x93: "Hiragana",
    0x94: "Zenkakuhankaku",
    0x95: "Lang 6",
    0x96: "Lang 7",
    0x97: "Lang 8",
    0x98: "Lang 9",
    0x99: "Alternate Erase",
    0x9a: "SysReq/Attention",
    0x9b: "Cancel",
    0x9c: "Clear",
    0x9d: "Prior",
    0x9e: "Return",
    0x9f: "Separator",
    0xa0: "Out",
    0xa1: "Oper",
    0xa2: "Clear/Again",
    0xa3: "CrSel/Props",
    0xa4: "ExSel",
    0xb0: "Keypad 00",
    0xb1: "Keypad 000",
    0xb2: "Thousands",
    0xb3: "Decimal",
    0xb4: "Currency",
    0xb5: "Sub Currency",
    0xb6: "Keypad Left paren",
    0xb7: "Keypad Right paren",
    0xb8: "Keypad Open brace",
    0xb9: "Keypad Close brace",
    0xba: "Keypad Tab",
    0xbb: "Keypad Backspace",
    0xbc: "Keypad A",
    0xbd: "Keypad B",
    0xbe: "Keypad C",
    0xbf: "Keypad D",
    0xc0: "Keypad E",
    0xc1: "Keypad F",
    0xc2: "Keypad Xor",
    0xc3: "Keypad Eor",
    0xc4: "Keypad Percent",
    0xc5: "Keypad Less",
    0xc6: "Keypad Greater",
    0xc7: "Keypad Ampersand",
    0xc8: "Keypad Double ampersand",
    0xc9: "Keypad Bar",
    0xca: "Keypad Double bar",
    0xcb: "Keypad Colon",
    0xcc: "Keypad Pound",
    0xcd: "Keypad Space",
    0xce: "Keypad AT",
    0xcf: "Keypad Exclam",
    0xd0: "Keypad Memory Store",
    0xd1: "Keypad Memory Recall",
    0xd2: "Keypad Memory Clear",
    0xd3: "Keypad Memory Add",
    0xd4: "Keypad Memory Subtract",
    0xd5: "Keypad Memory Multiply",
    0xd6: "Keypad Memory Divide",
    0xd7: "Keypad Plus/Minus",
    0xd8: "Keypad Clear",
    0xd9: "Keypad Clear Entry",
    0xda: "Keypad Binary",
    0xdb: "Keypad Octal",
    0xdc: "Keypad Decimal",
    0xdd: "Keypad Hexadecimal",
    0xe0: "Left Ctrl",
    0xe1: "Left Shift",
    0xe2: "Left Alt",
    0xe3: "Left Meta",
    0xe4: "Right Ctrl",
    0xe5: "Right Shift",
    0xe6: "Right Alt",
    0xe7: "Right Meta"
};

//////////////////////////////////////////////////////////////////////

const consumer_control_keys = {
    0x20: "Plus 10",
    0x21: "Plus 100",
    0x22: "AM/PM",
    0x30: "Power",
    0x31: "Reset",
    0x32: "Sleep",
    0x33: "Sleep After",
    0x34: "Sleep Mode",
    0x35: "Illumination",
    0x36: "Function Buttons",
    0x40: "Menu",
    0x41: "Menu/Pick",
    0x42: "Menu Up",
    0x43: "Menu Down",
    0x44: "Menu Left",
    0x45: "Menu Right",
    0x46: "Menu Escape",
    0x47: "Menu Value Increase",
    0x48: "Menu Value Decrease",
    0x60: "Data On Screen",
    0x61: "Closed Caption",
    0x62: "Closed Caption/Select",
    0x63: "VCR/TV",
    0x64: "Broadcast Mode",
    0x65: "Snapshot",
    0x66: "Still",
    0x80: "Selection",
    0x81: "Assign Selection",
    0x82: "Mode Step",
    0x83: "Recall/Last",
    0x84: "Enter Channel",
    0x85: "Order Movie",
    0x86: "Channel",
    0x87: "Media Selection",
    0x88: "Media Select Computer",
    0x89: "Media Select TV",
    0x8A: "Media Select WWW",
    0x8B: "Media Select DVD",
    0x8C: "Media Select Telephone",
    0x8D: "Media Select Program Guide",
    0x8E: "Media Select Video Phone",
    0x8F: "Media Select Games",
    0x90: "Media Select Messages",
    0x91: "Media Select CD",
    0x92: "Media Select VCR",
    0x93: "Media Select Tuner",
    0x94: "Quit",
    0x95: "Help",
    0x96: "Media Select Tape",
    0x97: "Media Select Cable",
    0x98: "Media Select Satellite",
    0x99: "Media Select Security",
    0x9A: "Media Select Home",
    0x9B: "Media Select Call",
    0x9C: "Channel Increment",
    0x9D: "Channel Decrement",
    0x9E: "Media Select SAP",
    0xA0: "VCR Plus",
    0xA1: "Once",
    0xA2: "Daily",
    0xA3: "Weekly",
    0xA4: "Monthly",
    0xB0: "Play",
    0xB1: "Pause",
    0xB2: "Record",
    0xB3: "Fast Forward",
    0xB4: "Rewind",
    0xB5: "Scan Next Track",
    0xB6: "Scan Previous Track",
    0xB7: "Stop",
    0xB8: "Eject",
    0xB9: "Random Play",
    0xBA: "Select Disc",
    0xBB: "Enter Disc",
    0xBC: "Repeat",
    0xBD: "Tracking",
    0xBE: "Track Normal",
    0xBF: "Slow Tracking",
    0xC0: "Frame Forward",
    0xC1: "Frame Back",
    0xC2: "Mark",
    0xC3: "Clear Mark",
    0xC4: "Repeat From Mark",
    0xC5: "Return To Mark",
    0xC6: "Search Mark Forward",
    0xC7: "Search Mark Backwards",
    0xC8: "Counter Reset",
    0xC9: "Show Counter",
    0xCA: "Tracking Increment",
    0xCB: "Tracking Decrement",
    0xE0: "Volume",
    0xE1: "Balance",
    0xE2: "Mute",
    0xE3: "Bass",
    0xE4: "Treble",
    0xE5: "Bass Boost",
    0xE6: "Surround Mode",
    0xE7: "Loudness",
    0xE8: "MPX",
    0xE9: "Volume Up",
    0xEA: "Volume Down",
    0xF0: "Speed Select",
    0xF1: "Playback Speed",
    0xF2: "Standard Play",
    0xF3: "Long Play",
    0xF4: "Extended Play",
    0xF5: "Slow",

    0x100: "Fan Enable",
    0x101: "Fan Speed",
    0x102: "Light",
    0x103: "Light Illumination Level",
    0x104: "Climate Control Enable",
    0x105: "Room Temperature",
    0x106: "Security Enable",
    0x107: "Fire Alarm",
    0x108: "Police Alarm",
    0x150: "Balance Right",
    0x151: "Balance Left",
    0x152: "Bass Increment",
    0x153: "Bass Decrement",
    0x154: "Treble Increment",
    0x155: "Treble Decrement",
    0x160: "Speaker System",
    0x161: "Channel Left",
    0x162: "Channel Right",
    0x163: "Channel Center",
    0x164: "Channel Front",
    0x165: "Channel Center Front",
    0x166: "Channel Side",
    0x167: "Channel Surround",
    0x168: "Channel Low Frequency Enhancement",
    0x169: "Channel Top",
    0x16A: "Channel Unknown",
    0x170: "Sub-channel",
    0x171: "Sub-channel Increment",
    0x172: "Sub-channel Decrement",
    0x173: "Alternate Audio Increment",
    0x174: "Alternate Audio Decrement",

    0x184: "App: Word Processor",
    0x185: "App: Text Editor",
    0x186: "App: Spreadsheet",
    0x187: "App: Graphics Editor",
    0x188: "App: Presentation App",
    0x189: "App: Database App",
    0x18A: "App: Email Reader",
    0x18B: "App: Newsreader",
    0x18C: "App: Voicemail",
    0x18D: "App: Contacts Address Book",
    0x18E: "App: Calendar Schedule",
    0x18F: "App: Task Project Manager",
    0x190: "App: Log Journal Timecard",
    0x191: "App: Checkbook Finance",
    0x192: "App: Calculator",
    0x193: "App: AV Capture Playback",
    0x194: "App: Local Machine Browser",
    0x195: "App: LAN WAN Browser",
    0x196: "App: Internet Browser",
    0x197: "App: Remote Networking ISP Connect",
    0x198: "App: Network Conference",
    0x199: "App: Network Chat",
    0x19A: "App: Telephony Dialer",
    0x19B: "App: Logon",
    0x19C: "App: Logoff",
    0x19D: "App: Logon Logoff",
    0x19E: "App: Terminal Lock Screensaver",
    0x19F: "App: Control Panel",
    0x1A0: "App: Command Line Processor Run",
    0x1A1: "App: Process Task Manager",
    0x1A2: "App: Select Tast Application",
    0x1A3: "App: Next Task Application",
    0x1A4: "App: Previous Task Application",
    0x1A5: "App: Preemptive Halt Task Application",

    0x201: "App Control: New",
    0x202: "App Control: Open",
    0x203: "App Control: Close",
    0x204: "App Control: Exit",
    0x205: "App Control: Maximize",
    0x206: "App Control: Minimize",
    0x207: "App Control: Save",
    0x208: "App Control: Print",
    0x209: "App Control: Properties",
    0x21A: "App Control: Undo",
    0x21B: "App Control: Copy",
    0x21C: "App Control: Cut",
    0x21D: "App Control: Paste",
    0x21E: "App Control: Select All",
    0x21F: "App Control: Find",
    0x220: "App Control: Find and Replace",
    0x221: "App Control: Search",
    0x222: "App Control: Go To",
    0x223: "App Control: Home",
    0x224: "App Control: Back",
    0x225: "App Control: Forward",
    0x226: "App Control: Stop",
    0x227: "App Control: Refresh",
    0x228: "App Control: Previous Link",
    0x229: "App Control: Next Link",
    0x22A: "App Control: Bookmarks",
    0x22B: "App Control: History",
    0x22C: "App Control: Subscriptions",
    0x22D: "App Control: Zoom In",
    0x22E: "App Control: Zoom Out",
    0x22F: "App Control: Zoom",
    0x230: "App Control: Full Screen View",
    0x231: "App Control: Normal View",
    0x232: "App Control: View Toggle",
    0x233: "App Control: Scroll Up",
    0x234: "App Control: Scroll Down",
    0x235: "App Control: Scroll",
    0x236: "App Control: Pan Left",
    0x237: "App Control: Pan Right",
    0x238: "App Control: Pan",
    0x239: "App Control: New Window",
    0x23A: "App Control: Tile Horizontally",
    0x23B: "App Control: Tile Vertically",
    0x23C: "App Control: Format",
};

let platform = navigator.platform.toUpperCase();

let isMac = platform.indexOf('MAC') >= 0;
let isPC = platform.indexOf('WIN32') >= 0 || platform.indexOf('LINUX') >= 0;

if (isMac) {
    key_modifiers["Cmd"] = 0x08;
    console.log("Platform == MAC");
} else if (isPC) {
    key_modifiers["Win"] = 0x08;
    console.log("Platform == Windows");
} else {
    // Neither Mac nor PC nor Linux
    key_modifiers["GUI"] = 0x08;
    console.log("Platform == OTHER");
}

//////////////////////////////////////////////////////////////////////

let key_codes = [];

let key_names = {};
let consumer_names = {};

for (const [key, value] of Object.entries(hid_keys)) {
    key_codes.push({
        name: value,
        keycode: key,
        is_consumer_key: false
    });
    key_names[value] = key;
}

for (const [key, value] of Object.entries(consumer_control_keys)) {
    key_codes.push({
        name: value,
        keycode: key,
        is_consumer_key: true
    });
    consumer_names[value] = key;
}

key_codes.sort((x, y) => {

    if (x.is_consumer_key && !y.is_consumer_key) {
        return 1;
    }
    if (!x.is_consumer_key && y.is_consumer_key) {
        return -1;
    }
    return x.name.localeCompare(y.name);
});

//////////////////////////////////////////////////////////////////////

export default {

    key_modifiers,

    key_codes,
    key_names,

    consumer_names,

    hid_keys,

    consumer_control_keys
}
