# LSP-Windowed-Mode — Virtual Monitor / Windowed Mode

Enables windowed and borderless windowed mode for Lossless Scaling by injecting a virtual monitor. Lossless Scaling normally requires exclusive fullscreen — this addon tricks it into running in a window.

## What It Does

The addon hooks DXGI monitor enumeration APIs to inject a fake virtual monitor (`0xBADF00D`). Lossless Scaling sees this as a valid display target, allowing it to run in a window instead of requiring fullscreen.

Supported modes:

- **Split-Screen** — Position Lossless Scaling on half the screen (left/right/top/bottom), with the game on the other half
- **Window Positioning** — Place the Lossless Scaling window next to the target game window at a custom position

## Installation

### Build

```bash
cd Addons/LSP-Windowed-Mode
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Deploy

1. Create a folder: `<Lossless Scaling>/addons/LSP-Windowed-Mode/`
2. Copy `LSP_Windowed.dll` and `addon.json` into that folder
3. Restart Lossless Scaling

## Configuration

Open the LosslessProxy overlay and navigate to the Windowed Mode settings tab.

| Setting | Description |
|---------|-------------|
| Mode | Split-Screen or Window Positioning |
| Split Direction | Left, Right, Top, or Bottom (split-screen mode) |
| Target Window | Which window to position next to (positioning mode) |

## How It Works

- Uses **MinHook** to intercept DXGI's `IMonitorEnumSink` and related interfaces
- Injects a fake monitor with handle `0xBADF00D` into the enumeration results
- Hooks `IDXGIOutput::GetDesc` to return fabricated monitor geometry
- Calculates window rectangles for split-screen or side-by-side layouts
- Integrates with D3D11 for Direct3D-aware window management

## Requirements

- LosslessProxy v0.3.0+
- Dependencies: MinHook (fetched automatically by CMake)





## ⚠️ Disclaimer

This add-on is an unofficial extension for Lossless Scaling.

It is NOT affiliated with, endorsed by, or supported by
Lossless Scaling Developers in any way.

This add-on was developed through independent analysis and
reverse engineering of the software behavior. No proprietary
source code or assets are included.

Use at your own risk.

The author assumes no responsibility for any damage, data loss,
account bans, or other consequences resulting from the use of
this add-on.

This add-on may interact with the software at runtime in
non-documented ways.


## Legal Notice / Trademarks

All trademarks, product names, and company names are the property
of their respective owners and are used for identification purposes only.

