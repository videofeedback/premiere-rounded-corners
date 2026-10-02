#include "RoundedMask.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>

struct Pixel { std::uint8_t alpha, red, green, blue; };

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    constexpr int width = 960, height = 600, tileWidth = 420, tileHeight = 240;
    std::ofstream out(argv[1], std::ios::binary);
    if (!out) return 1;
    out << "P6\n" << width << ' ' << height << "\n255\n";
    std::array<rounded::Settings, 4> settings;
    settings[0].radius = 0;
    for (int i = 1; i < 4; ++i) {
        settings[i].radius = 15;
        settings[i].top = 20;
        settings[i].bottom = 15;
    }
    settings[2].left = 35; settings[2].right = 5;
    settings[3].left = 5; settings[3].right = 5;
    settings[3].top = 30; settings[3].bottom = 25;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int column = x / 480, row = y / 300;
            const int lx = x % 480 - 30, ly = y % 300 - 30;
            const bool inside = lx >= 0 && ly >= 0 && lx < tileWidth && ly < tileHeight;
            Pixel result{255, 20, 23, 30};
            if (inside) {
                const int background = ((lx / 12 + ly / 12) % 2) ? 54 : 43;
                result = Pixel{255, static_cast<std::uint8_t>(background),
                              static_cast<std::uint8_t>(background + 3),
                              static_cast<std::uint8_t>(background + 8)};
                Pixel art{255, static_cast<std::uint8_t>(60 + lx * 170 / tileWidth),
                               static_cast<std::uint8_t>(175 - ly * 95 / tileHeight),
                               static_cast<std::uint8_t>(210 - lx * 50 / tileWidth)};
                if ((lx - 300)*(lx - 300) + (ly - 70)*(ly - 70) < 42*42)
                    art = Pixel{255, 255, 214, 145};
                if (ly > 175 - 50 * std::sin(lx / 70.0)) art = Pixel{255, 37, 60, 90};
                if (ly < 48 || ly >= 204) art = Pixel{255,0,0,0};
                const rounded::Mask mask(tileWidth, tileHeight, settings[row*2 + column]);
                const double a = mask.coverage(lx, ly);
                result.red = static_cast<std::uint8_t>(art.red*a + result.red*(1-a));
                result.green = static_cast<std::uint8_t>(art.green*a + result.green*(1-a));
                result.blue = static_cast<std::uint8_t>(art.blue*a + result.blue*(1-a));
            }
            const char rgb[] = {static_cast<char>(result.red), static_cast<char>(result.green),
                                static_cast<char>(result.blue)};
            out.write(rgb, 3);
        }
    }
    return out ? 0 : 1;
}
