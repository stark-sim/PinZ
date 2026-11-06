# PinZ

<img src="docs/images/pinZ-icon.svg" alt="PinZ icon" width="128">

Fast screenshot and pinning tool for GNOME on Linux, designed for low-latency capture and always-on-top references.

[![Platform](https://img.shields.io/badge/platform-GNOME%20Wayland%20%7C%20Linux-4b8bbe)](#platform-status)
[![Qt](https://img.shields.io/badge/Qt-6.x-41cd52)](https://www.qt.io/)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

> [!NOTE]
> PinZ is a GNOME-focused product built from the MIT-licensed EShot codebase. The visible product direction and documentation use PinZ, while some binary names, settings paths, desktop IDs, and packaging files still use the legacy `EShot` name until the technical rename is completed.

## What PinZ does

- Capture a screen region with a low-latency selection overlay
- Annotate with pen, arrow, line, rectangle, ellipse, text, highlighter, blur, pixelate, counter, eraser, and eyedropper tools
- Pin captures above other windows for reference while you work
- Run local OCR from the capture overlay or directly from a pinned image
- Copy, save, upload, or visually search the selected region
- Record the selected region as GIF or MP4
- Use global hotkeys, direct actions, command-line triggers, and a compact control window

## Why it is fast

The normal GNOME screenshot path waits until the hotkey is pressed, then asks the portal for a new screenshot. PinZ optionally moves that work out of the user-waiting path:

```text
screen changes
→ persistent, user-approved ScreenCast stream
→ PipeWire/GStreamer receives frames in the background
→ only the newest frame is retained

hotkey
→ consume the latest frame
→ show the selection overlay immediately
```

Fast Capture currently:

- Uses GNOME's XDG Desktop Portal; it does not bypass screen permission
- Keeps GNOME's screen-sharing indicator visible
- Restores the authorized stream with the portal restore token
- Limits the stream to 10 FPS and keeps only the newest frame
- Shares the mapped PipeWire buffer with `QImage` instead of copying every 4K frame
- Falls back to the normal one-shot Screenshot portal when the stream is unavailable, stale, unsupported, or revoked

## Verified GNOME performance

Measured on GNOME Wayland with the Qt overlay running through XWayland and a 3840×2160 PipeWire frame:

| Stage | Measured |
| --- | ---: |
| Overlay window presentation returned | 10 ms |
| First frame painted | 17 ms |
| Pointer movement accepted | 34 ms |
| Pointer press accepted | 150 ms |
| Selection completed | 250 ms |
| Persistent stream CPU usage | 0.2% over 5 seconds |

Pointer timings include deliberate waits in the kernel-input test harness. The overlay's own first-frame path is already near one display frame.

## Platform status

| Platform | Status |
| --- | --- |
| GNOME Wayland, single monitor | Primary target; Fast Capture available |
| GNOME Wayland, multiple monitors | Works through the normal portal fallback |
| KDE Plasma Wayland | Inherited EShot support; not the current PinZ focus |
| Windows / macOS | Inherited code remains, but PinZ product testing currently targets GNOME |

## Quick start

PinZ has not published renamed release packages yet. Build from source:

```bash
./scripts/linux/install-ubuntu-deps.sh
./scripts/linux/check-linux-runtime.sh
./scripts/linux/build-linux.sh
./dist-linux/bin/EShot
```

For the current development build:

```bash
cmake --build build -j8
./build/bin/EShot --control
```

The executable and desktop integration still use the legacy `EShot` names. That is expected until the technical rename lands.

## Enable GNOME Fast Capture

1. Start the app.
2. Open **Settings → Capture**.
3. Enable **Fast capture**.
4. Approve GNOME's ScreenCast picker once.

After approval, PinZ reuses the persistent stream. To revoke it, disable Fast Capture or stop screen sharing from GNOME's sharing indicator.

## Core shortcuts

| Shortcut | Action |
| --- | --- |
| `Print Screen` | Start region capture |
| Drag | Select a region |
| `Ctrl+C` / `Enter` | Copy selection |
| `Ctrl+S` | Save selection |
| `Ctrl+P` | Pin selection to the screen |
| `Ctrl+O` | OCR selection |
| `Ctrl+G` | Record selection as GIF |
| `Ctrl+Shift+V` | Record selection as video |
| `Esc` | Cancel or close |
| `?` / `F1` | Show the shortcut sheet |

## Product and engineering records

- [GNOME product record](docs/gnome-product-record.zh-CN.md): product direction, naming, architecture, measured performance, limitations, and roadmap
- [Linux dependency setup](docs/linux-dependency-setup.md)
- [Linux port test notes](docs/linux-port-test.md)

## Current limitations

- Fast Capture supports one GNOME Wayland monitor; multi-monitor capture uses the portal fallback
- The overlay still converts the latest `QImage` to `QPixmap` when capture starts
- The overlay currently runs through XWayland for consistent input and pinned-window behavior
- Scrolling/long screenshot is not implemented yet
- Binary names, App IDs, desktop files, settings paths, and release packaging still use EShot names
- DMA-BUF/GPU-texture capture is a later QRhi/OpenGL/Vulkan-level optimization, not part of the current Qt Widgets overlay

## Issues and feedback

Open an issue in [stark-sim/PinZ](https://github.com/stark-sim/PinZ/issues). Include your GNOME version, Wayland session, monitor layout, and whether **Fast capture** was enabled.

## Attribution and license

PinZ is built from [EShot](https://github.com/Benoks/EShot), which is MIT licensed. The upstream license is retained in [LICENSE](LICENSE), and third-party licenses are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

PinZ itself is released under the [MIT License](LICENSE).
