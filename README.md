# LosslessProxy v2.0.0

A transparent DLL proxy for [Lossless Scaling](https://store.steampowered.com/app/993090/Lossless_Scaling/) that adds a plugin/addon system without modifying the original application.

LosslessProxy sits between `LosslessScaling.exe` and its native engine `Lossless.dll`, forwarding all original functionality while enabling third-party addons to extend, customize, and monitor the scaling pipeline.

## How It Works

```
LosslessScaling.exe
  └─ loads Lossless.dll  ← this is LosslessProxy (renamed)
       ├─ loads Lossless_original.dll (the real engine)
       ├─ forwards all 11 exports transparently
       ├─ intercepts shader resource loading
       ├─ loads addons from addons/ directory
       └─ provides ImGui overlay for addon UIs
```

LosslessProxy replaces `Lossless.dll` and renames the original to `Lossless_original.dll`. All function calls (Init, Activate, ApplySettings, etc.) are forwarded via linker-level export forwarding — zero runtime overhead for unhooked calls.

## Installation

### Requirements

- [Lossless Scaling](https://store.steampowered.com/app/993090/Lossless_Scaling/) (Steam)
- Windows 10/11 (x64)
- Visual Studio 2019+ or CMake 3.14+ with MSVC

### Build

```bash
cd LosslessProxy
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

This produces `Lossless.dll` in the build output.

### Deploy

1. Navigate to the Lossless Scaling install folder:
   ```
   C:\Program Files (x86)\Steam\steamapps\common\Lossless Scaling\
   ```
2. Rename the original `Lossless.dll` to `Lossless_original.dll`
3. Copy the built `Lossless.dll` (the proxy) into the same folder
4. Create an `addons\` folder in the install directory
5. Place addon folders inside `addons\` (each addon has its own subfolder)
6. Launch Lossless Scaling normally — the proxy loads automatically

### Uninstall

1. Delete the proxy `Lossless.dll`
2. Rename `Lossless_original.dll` back to `Lossless.dll`
3. Delete the `addons\` folder

## Features

- **Export Forwarding** — All 11 exports (Init, UnInit, Activate, ApplySettings, etc.) forwarded to the original DLL
- **Shader Interception** — Hooks FindResourceW/LoadResource to let addons replace any of the 300 built-in HLSL shaders
- **Addon System** — Full plugin lifecycle: scan, load, initialize, hot-reload settings, shutdown
- **ImGui Overlay** — Shared ImGui context with tabs for addon management, settings, logs, and about
- **Event Bus** — Thread-safe publish/subscribe system for inter-addon communication
- **Configuration** — Per-addon JSON config persisted in `addons/config.json`
- **Security** — SHA-256 hash verification of addon DLLs against `trusted_addons.json`
- **Dependency Resolution** — Topological sort ensures addons load in correct order
- **Crash Isolation** — SEH wrapping prevents a faulting addon from crashing the host

## Addon SDK

Addons are native DLLs (C++17) that export a standard set of functions. The SDK headers are in `sdk/include/lsproxy/`.

### Minimal Addon

```cpp
#include <lsproxy/addon_sdk.h>
#include "imgui.h"

static IHost* g_host = nullptr;

LSPROXY_EXPORT void AddonInitialize(IHost* host, ImGuiContext* ctx,
                                    void* allocFunc, void* freeFunc, void* userData) {
    ImGui::SetCurrentContext(ctx);
    ImGui::SetAllocatorFunctions((ImGuiMemAllocFunc)allocFunc,
                                (ImGuiMemFreeFunc)freeFunc, userData);
    g_host = host;
    g_host->Log(LSPROXY_LOG_INFO, "Hello from my addon!");
}

LSPROXY_EXPORT void AddonShutdown() { g_host = nullptr; }
LSPROXY_EXPORT uint32_t GetAddonCapabilities() { return LSPROXY_CAP_NONE; }
LSPROXY_EXPORT const char* GetAddonName() { return "My Addon"; }
LSPROXY_EXPORT const char* GetAddonVersion() { return "1.0.0"; }
LSPROXY_EXPORT const char* GetAddonAuthor() { return "Me"; }
LSPROXY_EXPORT const char* GetAddonDescription() { return "Does something cool."; }
```

### Required Exports

| Export | Signature | Purpose |
|--------|-----------|---------|
| `AddonInitialize` | `void(IHost*, ImGuiContext*, void*, void*, void*)` | Called when addon loads. Receive host interface and shared ImGui context. |
| `AddonShutdown` | `void()` | Called on unload. Clean up resources. |
| `GetAddonCapabilities` | `uint32_t()` | Return capability bitmask. |

### Optional Exports

| Export | Signature | Purpose |
|--------|-----------|---------|
| `AddonRenderSettings` | `void()` | Render ImGui settings UI (requires `LSPROXY_CAP_HAS_SETTINGS`). |
| `AddonInterceptResource` | `bool(const wchar_t*, const wchar_t*, const void**, uint32_t*)` | Intercept and replace shader resources. |
| `GetAddonName` | `const char*()` | Display name. |
| `GetAddonVersion` | `const char*()` | Version string. |
| `GetAddonAuthor` | `const char*()` | Author name. |
| `GetAddonDescription` | `const char*()` | Short description. |

### Capabilities

```cpp
LSPROXY_CAP_NONE             // No special capabilities
LSPROXY_CAP_HAS_SETTINGS     // Provides AddonRenderSettings()
LSPROXY_CAP_REQUIRES_RESTART // Enable/disable requires app restart
LSPROXY_CAP_PATCH_LS1_LOGIC  // Request JMP patching for LS1 algorithm
```

### IHost Interface

Available to addons via the pointer received in `AddonInitialize`:

```cpp
host->Log(LSPROXY_LOG_INFO, "message");              // Thread-safe logging
host->GetConfig("addon-id", "key", "default");       // Read persisted config
host->SetConfig("addon-id", "key", "value");          // Write config
host->SaveConfig();                                    // Flush to disk
host->GetHostVersion();                                // Proxy version
host->SubscribeEvent(eventId, callback, userData);     // Event subscription
host->UnsubscribeEvent(eventId, callback);
host->PublishEvent(eventId, data, dataSize);            // Broadcast event
```

### Events

| Event | ID | Data |
|-------|-----|------|
| `LSPROXY_EVENT_ADDON_LOADED` | 1 | `LsProxyAddonEventData` |
| `LSPROXY_EVENT_ADDON_UNLOADED` | 2 | `LsProxyAddonEventData` |
| `LSPROXY_EVENT_SETTINGS_CHANGED` | 3 | — |
| `LSPROXY_EVENT_SHADER_INTERCEPTED` | 4 | `LsProxyShaderEventData` |
| `LSPROXY_EVENT_HOST_SHUTDOWN` | 5 | — |
| `LSPROXY_EVENT_CUSTOM` | 0x10000+ | User-defined |

### Addon Folder Structure

```
addons/
  MyAddon/
    addon.json          # Manifest (name, version, author, description, tags)
    MyAddon.dll         # The addon DLL
    icon.png            # Optional icon (shown in addon manager UI)
```

### addon.json Manifest

```json
{
    "name": "My Addon",
    "version": "1.0.0",
    "author": "Author Name",
    "description": "What it does",
    "min_host_version": "2.0.0",
    "dependencies": [],
    "tags": ["category"],
    "dll": "CustomName.dll",
    "icon": "icon.png"
}
```

## Available Addons

| Addon | Description |
|-------|-------------|
| [LSP-ReShade](Addons/LSP-ReShade/) | Keyboard/mouse passthrough for ReShade overlays |
| [LSP-Windowed](Addons/LSP-Windowed/) | Windowed mode via virtual monitor injection |

## Project Structure

```
LosslessProxy/
  CMakeLists.txt
  sdk/
    include/lsproxy/     # Public SDK headers
      addon_sdk.h        # Master include
      addon_exports.h    # Export declarations and capabilities
      events.h           # Event definitions
      ihost.h            # Host interface
      version.h          # Version defines
    examples/            # Example addon
  src/
    core/                # DLL entry, export forwarding, shader hooks
    addon/               # Addon scanning, loading, security, dependencies
    event/               # Thread-safe event bus
    host/                # IHost implementation
    config/              # JSON config manager
    log/                 # File + in-memory logger
    gui/                 # ImGui overlay (tabs, widgets, addon cards)
  third_party/           # nlohmann/json, stb_image
Addons/
  LSP-ReShade/           # ReShade input passthrough
  LSP-Windowed-Mode/     # Virtual monitor / windowed mode
  LSP-Shader-Replace/    # Custom shader injection
  LSP-Settings-Tuner/    # Internal parameter tuner
  LSP-Game-Profiles/     # Per-game auto profiles
  LSP-Perf-Overlay/      # Performance overlay
```

## License

See [LICENSE](LICENSE) for details.


## ⚠️ Disclaimer

This project is an unofficial add-on manager for Lossless Scaling.

It is NOT affiliated with, endorsed by, or supported by
Lossless Scaling Developers in any way.

This project is based on reverse engineering and independent analysis
of the software behavior. No proprietary source code, assets, or
confidential materials from Lossless Scaling are included.


Use at your own risk.

Using this project may violate the terms of service or license
agreement of Lossless Scaling.

The author assumes no responsibility for any damage, data loss,
account bans, or legal consequences resulting from the use of this
software.


## Legal Notice / Trademarks

All trademarks, product names, and company names are the property
of their respective owners and are used for identification purposes only.


Each add-on is distributed as a separate project and may have
its own license and disclaimer.

