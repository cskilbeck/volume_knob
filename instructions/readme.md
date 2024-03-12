## Tiny USB Midi Knob - Instructions for flashing new firmware

#### 1.  Install <a href="https://www.wch-ic.com/downloads/WCHISPTool_Setup_exe.html" target="_blank">WCHISPTool</a>

#### 2. Download [tiny_midi_knob_1.0.bin](tiny_midi_knob_1.0.bin)

#### 3. Put the device in Firmware Flashing Mode
- If you are converting a HID Volume Knob (i.e. it has never been flashed with Midi Firmware):
    - Press and hold the button for more than 5 seconds
- If it's already a Midi Knob (i.e. you bought a Midi Knob or you've already flashed it with the Midi Firmware at least once):
    - Activate Firmware Flashing Mode using the 'Advanced' button in the [Web Config UI](https://midi.skilbeck.com)
![screenshot_1]
    - When the device goes into Firmware Flashing Mode you should see the LED flicker briefly.

#### 4. Run WCHISPTool
- It should look something like this
![isp_tool_1]
- Click 'E8051 USB MCUs' to bring up the correct page
![isp_tool_2]
- Your device should show up in the 'Dev List'. If it doesn't try clicking 'Search'
- Click the browse button for 'Object File 1' and select `tiny_midi_knob_1.0.bin` that you downloaded in step 3.
![isp_tool_3]
- Make sure 'Run The Target Program After Download' is checked
![isp_tool_5]
- Click 'Download' to flash the device
![isp_tool_6]
- Inspect the 'Download Record' for any errors
- Test the device - you will need to refresh the [Web Config UI](https://midi.skilbeck.com) page or click 'Scan'.

[screenshot_1]:./content/screenshot_1.png

[isp_tool_1]:./content/isp_tool_1.png
[isp_tool_2]:./content/isp_tool_2.png
[isp_tool_3]:./content/isp_tool_3.png
[isp_tool_4]:./content/isp_tool_4.png
[isp_tool_5]:./content/isp_tool_5.png
[isp_tool_6]:./content/isp_tool_6.png

