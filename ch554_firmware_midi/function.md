## When I turn the knob

#### A Control Change value is changed
- It might be a 14 bit or 7 bit value
    - If it's a 14 bit value, the LSB and MSB CC indices are specified
- It might use one of the following modes:
    - absolute      - send 1 or 2 CC values directly (depending on whether it's a 7 bit or 14 bit value)
    - relative 1    - treat 64 as zero
    - relative 2    - treat 128 as zero
    - relative 3    - treat 16 as zero
- If it's absolute, I save the value in flash memory after some period of inactivity (e.g. 1 second)
- It might flash the LED
    - Whenever the knob is turned
    - When an absolute value hits a low limit
    - When an absolute value hits a high limit
- Acceleration may be applied whether it's absolute or relative

## When I click or release the knob

- A Control Change value is set
- It might be a 14 bit or 7 bit value
    - If it's a 14 bit value, the LSB and MSB CC indices are specified
- It will set one of two arbitrary values depending on whether the knob is clicked or released
- It might flash the LED
    - When the knob is clicked
    - When the knob is released
- It might keep the LED lit
    - When the knob is held down
    - When the knob is left alone
