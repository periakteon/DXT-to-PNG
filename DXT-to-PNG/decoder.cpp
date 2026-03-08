#include "decoder.h"
#include <algorithm>
#include <cstring>

void UnpackRGB565(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = static_cast<uint8_t>(((c >> 11) & 0x1F) * 255 / 31);
    g = static_cast<uint8_t>(((c >>  5) & 0x3F) * 255 / 63);
    b = static_cast<uint8_t>( (c        & 0x1F) * 255 / 31);
}

void DecompressColorBlock(std::span<const uint8_t, 8> src,
                          std::array<std::array<uint8_t, 4>, 16>& out,
                          bool forceOpaque) {
    const uint16_t c0 = static_cast<uint16_t>(src[0] | (src[1] << 8));
    const uint16_t c1 = static_cast<uint16_t>(src[2] | (src[3] << 8));

    std::array<uint8_t, 4> r{}, g{}, b{}, a{};
    UnpackRGB565(c0, r[0], g[0], b[0]); a[0] = 255;
    UnpackRGB565(c1, r[1], g[1], b[1]); a[1] = 255;

    if (c0 > c1 || forceOpaque) {
        r[2] = static_cast<uint8_t>((2*r[0] + r[1] + 1) / 3);
        g[2] = static_cast<uint8_t>((2*g[0] + g[1] + 1) / 3);
        b[2] = static_cast<uint8_t>((2*b[0] + b[1] + 1) / 3);
        a[2] = 255;
        r[3] = static_cast<uint8_t>((r[0] + 2*r[1] + 1) / 3);
        g[3] = static_cast<uint8_t>((g[0] + 2*g[1] + 1) / 3);
        b[3] = static_cast<uint8_t>((b[0] + 2*b[1] + 1) / 3);
        a[3] = 255;
    } else {
        r[2] = static_cast<uint8_t>((r[0] + r[1]) / 2);
        g[2] = static_cast<uint8_t>((g[0] + g[1]) / 2);
        b[2] = static_cast<uint8_t>((b[0] + b[1]) / 2);
        a[2] = 255;
        r[3] = 0; g[3] = 0; b[3] = 0; a[3] = 0;
    }

    const uint32_t bits = static_cast<uint32_t>(
        src[4] | (src[5] << 8) | (src[6] << 16) | (src[7] << 24));
    for (int i = 0; i < 16; ++i) {
        const int idx = (bits >> (2 * i)) & 0x3;
        out[i] = { r[idx], g[idx], b[idx], a[idx] };
    }
}

std::vector<uint8_t> DecompressDXT1(std::span<const uint8_t> src, int w, int h) {
    std::vector<uint8_t> dst(static_cast<size_t>(w * h * 4), 0);
    const int bw = (w + 3) / 4, bh = (h + 3) / 4;
    size_t srcOff = 0;

    for (int by = 0; by < bh; ++by) {
        for (int bx = 0; bx < bw; ++bx, srcOff += 8) {
            if (srcOff + 8 > src.size()) break;
            std::array<std::array<uint8_t, 4>, 16> c{};
            DecompressColorBlock(std::span<const uint8_t, 8>(src.data() + srcOff, 8), c, false);
            for (int py = 0; py < 4; ++py) for (int px = 0; px < 4; ++px) {
                const int x = bx*4+px, y = by*4+py;
                if (x >= w || y >= h) continue;
                const int pi = py*4+px;
                auto* p = dst.data() + (y*w+x)*4;
                p[0]=c[pi][0]; p[1]=c[pi][1]; p[2]=c[pi][2]; p[3]=c[pi][3];
            }
        }
    }
    return dst;
}

std::vector<uint8_t> DecompressDXT3(std::span<const uint8_t> src, int w, int h) {
    std::vector<uint8_t> dst(static_cast<size_t>(w * h * 4), 0);
    const int bw = (w + 3) / 4, bh = (h + 3) / 4;
    size_t srcOff = 0;

    for (int by = 0; by < bh; ++by) {
        for (int bx = 0; bx < bw; ++bx) {
            if (srcOff + 16 > src.size()) break;
            std::array<uint8_t, 16> alpha{};
            for (int i = 0; i < 8; ++i) {
                alpha[2*i+0] = (src[srcOff+i] & 0x0F) * 17;
                alpha[2*i+1] = (src[srcOff+i] >>    4) * 17;
            }
            srcOff += 8;

            std::array<std::array<uint8_t, 4>, 16> c{};
            DecompressColorBlock(std::span<const uint8_t, 8>(src.data() + srcOff, 8), c, true);
            srcOff += 8;

            for (int py = 0; py < 4; ++py) for (int px = 0; px < 4; ++px) {
                const int x = bx*4+px, y = by*4+py;
                if (x >= w || y >= h) continue;
                const int pi = py*4+px;
                auto* p = dst.data() + (y*w+x)*4;
                p[0]=c[pi][0]; p[1]=c[pi][1]; p[2]=c[pi][2]; p[3]=alpha[pi];
            }
        }
    }
    return dst;
}

std::vector<uint8_t> DecompressDXT5(std::span<const uint8_t> src, int w, int h) {
    std::vector<uint8_t> dst(static_cast<size_t>(w * h * 4), 0);
    const int bw = (w + 3) / 4, bh = (h + 3) / 4;
    size_t srcOff = 0;

    for (int by = 0; by < bh; ++by) {
        for (int bx = 0; bx < bw; ++bx) {
            if (srcOff + 16 > src.size()) break;
            const uint8_t a0 = src[srcOff], a1 = src[srcOff+1];
            std::array<uint8_t, 8> at{};
            at[0] = a0; at[1] = a1;
            if (a0 > a1) {
                at[2] = static_cast<uint8_t>((6*a0 + 1*a1 + 3) / 7);
                at[3] = static_cast<uint8_t>((5*a0 + 2*a1 + 3) / 7);
                at[4] = static_cast<uint8_t>((4*a0 + 3*a1 + 3) / 7);
                at[5] = static_cast<uint8_t>((3*a0 + 4*a1 + 3) / 7);
                at[6] = static_cast<uint8_t>((2*a0 + 5*a1 + 3) / 7);
                at[7] = static_cast<uint8_t>((1*a0 + 6*a1 + 3) / 7);
            } else {
                at[2] = static_cast<uint8_t>((4*a0 + 1*a1 + 2) / 5);
                at[3] = static_cast<uint8_t>((3*a0 + 2*a1 + 2) / 5);
                at[4] = static_cast<uint8_t>((2*a0 + 3*a1 + 2) / 5);
                at[5] = static_cast<uint8_t>((1*a0 + 4*a1 + 2) / 5);
                at[6] = 0;
                at[7] = 255;
            }

            uint64_t bits = 0;
            for (int i = 0; i < 6; ++i)
                bits |= (static_cast<uint64_t>(src[srcOff+2+i]) << (8*i));

            std::array<uint8_t, 16> alpha{};
            for (int i = 0; i < 16; ++i) alpha[i] = at[(bits >> (3*i)) & 0x7];
            srcOff += 8;

            std::array<std::array<uint8_t, 4>, 16> c{};
            DecompressColorBlock(std::span<const uint8_t, 8>(src.data() + srcOff, 8), c, true);
            srcOff += 8;

            for (int py = 0; py < 4; ++py) for (int px = 0; px < 4; ++px) {
                const int x = bx*4+px, y = by*4+py;
                if (x >= w || y >= h) continue;
                const int pi = py*4+px;
                auto* p = dst.data() + (y*w+x)*4;
                p[0]=c[pi][0]; p[1]=c[pi][1]; p[2]=c[pi][2]; p[3]=alpha[pi];
            }
        }
    }
    return dst;
}

std::vector<uint8_t> DecodeA8R8G8B8(std::span<const uint8_t> src, int w, int h) {
    std::vector<uint8_t> dst(src.begin(), src.end());
    for (int i = 0; i < w * h * 4; i += 4)
        std::swap(dst[i], dst[i+2]);  // BGRA -> RGBA
    return dst;
}

std::vector<uint8_t> DecodeX8R8G8B8(std::span<const uint8_t> src, int w, int h) {
    std::vector<uint8_t> dst(src.begin(), src.end());
    for (int i = 0; i < w * h * 4; i += 4) {
        std::swap(dst[i], dst[i+2]);  // BGRA -> RGBA
        dst[i+3] = 255;
    }
    return dst;
}

std::vector<uint8_t> DecodeA1R5G5B5(std::span<const uint8_t> src, int w, int h) {
    std::vector<uint8_t> dst(static_cast<size_t>(w * h * 4));
    for (int i = 0; i < w * h; ++i) {
        const uint16_t pix = static_cast<uint16_t>(src[2*i] | (src[2*i+1] << 8));
        dst[4*i+0] = static_cast<uint8_t>(((pix >> 10) & 0x1F) << 3);
        dst[4*i+1] = static_cast<uint8_t>(((pix >>  5) & 0x1F) << 3);
        dst[4*i+2] = static_cast<uint8_t>( (pix        & 0x1F) << 3);
        dst[4*i+3] = (pix >> 15) ? 255 : 0;
    }
    return dst;
}

std::vector<uint8_t> DecodeA4R4G4B4(std::span<const uint8_t> src, int w, int h) {
    std::vector<uint8_t> dst(static_cast<size_t>(w * h * 4));
    for (int i = 0; i < w * h; ++i) {
        const uint16_t pix = static_cast<uint16_t>(src[2*i] | (src[2*i+1] << 8));
        dst[4*i+0] = static_cast<uint8_t>(((pix >>  8) & 0xF) << 4);
        dst[4*i+1] = static_cast<uint8_t>(((pix >>  4) & 0xF) << 4);
        dst[4*i+2] = static_cast<uint8_t>( (pix        & 0xF) << 4);
        dst[4*i+3] = static_cast<uint8_t>( (pix >> 12)        << 4);
    }
    return dst;
}
