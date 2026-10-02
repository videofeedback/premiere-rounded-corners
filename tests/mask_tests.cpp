#include "RoundedMask.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

struct Pixel { std::uint8_t alpha, red, green, blue; };

// Independent analytic signed-distance oracle for circular rounded rectangles.
bool circleRectangle(double x, double y, double left, double top,
                     double right, double bottom, double r) {
    if (right <= left || bottom <= top) return false;
    const double qx = std::abs(x - (left + right)/2) - ((right-left)/2-r);
    const double qy = std::abs(y - (top + bottom)/2) - ((bottom-top)/2-r);
    return std::hypot(std::max(qx, 0.0), std::max(qy, 0.0))
         + std::min(std::max(qx, qy), 0.0) <= r;
}

int main() {
    using rounded::Mask;
    rounded::Settings s;
    s.radius = 0;
    Mask square(64, 32, s);
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 64; ++x) assert(square.coverage(x, y) == 1.0);
    s.radius = 37.5; // 37.5% of 32 = 12px in BOTH directions.
    Mask m(64, 32, s);
    assert(m.coverage(0, 0) == 0.0);
    assert(m.coverage(32, 16) == 1.0);
    assert(m.coverage(32, 0) == 1.0);
    bool antialiased = false;
    double area = 0;
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 64; ++x) {
            double a = m.coverage(x, y);
            assert(a >= 0 && a <= 1);
            assert(a == m.coverage(63 - x, y));
            assert(a == m.coverage(x, 31 - y));
            antialiased |= a > 0 && a < 1;
            area += a;
        }
    }
    assert(antialiased);
    const double expected = 64 * 32 - (4 - std::acos(-1.0)) * 12 * 12;
    assert(std::abs(area - expected) < 1.0);

    // Sweep portrait/wide frames with asymmetric insets. The circle radius and
    // its centers must be based on the FINAL rectangle, not the source center.
    for (double fw : {25.0, 50.0, 80.0, 100.0}) {
        for (double fh : {25.0, 50.0, 80.0, 100.0}) {
            for (double radius : {0.0, 10.0, 25.0, 50.0}) {
                rounded::Settings a;
                a.frameWidth = fw; a.frameHeight = fh; a.radius = radius;
                a.left = 8; a.right = 3; a.top = 12; a.bottom = 4;
                const double l = 200*((1-fw/100)/2+.08), r = 200*((1+fw/100)/2-.03);
                const double t = 100*((1-fh/100)/2+.12), b = 100*((1+fh/100)/2-.04);
                const double cr = std::min(r-l, b-t)*radius/100;
                Mask frame(200, 100, a);
                for (int y = 0; y < 100; ++y)
                    for (int x = 0; x < 200; ++x)
                        assert(frame.contains(x+.371, y+.619) ==
                               circleRectangle(x+.371, y+.619, l, t, r, b, cr));
            }
        }
    }

    // A top letterbox is explicitly excluded. Top-left arc center is (24,54),
    // from the selected left=10, top=40 and 14px radius (10% of 140px height).
    s = {}; s.left = 5; s.right = 10; s.top = 20; s.bottom = 10;
    Mask letterbox(200, 200, s);
    assert(letterbox.contains(24,40));
    assert(!letterbox.contains(10,40));
    assert(letterbox.contains(10,54));
    assert(!letterbox.contains(100,39.99));
    assert(!letterbox.contains(180.01,100));
    assert(!letterbox.contains(100,180.01));
    // Equal distances from a circular arc's center, tested on both sides of it.
    for (int degrees = 5; degrees < 90; degrees += 5) {
        double angle = degrees * std::acos(-1.0)/180;
        assert(letterbox.contains(24-13.99*std::cos(angle),54-13.99*std::sin(angle)));
        assert(!letterbox.contains(24-14.01*std::cos(angle),54-14.01*std::sin(angle)));
    }
    // PAR=2 halves horizontal buffer radius, but displayed geometry is circular.
    Mask nonSquarePixels(100, 200, s, 1, 1, 2);
    Mask half(100, 100, s, .5, .5);
    Mask unequalPreview(100, 50, s, .5, .25);
    for (int y = 0; y < 200; ++y)
        for (int x = 0; x < 200; ++x) {
            const double px = x+.371, py = y+.619;
            assert(letterbox.contains(px, py) == nonSquarePixels.contains(px/2, py));
            assert(letterbox.contains(px, py) == half.contains(px/2, py/2));
            assert(letterbox.contains(px, py) == unequalPreview.contains(px/2, py/4));
        }
    s.top = 90; s.bottom = 20;
    Mask crossed(200, 200, s);
    assert(crossed.coverage(100,100) == 0);
    s = {}; s.frameWidth = 0;
    Mask empty(64, 32, s);
    assert(empty.coverage(32,16) == 0);
    s = {}; s.radius = std::numeric_limits<double>::quiet_NaN();
    Mask safe(1,1,s);
    assert(std::isfinite(safe.coverage(0,0)));
    s = {}; s.radius = 1000;
    Mask circle(100,100,s);
    area = 0;
    for (int y = 0; y < 100; ++y)
        for (int x = 0; x < 100; ++x) area += circle.coverage(x,y);
    assert(std::abs(area - std::acos(-1.0)*2500) < 2);
    Pixel p{128,100,50,25};
    Pixel p0 = rounded::apply(p,0), p1 = rounded::apply(p,1), ph = rounded::apply(p,.5);
    assert(p0.alpha == 0 && p0.red == 0);
    assert(p1.alpha == 128 && p1.blue == 25);
    assert(ph.alpha == 64 && ph.red == 50 && ph.green == 25 && ph.blue == 13);
    std::cout << "PASS: circular geometry oracle, asymmetric edges, letterbox arc centers, "
                 "AA/area, PAR, unequal preview scales, empty/crossed bounds, premultiplied alpha\n";
}
