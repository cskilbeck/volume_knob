const cc_names = new Map([
    [0, "Bank Select"],
    [1, "Modulation Wheel"],
    [2, "Breath Controller"],
    [3, "Undefined"],
    [4, "Foot Pedal"],
    [5, "Portamento Time"],
    [6, "Data Entry"],
    [7, "Volume"],
    [8, "Balance"],
    [9, "Undefined"],
    [10, "Pan"],
    [11, "Expression"],
    [12, "Effect Controller 1"],
    [13, "Effect Controller 2"],
    [14, "Undefined"],
    [15, "Undefined"],
    [16, "General Purpose "],
    [17, "General Purpose"],
    [18, "General Purpose"],
    [19, "General Purpose"],

    [64, "Damper Pedal on/off"],
    [65, "Portamento on/off"],
    [66, "Sostenuto Pedal on/off"],
    [67, "Soft Pedal on/off"],
    [68, "Legato FootSwitch"],
    [69, "Hold 2"],
    [70, "Sound Controller 1"],
    [71, "Sound Controller 2"],
    [72, "Sound Controller 3"],
    [73, "Sound Controller 4"],
    [74, "Sound Controller 5"],
    [75, "Sound Controller 6"],
    [76, "Sound Controller 7"],
    [77, "Sound Controller 8"],
    [78, "Sound Controller 9"],
    [79, "Sound Controller 10"],
    [80, "General Purpose"],
    [81, "General Purpose"],
    [82, "General Purpose"],
    [83, "General Purpose"],
    [84, "Portamento CC Control"],
    [88, "High Resolution Velocity Prefix"],
    [91, "Effect 1 Depth"],
    [92, "Effect 2 Depth"],
    [93, "Effect 3 Depth"],
    [94, "Effect 4 Depth"],
    [95, "Effect 5 Depth"],
    [96, "(+1) Data Increment"],
    [97, "(-1) Data Decrement"],
    [98, "NRPN LSB"],
    [99, "NRPN MSB"],
    [100, "RPN LSB"],
    [101, "RPN MSB"],
    [120, "All Sound Off"],
    [121, "Reset All Controllers"],
    [122, "Local on/off Switch"],
    [123, "All Notes Off"],
    [124, "Omni Mode Off"],
    [125, "Omni Mode On"],
    [126, "Mono Mode"],
    [127, "Poly Mode"],
]);

function get_cc_name(index) {
    let name = "Undefined";
    if (cc_names.has(index)) {
        name = cc_names.get(index);
    }
    if (index < 31) {
        return `${name} (MSB}`;
    }
    if (index < 64) {
        return `${name} (LSB}`;
    }
    return name;
}
