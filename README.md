# DXT-to-PNG — Knight Online Texture Converter

Converts Knight Online `.dxt` texture files to standard `.png` images.

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

## Usage

```
DXT-to-PNG <input.dxt> [output.png]
```

### Arguments

| Argument | Required | Description |
|----------|----------|-------------|
| `input.dxt` | Yes | Path to the Knight Online `.dxt` texture file |
| `output.png` | No | Path for the output PNG. Defaults to the same name as the input |

### Examples

Convert with auto-generated output name:
```
DXT-to-PNG texture.dxt
```
This produces `texture.png` in the same folder.

Convert with a custom output path:
```
DXT-to-PNG texture.dxt C:\output\my_texture.png
```

## Output

On success the converter prints the texture info and the saved path:

```
Name:   my_texture
Format: DXT5
Size:   256 x 256
Saved:  texture.png
```

## Building

Open `DXT-to-PNG.slnx` in **Visual Studio 2022** and build the solution (`Ctrl+Shift+B`).

The executable is written to:
- Debug:   `DXT-to-PNG\x64\Debug\DXT-to-PNG.exe`
- Release: `DXT-to-PNG\x64\Release\DXT-to-PNG.exe`

## Dependencies

- [`stb_image_write.h`](https://github.com/nothings/stb) — single-header PNG writer (included in the project)
