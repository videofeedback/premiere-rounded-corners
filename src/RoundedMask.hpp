#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace rounded {

struct Settings {
    double radius = 10.0;       // Percent of the shorter FINAL frame side (0..50).
    double frameWidth = 100.0;   // Percent of input frame, centered; crops, not scales.
    double frameHeight = 100.0;
    double top = 0.0;           // Additional insets, as percentages of the input.
    double bottom = 0.0;
    double left = 0.0;
    double right = 0.0;
};

inline double finiteClamp(double value, double low, double high) noexcept {
    return std::isfinite(value) ? std::clamp(value, low, high) : low;
}

// All geometry is in the current render buffer's pixel coordinates. The host
// supplies preview scaling and pixel aspect ratio; the core has no Adobe dependency.
class Mask {
public:
    Mask(int width, int height, const Settings& s,
         double scaleX = 1.0, double scaleY = 1.0, double pixelAspect = 1.0) noexcept
        : cx_(0), cy_(0) {
        const double w = std::max(0, width), h = std::max(0, height);
        const double frameScaleX = finiteClamp(s.frameWidth, 0.0, 100.0) / 100.0;
        const double frameScaleY = finiteClamp(s.frameHeight, 0.0, 100.0) / 100.0;
        // Establish the actual rectangle FIRST, including asymmetric black-bar trims.
        const double left = w * ((1.0 - frameScaleX) * 0.5 + finiteClamp(s.left, 0, 100) / 100.0);
        const double right = w * ((1.0 + frameScaleX) * 0.5 - finiteClamp(s.right, 0, 100) / 100.0);
        const double top = h * ((1.0 - frameScaleY) * 0.5 + finiteClamp(s.top, 0, 100) / 100.0);
        const double bottom = h * ((1.0 + frameScaleY) * 0.5 - finiteClamp(s.bottom, 0, 100) / 100.0);
        cx_ = (left + right) * 0.5; cy_ = (top + bottom) * 0.5;
        hx_ = std::max(0.0, right - left) * 0.5;
        hy_ = std::max(0.0, bottom - top) * 0.5;
        scaleX = finiteClamp(scaleX, 0.000001, 16.0);
        scaleY = finiteClamp(scaleY, 0.000001, 16.0);
        pixelAspect = finiteClamp(pixelAspect, 0.000001, 100.0);
        // One radius in full-resolution DISPLAY pixels keeps the contour circular.
        // Conversion to buffer pixels compensates for PAR and nonuniform previews;
        // changing frame aspect ratio must never stretch the corner itself.
        const double finalWidth = 2.0 * hx_ * pixelAspect / scaleX;
        const double finalHeight = 2.0 * hy_ / scaleY;
        const double radius = std::min(finalWidth, finalHeight)
                            * finiteClamp(s.radius, 0.0, 50.0) / 100.0;
        rx_ = radius * scaleX / pixelAspect;
        ry_ = radius * scaleY;
        // Arc centers are (left + rx, top + ry), (right - rx, top + ry), etc.
    }

    bool contains(double x, double y) const noexcept {
        if (hx_ <= 0.0 || hy_ <= 0.0) return false;
        const double ax = std::abs(x - cx_), ay = std::abs(y - cy_);
        if (ax > hx_ || ay > hy_) return false;
        if (rx_ == 0.0 || ax <= hx_ - rx_ || ay <= hy_ - ry_) return true;
        const double ex = (ax - hx_ + rx_) / rx_;
        const double ey = (ay - hy_ + ry_) / ry_;
        return ex * ex + ey * ey <= 1.0;
    }

    double coverage(int x, int y) const noexcept {
        if (hx_ <= 0.0 || hy_ <= 0.0) return 0.0;
        // Rounded rectangles are symmetric and monotonic in each quadrant.
        // Testing the nearest and farthest pixel corners cheaply classifies all
        // pixels except those crossing the contour.
        const double ax = std::abs(x + 0.5 - cx_);
        const double ay = std::abs(y + 0.5 - cy_);
        if (contains(cx_ + ax + 0.5, cy_ + ay + 0.5)) return 1.0;
        if (!contains(cx_ + std::max(0.0, ax - 0.5),
                      cy_ + std::max(0.0, ay - 0.5))) return 0.0;
        int hits = 0;
        constexpr int samples = 8;
        for (int sy = 0; sy < samples; ++sy)
            for (int sx = 0; sx < samples; ++sx)
                hits += contains(x + (sx + 0.5) / samples,
                                 y + (sy + 0.5) / samples);
        return static_cast<double>(hits) / (samples * samples);
    }

private:
    double cx_, cy_, hx_ = 0, hy_ = 0, rx_ = 0, ry_ = 0;
};

template <class Channel>
inline Channel attenuate(Channel value, double coverage) noexcept {
    if constexpr (std::is_floating_point_v<Channel>) {
        return static_cast<Channel>(value * coverage);
    } else {
        return static_cast<Channel>(std::floor(value * coverage + 0.5));
    }
}

// AE-compatible ARGB pixels are premultiplied. Scale RGB with alpha so
// compositing an antialiased corner does not leave a bright fringe.
template <class Pixel>
inline Pixel apply(const Pixel& source, double coverage) noexcept {
    if (coverage >= 1.0) return source;
    if (coverage <= 0.0) return Pixel{};
    Pixel result = source;
    result.alpha = attenuate(source.alpha, coverage);
    result.red = attenuate(source.red, coverage);
    result.green = attenuate(source.green, coverage);
    result.blue = attenuate(source.blue, coverage);
    return result;
}

template <class Pixel>
inline void renderRow(const Pixel* source, Pixel* destination, int width,
                      int y, const Mask& mask) noexcept {
    for (int x = 0; x < width; ++x)
        destination[x] = apply(source[x], mask.coverage(x, y));
}

} // namespace rounded
