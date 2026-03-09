# DXT-to-PNG — Knight Online Texture Converter

Converts Knight Online `.dxt` texture files to standard `.png` images.

Supports both a **graphical interface** (double-click to launch) and a **command-line interface** (pass arguments directly).

## Supported Formats

| Format | Description |
|--------|-------------|
| DXT1 | BC1 compressed, 1-bit alpha |
| DXT3 | BC2 compressed, explicit 4-bit alpha |
| DXT5 | BC3 compressed, interpolated alpha |
| A8R8G8B8 | 32-bit uncompressed with alpha |
| X8R8G8B8 | 32-bit uncompressed, no alpha |
| A1R5G5B5 | 16-bit with 1-bit alpha |
| A4R4G4B4 | 16-bit with 4-bit alpha |

> **Note:** Encrypted textures and TGA-wrapped textures are not supported.

---

## GUI Mode

Launch the app with no arguments (double-click the `.exe` or run it without any flags).

![GUI screenshot placeholder]

### Features

- **Drag-and-drop** — drop `.dxt` files directly onto the window
- **Add Files** — multi-select file picker filtered to `.dxt`
- **Batch conversion** — queue any number of files and convert them all at once
- **Parallel processing** — each file converts on its own thread
- **Per-file status** — Pending / Converting / Done / Error with inline error messages
- **Output folder** — pick a custom destination, or leave blank to save next to each source file
- **Progress bar** — live `X / N` completion counter

---

## CLI Mode

Pass arguments to run headlessly from a terminal (cmd, PowerShell, etc.).

```
DXT-to-PNG <input.dxt> [output.png]
```

### Arguments

| Argument | Required | Description |
|----------|----------|-------------|
| `input.dxt` | Yes | Path to the Knight Online `.dxt` texture file |
| `output.png` | No | Output path. Defaults to the same folder and stem as the input |

### Examples

Convert with auto-generated output name:
```
DXT-to-PNG texture.dxt
```
Produces `texture.png` in the same folder.

Convert with a custom output path:
```
DXT-to-PNG texture.dxt C:\output\my_texture.png
```

### Output

On success the converter prints the texture info and saved path:

```
Name:   my_texture
Format: DXT5
Size:   256 x 256
Saved:  texture.png
```

---

## Building

Open `DXT-to-PNG.slnx` in **Visual Studio 2022** and build (`Ctrl+Shift+B`).

Output locations:
- Debug:   `DXT-to-PNG\x64\Debug\DXT-to-PNG.exe`
- Release: `DXT-to-PNG\x64\Release\DXT-to-PNG.exe`

---

## Dependencies

| Library | Purpose |
|---------|---------|
| [`stb_image_write.h`](https://github.com/nothings/stb) | Single-header PNG writer (vendored) |
| [Dear ImGui](https://github.com/ocornut/imgui) | Immediate-mode GUI (vendored, master branch) |
| DirectX 11 | Rendering backend for ImGui (ships with Windows SDK) |
