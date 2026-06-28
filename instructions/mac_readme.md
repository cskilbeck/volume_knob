We will use a small free tool called **wchisp**. There's no app to click on, you'll type a few commands into a program called **Terminal**. Don't worry if you've never done this before: every command below can be **copied and pasted** into the Terminal.

> These instructions are for **Apple Silicon Macs** (any Mac with an M1, M2, M3 or M4 chip - that's every Mac sold since late 2020). To check: click the  menu (top-left) → **About This Mac**. If it says "Chip: Apple M…", you're good to go.

The first time takes about 5 minutes (mostly the one-time setup in Step 1). After that, re-flashing only takes a minute.

---

### Before you start: how to open Terminal

Terminal is an app that comes with every Mac. To open it:

1. Press **Cmd (⌘) + Space** to open Spotlight search.
2. Type **Terminal** and press **Return**.
3. A window with plain text and a blinking cursor will appear. This is where you'll paste commands.

**How to run a command:** click once inside the Terminal window, paste the command (**Cmd + V**), then press **Return**. That's it. When a command finishes, the blinking cursor comes back and you can paste the next one.

> Tip: Terminal won't show anything when you type or paste a password - no dots, no stars. That's normal. Just type it and press Return.

---

### 1. Download and unlock the flashing tool (one-time setup)

You only ever have to do this **once**. After it's set up, skip straight to Step 2 next time. Nothing here gets "installed" - it's just a single small file that lives in your Downloads folder.

#### 1a. Download wchisp

Click this link to download the tool:

- **[Download wchisp for Mac](https://github.com/ch32-rs/wchisp/releases/download/v0.3.0/wchisp-v0.3.0-macos-arm64.tar.gz)** *(`wchisp-v0.3.0-macos-arm64.tar.gz`)*

> Prefer to see the source? It's on the [wchisp releases page](https://github.com/ch32-rs/wchisp/releases) - choose the file ending in **`macos-arm64.tar.gz`**.

#### 1b. Unzip it

Go to your **Downloads** folder in Finder and **double-click** the file you just downloaded (`wchisp-v0.3.0-macos-arm64.tar.gz`). It will unzip into a single file simply called **`wchisp`**. Leave it in Downloads.

#### 1c. Unlock it so your Mac will run it

Because this tool is downloaded from the internet, macOS locks it by default. We unlock it with two quick commands. **Open Terminal** (see "Before you start" above), then paste these **one at a time**, pressing **Return** after each:

```bash
cd ~/Downloads
```

```bash
xattr -c wchisp && chmod +x wchisp
```

The first command moves Terminal into your Downloads folder (where the `wchisp` file is). The second one unlocks it. Neither prints anything if it works, that's fine.

> **Check it worked:** paste `./wchisp --version` and press Return. If you see a version number (like `wchisp 0.3.0`), you're all set.

> If macOS still pops up a warning that it "cannot verify the developer," click **Cancel**, then go to **System Settings → Privacy & Security**, scroll down, and click **Open Anyway** next to the wchisp message - then run `./wchisp --version` again.

---

### 2. Download the firmware

First work out which version of the knob you have by looking at its USB connector, then download the **matching** firmware and leave it in your **Downloads** folder.

> ⚠️ **Important:** the firmware must match your knob's version. Flashing **Direct** firmware onto an **Original** knob (or the other way round) will stop it working. When in doubt, check the connector before you download.

**Original version** - has a **USB Micro-B** or a **USB-C** socket (you plug a cable into it):

- **Volume knob:** [tiny_usb_knob_2.0.bin](tiny_usb_knob_2.0.bin)
- **MIDI knob:** [tiny_midi_knob_1.0.bin](tiny_midi_knob_1.0.bin)

**Direct version (V1 or V2)** - has a **USB Type-A plug** built in (it plugs straight into your USB port, no cable):

- **Volume knob:** [hid_knob_DIRECT.bin](hid_knob_DIRECT.bin)
- **MIDI knob:** *not available for Direct versions.*

> If your browser warns that the file "can't be downloaded safely," choose **Keep** - it's just a firmware file.

---

### 3. Put the device in Firmware Flashing Mode

1. Unplug the knob if it's plugged in.
2. **Press and hold the button down**, and - while still holding it - plug the knob into your Mac.
3. Keep holding for a moment, then let go.

If it worked, you'll see the **LED flicker for about 1 second** as you plug it in. That flicker means the knob is in Firmware Flashing Mode and ready.

> Didn't see the flicker? Unplug it and try again, making sure you're holding the button down **before** it touches the Mac.

---

### 4. Check that your Mac can see the knob

First, make sure Terminal is still in your Downloads folder (where the `wchisp` file lives). If you closed and reopened Terminal since Step 1, paste this and press **Return**:

```bash
cd ~/Downloads
```

Now paste this and press **Return**:

```bash
./wchisp info
```

- If it prints a few lines of chip details, great - your Mac sees the knob. Continue to Step 5.
- If it says something like **"device not found"**, the knob isn't in flashing mode. Repeat Step 3 and try `./wchisp info` again.

---

### 5. Flash the new firmware

This is the easy part. You're going to type the start of a command and then let the Mac fill in the file location for you:

1. Type this into Terminal, **including the space at the end** - but do **not** press Return yet:

   ```
   ./wchisp flash 
   ```

2. Open your **Downloads** folder in Finder, then **drag the firmware file you downloaded** (e.g. `tiny_usb_knob_2.0.bin`) **into the Terminal window** and let go. The full file location appears automatically after the command.

3. Now press **Return**.

wchisp will erase, write, and double-check the firmware, then restart the knob for you. Wait until it says it's finished and the blinking cursor returns. **Don't unplug the knob while it's flashing.**

---

### 6. Test the device

Once flashing finishes, the knob restarts on its own and works like a normal USB device again. To check it:

- Refresh the [Web Config UI](https://usbknob.com) page (or click **'Scan'**) and confirm the knob shows up and responds.

That's it.

---

### Troubleshooting

- **`./wchisp info` shows "device not found"** - the knob isn't in flashing mode. Redo Step 3 (look for the LED flicker), then try again.
- **`no such file or directory` when you run `./wchisp …`** — Terminal isn't in your Downloads folder. Paste `cd ~/Downloads` and press Return, then try again. (This also happens if you've moved the `wchisp` file out of Downloads.)
- **macOS says it "cannot verify the developer" / "wchisp can't be opened"** — go to **System Settings → Privacy & Security**, scroll down, and click **Open Anyway** next to the wchisp message, then run your command again. (Re-running `xattr -c wchisp` from Step 1c also fixes this.)
- **Anything else** — unplug the knob, close Terminal, reopen it, and start again from Step 4 (`cd ~/Downloads` first). A fresh start fixes most issues.


