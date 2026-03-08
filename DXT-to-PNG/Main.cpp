#define _CRT_SECURE_NO_WARNINGS
/*
  N3TexConverter - Knight Online .dxt texture to PNG converter
  Usage:
    N3TexConverter input.dxt
    N3TexConverter input.dxt output.png

  Supported formats: DXT1, DXT3, DXT5, A8R8G8B8, X8R8G8B8, A1R5G5B5, A4R4G4B4, TGA
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// ---------------------------------------------------------------------------
// RGB565 unpack

static void UnpackRGB565(uint16_t c, uint8_t* r, uint8_t* g, uint8_t* b) {
    *r = (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
    *g = (uint8_t)(((c >>  5) & 0x3F) * 255 / 63);
    *b = (uint8_t)( (c        & 0x1F) * 255 / 31);
}

// Decompress one DXT color block (8 bytes) into 16 RGBA pixels.
// forceOpaque: skip the DXT1 1-bit transparency mode (used inside DXT3/5).
static void DecompressColorBlock(const uint8_t* src, uint8_t out[16][4], bool forceOpaque) {
    uint16_t c0 = (uint16_t)(src[0] | (src[1] << 8));
    uint16_t c1 = (uint16_t)(src[2] | (src[3] << 8));

    uint8_t r[4], g[4], b[4], a[4];
    UnpackRGB565(c0, &r[0], &g[0], &b[0]); a[0] = 255;
    UnpackRGB565(c1, &r[1], &g[1], &b[1]); a[1] = 255;

    if (c0 > c1 || forceOpaque) {
        r[2] = (uint8_t)((2*r[0] + r[1] + 1) / 3); g[2] = (uint8_t)((2*g[0] + g[1] + 1) / 3); b[2] = (uint8_t)((2*b[0] + b[1] + 1) / 3); a[2] = 255;
        r[3] = (uint8_t)((r[0] + 2*r[1] + 1) / 3); g[3] = (uint8_t)((g[0] + 2*g[1] + 1) / 3); b[3] = (uint8_t)((b[0] + 2*b[1] + 1) / 3); a[3] = 255;
    } else {
        r[2] = (r[0] + r[1]) / 2; g[2] = (g[0] + g[1]) / 2; b[2] = (b[0] + b[1]) / 2; a[2] = 255;
        r[3] = 0; g[3] = 0; b[3] = 0; a[3] = 0;
    }

    uint32_t bits = (uint32_t)(src[4] | (src[5] << 8) | (src[6] << 16) | (src[7] << 24));
    for (int i = 0; i < 16; ++i) {
        int idx = (bits >> (2 * i)) & 0x3;
        out[i][0] = r[idx]; out[i][1] = g[idx]; out[i][2] = b[idx]; out[i][3] = a[idx];
    }
}

// ---------------------------------------------------------------------------
// DXT1 decompressor

static uint8_t* DecompressDXT1(const uint8_t* src, int w, int h) {
    uint8_t* dst = (uint8_t*)calloc(w * h, 4);
    int bw = (w + 3) / 4, bh = (h + 3) / 4;

    for (int by = 0; by < bh; ++by) {
        for (int bx = 0; bx < bw; ++bx, src += 8) {
            uint8_t c[16][4];
            DecompressColorBlock(src, c, false);
            for (int py = 0; py < 4; ++py) for (int px = 0; px < 4; ++px) {
                int x = bx*4+px, y = by*4+py;
                if (x >= w || y >= h) continue;
                int pi = py*4+px;
                uint8_t* p = dst + (y*w+x)*4;
                p[0]=c[pi][0]; p[1]=c[pi][1]; p[2]=c[pi][2]; p[3]=c[pi][3];
            }
        }
    }
    return dst;
}

// ---------------------------------------------------------------------------
// DXT3 decompressor

static uint8_t* DecompressDXT3(const uint8_t* src, int w, int h) {
    uint8_t* dst = (uint8_t*)calloc(w * h, 4);
    int bw = (w + 3) / 4, bh = (h + 3) / 4;

    for (int by = 0; by < bh; ++by) {
        for (int bx = 0; bx < bw; ++bx) {
            // 8 bytes explicit alpha (4-bit per pixel, expand to 8-bit)
            uint8_t alpha[16];
            for (int i = 0; i < 8; ++i) {
                alpha[2*i+0] = (src[i] & 0x0F) * 17;
                alpha[2*i+1] = (src[i] >>   4) * 17;
            }
            src += 8;

            uint8_t c[16][4];
            DecompressColorBlock(src, c, true);
            src += 8;

            for (int py = 0; py < 4; ++py) for (int px = 0; px < 4; ++px) {
                int x = bx*4+px, y = by*4+py;
                if (x >= w || y >= h) continue;
                int pi = py*4+px;
                uint8_t* p = dst + (y*w+x)*4;
                p[0]=c[pi][0]; p[1]=c[pi][1]; p[2]=c[pi][2]; p[3]=alpha[pi];
            }
        }
    }
    return dst;
}

// ---------------------------------------------------------------------------
// DXT5 decompressor

static uint8_t* DecompressDXT5(const uint8_t* src, int w, int h) {
    uint8_t* dst = (uint8_t*)calloc(w * h, 4);
    int bw = (w + 3) / 4, bh = (h + 3) / 4;

    for (int by = 0; by < bh; ++by) {
        for (int bx = 0; bx < bw; ++bx) {
            // Interpolated alpha
            uint8_t a0 = src[0], a1 = src[1];
            uint8_t at[8];
            at[0] = a0; at[1] = a1;
            if (a0 > a1) {
                at[2] = (uint8_t)((6*a0 + 1*a1 + 3) / 7);
                at[3] = (uint8_t)((5*a0 + 2*a1 + 3) / 7);
                at[4] = (uint8_t)((4*a0 + 3*a1 + 3) / 7);
                at[5] = (uint8_t)((3*a0 + 4*a1 + 3) / 7);
                at[6] = (uint8_t)((2*a0 + 5*a1 + 3) / 7);
                at[7] = (uint8_t)((1*a0 + 6*a1 + 3) / 7);
            } else {
                at[2] = (uint8_t)((4*a0 + 1*a1 + 2) / 5);
                at[3] = (uint8_t)((3*a0 + 2*a1 + 2) / 5);
                at[4] = (uint8_t)((2*a0 + 3*a1 + 2) / 5);
                at[5] = (uint8_t)((1*a0 + 4*a1 + 2) / 5);
                at[6] = 0;
                at[7] = 255;
            }

            // 48-bit index table (6 bytes, sixteen 3-bit indices)
            uint64_t bits = 0;
            for (int i = 0; i < 6; ++i) bits |= ((uint64_t)src[2+i] << (8*i));

            uint8_t alpha[16];
            for (int i = 0; i < 16; ++i) alpha[i] = at[(bits >> (3*i)) & 0x7];
            src += 8;

            uint8_t c[16][4];
            DecompressColorBlock(src, c, true);
            src += 8;

            for (int py = 0; py < 4; ++py) for (int px = 0; px < 4; ++px) {
                int x = bx*4+px, y = by*4+py;
                if (x >= w || y >= h) continue;
                int pi = py*4+px;
                uint8_t* p = dst + (y*w+x)*4;
                p[0]=c[pi][0]; p[1]=c[pi][1]; p[2]=c[pi][2]; p[3]=alpha[pi];
            }
        }
    }
    return dst;
}

// ---------------------------------------------------------------------------
// Read an entire file into a malloc'd buffer. Returns NULL on error.

static uint8_t* ReadFile(const char* path, long* outSize) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t* buf = (uint8_t*)malloc(sz);
    if (!buf) { fclose(fp); return NULL; }
    fread(buf, 1, sz, fp);
    fclose(fp);
    if (outSize) *outSize = sz;
    return buf;
}

// ---------------------------------------------------------------------------
// Build default output path: replace extension with .png

static void MakeOutputPath(const char* input, char* out, size_t outLen) {
    strncpy(out, input, outLen - 1);
    out[outLen - 1] = '\0';
    char* dot = strrchr(out, '.');
    if (dot) strcpy(dot, ".png");
    else strncat(out, ".png", outLen - strlen(out) - 1);
}

// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: N3TexConverter <input.dxt> [output.png]\n");
        return 1;
    }

    const char* inputPath = argv[1];

    long fileSize = 0;
    uint8_t* data = ReadFile(inputPath, &fileSize);
    if (!data) {
        fprintf(stderr, "Error: Cannot open '%s'\n", inputPath);
        return 1;
    }

    // ---- TGA detection: check footer signature ----
    const char* tgaSig = "TRUEVISION-XFILE.";
    int tgaSigLen = (int)strlen(tgaSig);
    if (fileSize > tgaSigLen + 1) {
        const char* footer = (const char*)(data + fileSize - tgaSigLen - 1);
        if (memcmp(footer, tgaSig, tgaSigLen) == 0) {
            fprintf(stderr, "Error: TGA-wrapped textures are not supported by this converter.\n");
            free(data);
            return 1;
        }
    }

    // ---- Parse N3 DXT header ----
    int offset = 0;

    if (fileSize < 4) { fprintf(stderr, "Error: File too small\n"); free(data); return 1; }
    int32_t nameLen = *(int32_t*)(data + offset); offset += 4;

    if (nameLen < 0 || nameLen > 200 || offset + nameLen > fileSize) {
        fprintf(stderr, "Error: Invalid name length %d\n", nameLen);
        free(data); return 1;
    }

    char name[256] = {};
    memcpy(name, data + offset, nameLen); offset += nameLen;

    if (offset + 3 + 1 + 4 + 4 + 8 > fileSize) {
        fprintf(stderr, "Error: File truncated in header\n");
        free(data); return 1;
    }

    offset += 3;                                // skip "NTF"
    uint8_t special = data[offset]; offset += 1;

    if (special & 0x04) {
        fprintf(stderr, "Error: Encrypted textures are not supported\n");
        free(data); return 1;
    }

    int32_t width  = *(int32_t*)(data + offset); offset += 4;
    int32_t height = *(int32_t*)(data + offset); offset += 4;

    uint8_t* typeData = data + offset; offset += 8;

    if (width <= 0 || height <= 0 || width > 8192 || height > 8192) {
        fprintf(stderr, "Error: Invalid dimensions %dx%d\n", width, height);
        free(data); return 1;
    }

    const uint8_t* pixels = data + offset;

    // ---- Identify format and decompress ----
    char typeStr[9] = {};
    memcpy(typeStr, typeData, 8);

    const char* formatName = "unknown";
    uint8_t* rgba = NULL;

    if (strstr(typeStr, "DXT1")) {
        formatName = "DXT1";
        rgba = DecompressDXT1(pixels, width, height);
    } else if (strstr(typeStr, "DXT3")) {
        formatName = "DXT3";
        rgba = DecompressDXT3(pixels, width, height);
    } else if (strstr(typeStr, "DXT5")) {
        formatName = "DXT5";
        rgba = DecompressDXT5(pixels, width, height);
    } else if (typeData[0] == 21) {
        formatName = "A8R8G8B8";
        long expected = (long)width * height * 4;
        rgba = (uint8_t*)malloc(expected);
        memcpy(rgba, pixels, expected);
        for (int i = 0; i < width * height * 4; i += 4) {
            uint8_t t = rgba[i]; rgba[i] = rgba[i+2]; rgba[i+2] = t;  // BGRA -> RGBA
        }
    } else if (typeData[0] == 22) {
        formatName = "X8R8G8B8";
        long expected = (long)width * height * 4;
        rgba = (uint8_t*)malloc(expected);
        memcpy(rgba, pixels, expected);
        for (int i = 0; i < width * height * 4; i += 4) {
            uint8_t t = rgba[i]; rgba[i] = rgba[i+2]; rgba[i+2] = t;
            rgba[i+3] = 255;  // discard stored alpha
        }
    } else if (typeData[0] == 25) {
        formatName = "A1R5G5B5";
        rgba = (uint8_t*)malloc((size_t)width * height * 4);
        for (int i = 0; i < width * height; ++i) {
            uint16_t pix = (uint16_t)(pixels[2*i] | (pixels[2*i+1] << 8));
            rgba[4*i+0] = (uint8_t)(((pix >> 10) & 0x1F) << 3);
            rgba[4*i+1] = (uint8_t)(((pix >>  5) & 0x1F) << 3);
            rgba[4*i+2] = (uint8_t)( (pix        & 0x1F) << 3);
            rgba[4*i+3] = (pix >> 15) ? 255 : 0;
        }
    } else if (typeData[0] == 26) {
        formatName = "A4R4G4B4";
        rgba = (uint8_t*)malloc((size_t)width * height * 4);
        for (int i = 0; i < width * height; ++i) {
            uint16_t pix = (uint16_t)(pixels[2*i] | (pixels[2*i+1] << 8));
            rgba[4*i+0] = (uint8_t)(((pix >>  8) & 0xF) << 4);
            rgba[4*i+1] = (uint8_t)(((pix >>  4) & 0xF) << 4);
            rgba[4*i+2] = (uint8_t)( (pix        & 0xF) << 4);
            rgba[4*i+3] = (uint8_t)( (pix >> 12)        << 4);
        }
    } else {
        fprintf(stderr, "Error: Unknown format bytes [%d,%d,%d,%d,%d,%d,%d,%d]\n",
            typeData[0],typeData[1],typeData[2],typeData[3],
            typeData[4],typeData[5],typeData[6],typeData[7]);
        free(data); return 1;
    }

    free(data);

    if (!rgba) {
        fprintf(stderr, "Error: Decompression failed (out of memory?)\n");
        return 1;
    }

    // ---- Write PNG ----
    char outputPath[512];
    if (argc >= 3)
        strncpy(outputPath, argv[2], sizeof(outputPath) - 1);
    else
        MakeOutputPath(inputPath, outputPath, sizeof(outputPath));

    printf("Name:   %s\n", name);
    printf("Format: %s\n", formatName);
    printf("Size:   %d x %d\n", width, height);

    int ok = stbi_write_png(outputPath, width, height, 4, rgba, width * 4);
    free(rgba);

    if (ok) {
        printf("Saved:  %s\n", outputPath);
        return 0;
    } else {
        fprintf(stderr, "Error: Failed to write '%s'\n", outputPath);
        return 1;
    }
}
