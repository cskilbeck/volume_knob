const ANY = 0;
const MSB = 1;
const LSB = 2;

// exactly 127 CCs

const CCs = [
    /* 0:*/ { alt: 32, type: MSB, name: "Bank Select" },
    /* 1:*/ { alt: 33, type: MSB, name: "Modulation Wheel" },
    /* 2:*/ { alt: 34, type: MSB, name: "Breath Controller" },
    /* 3:*/ { alt: 35, type: MSB, name: "Unassigned" },
    /* 4:*/ { alt: 36, type: MSB, name: "Foot Pedal" },
    /* 5:*/ { alt: 37, type: MSB, name: "Portamento Time" },
    /* 6:*/ { alt: 38, type: MSB, name: "Data Entry" },
    /* 7:*/ { alt: 39, type: MSB, name: "Volume" },
    /* 8:*/ { alt: 40, type: MSB, name: "Balance" },
    /* 9:*/ { alt: 41, type: MSB, name: "Unassigned" },
    /* 10:*/ { alt: 42, type: MSB, name: "Pan" },
    /* 11:*/ { alt: 43, type: MSB, name: "Expression" },
    /* 12:*/ { alt: 44, type: MSB, name: "Effect Controller 1" },
    /* 13:*/ { alt: 45, type: MSB, name: "Effect Controller 2" },
    /* 14:*/ { alt: 46, type: MSB, name: "Unassigned" },
    /* 15:*/ { alt: 47, type: MSB, name: "General Purpose" },
    /* 16:*/ { alt: 48, type: MSB, name: "Unassigned" },
    /* 17:*/ { alt: 49, type: MSB, name: "General Purpose" },
    /* 18:*/ { alt: 50, type: MSB, name: "General Purpose" },
    /* 19:*/ { alt: 51, type: MSB, name: "General Purpose" },
    /* 20:*/ { alt: 52, type: MSB, name: "Unassigned" },
    /* 21:*/ { alt: 53, type: MSB, name: "Unassigned" },
    /* 22:*/ { alt: 54, type: MSB, name: "Unassigned" },
    /* 23:*/ { alt: 55, type: MSB, name: "Unassigned" },
    /* 24:*/ { alt: 56, type: MSB, name: "Unassigned" },
    /* 25:*/ { alt: 57, type: MSB, name: "Unassigned" },
    /* 26:*/ { alt: 58, type: MSB, name: "Unassigned" },
    /* 27:*/ { alt: 59, type: MSB, name: "Unassigned" },
    /* 28:*/ { alt: 60, type: MSB, name: "Unassigned" },
    /* 29:*/ { alt: 61, type: MSB, name: "Unassigned" },
    /* 30:*/ { alt: 62, type: MSB, name: "Unassigned" },
    /* 31:*/ { alt: 63, type: MSB, name: "Unassigned" },
    /* 32:*/ { alt: 0, type: LSB, name: "Bank Select" },
    /* 33:*/ { alt: 1, type: LSB, name: "Modulation Wheel" },
    /* 34:*/ { alt: 2, type: LSB, name: "Breath Controller" },
    /* 35:*/ { alt: 3, type: LSB, name: "Unassigned" },
    /* 36:*/ { alt: 4, type: LSB, name: "Foot Pedal" },
    /* 37:*/ { alt: 5, type: LSB, name: "Portamento Time" },
    /* 38:*/ { alt: 6, type: LSB, name: "Data Entry" },
    /* 39:*/ { alt: 7, type: LSB, name: "Volume" },
    /* 40:*/ { alt: 8, type: LSB, name: "Balance" },
    /* 41:*/ { alt: 9, type: LSB, name: "Unassigned" },
    /* 42:*/ { alt: 10, type: LSB, name: "Pan" },
    /* 43:*/ { alt: 11, type: LSB, name: "Expression" },
    /* 44:*/ { alt: 12, type: LSB, name: "Effect Controller 1" },
    /* 45:*/ { alt: 13, type: LSB, name: "Effect Controller 2" },
    /* 46:*/ { alt: 14, type: LSB, name: "Unassigned" },
    /* 47:*/ { alt: 15, type: LSB, name: "General Purpose" },
    /* 48:*/ { alt: 16, type: LSB, name: "Unassigned" },
    /* 49:*/ { alt: 17, type: LSB, name: "General Purpose" },
    /* 50:*/ { alt: 18, type: LSB, name: "General Purpose" },
    /* 51:*/ { alt: 19, type: LSB, name: "General Purpose" },
    /* 52:*/ { alt: 20, type: LSB, name: "Unassigned" },
    /* 53:*/ { alt: 21, type: LSB, name: "Unassigned" },
    /* 54:*/ { alt: 22, type: LSB, name: "Unassigned" },
    /* 55:*/ { alt: 23, type: LSB, name: "Unassigned" },
    /* 56:*/ { alt: 24, type: LSB, name: "Unassigned" },
    /* 57:*/ { alt: 25, type: LSB, name: "Unassigned" },
    /* 58:*/ { alt: 26, type: LSB, name: "Unassigned" },
    /* 59:*/ { alt: 27, type: LSB, name: "Unassigned" },
    /* 60:*/ { alt: 28, type: LSB, name: "Unassigned" },
    /* 61:*/ { alt: 29, type: LSB, name: "Unassigned" },
    /* 62:*/ { alt: 30, type: LSB, name: "Unassigned" },
    /* 63:*/ { alt: 31, type: LSB, name: "Unassigned" },
    /* 64:*/ { alt: 0, type: ANY, name: "Damper Pedal on/off" },
    /* 65:*/ { alt: 0, type: ANY, name: "Portamento on/off" },
    /* 66:*/ { alt: 0, type: ANY, name: "Sostenuto Pedal on/off" },
    /* 67:*/ { alt: 0, type: ANY, name: "Soft Pedal on/off" },
    /* 68:*/ { alt: 0, type: ANY, name: "Legato FootSwitch" },
    /* 69:*/ { alt: 0, type: ANY, name: "Hold 2" },
    /* 70:*/ { alt: 0, type: ANY, name: "Sound Controller 1" },
    /* 71:*/ { alt: 0, type: ANY, name: "Sound Controller 2" },
    /* 72:*/ { alt: 0, type: ANY, name: "Sound Controller 3" },
    /* 73:*/ { alt: 0, type: ANY, name: "Sound Controller 4" },
    /* 74:*/ { alt: 0, type: ANY, name: "Sound Controller 5" },
    /* 75:*/ { alt: 0, type: ANY, name: "Sound Controller 6" },
    /* 76:*/ { alt: 0, type: ANY, name: "Sound Controller 7" },
    /* 77:*/ { alt: 0, type: ANY, name: "Sound Controller 8" },
    /* 78:*/ { alt: 0, type: ANY, name: "Sound Controller 9" },
    /* 79:*/ { alt: 0, type: ANY, name: "Sound Controller 10" },
    /* 80:*/ { alt: 0, type: ANY, name: "General Purpose" },
    /* 81:*/ { alt: 0, type: ANY, name: "General Purpose" },
    /* 82:*/ { alt: 0, type: ANY, name: "General Purpose" },
    /* 83:*/ { alt: 0, type: ANY, name: "General Purpose" },
    /* 84:*/ { alt: 0, type: ANY, name: "Portamento CC Control" },
    /* 85:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 86:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 87:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 88:*/ { alt: 0, type: ANY, name: "High Res Velocity Prefix" },
    /* 89:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 90:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 91:*/ { alt: 0, type: ANY, name: "Effect 1 Depth" },
    /* 92:*/ { alt: 0, type: ANY, name: "Effect 2 Depth" },
    /* 93:*/ { alt: 0, type: ANY, name: "Effect 3 Depth" },
    /* 94:*/ { alt: 0, type: ANY, name: "Effect 4 Depth" },
    /* 95:*/ { alt: 0, type: ANY, name: "Effect 5 Depth" },
    /* 96:*/ { alt: 0, type: ANY, name: "(+1) Data Increment" },
    /* 97:*/ { alt: 0, type: ANY, name: "(-1) Data Decrement" },
    /* 98:*/ { alt: 99, type: LSB, name: "NRPN" },
    /* 99:*/ { alt: 98, type: MSB, name: "NRPN" },
    /* 100:*/ { alt: 101, type: LSB, name: "RPN" },
    /* 101:*/ { alt: 102, type: MSB, name: "RPN" },
    /* 102:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 103:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 104:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 105:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 106:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 107:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 108:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 109:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 110:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 111:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 112:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 113:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 114:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 115:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 116:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 117:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 118:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 119:*/ { alt: 0, type: ANY, name: "Unassigned" },
    /* 120:*/ { alt: 0, type: ANY, name: "All Sound Off" },
    /* 121:*/ { alt: 0, type: ANY, name: "Reset All Controllers" },
    /* 122:*/ { alt: 0, type: ANY, name: "Local on/off Switch" },
    /* 123:*/ { alt: 0, type: ANY, name: "All Notes Off" },
    /* 124:*/ { alt: 0, type: ANY, name: "Omni Mode Off" },
    /* 125:*/ { alt: 0, type: ANY, name: "Omni Mode On" },
    /* 126:*/ { alt: 0, type: ANY, name: "Mono Mode" },
    /* 127:*/ { alt: 0, type: ANY, name: "Poly Mode" },
];

function is_LSB(cc) {
    return cc.type == LSB;
}

function is_MSB(cc) {
    return cc.type == MSB;
}

export default {
    CCs,
    is_LSB,
    is_MSB

};
