# Changelog

## 0.3.0 — 2026-10-01

- Add independent Top, Bottom, Left and Right frame insets.
- Calculate one circular radius from the shorter side of the final frame.
- Position arc centers from the final, possibly asymmetric, rectangle.
- Retire the old pixel radius and independent corner-axis multipliers; preserve
  their saved parameter IDs as hidden, unused values.
- Add tests for letterboxed sources, circular arc centers and unchanged source pixels.

## 0.2.0 — 2026-10-01

- Scale horizontal and vertical radii with Frame Width and Height.
- This could produce elliptical corners; superseded by the circular geometry in 0.3.0.

## 0.1.0 — 2026-10-01

- Initial native macOS prototype, adjustable radius and centered frame dimensions.
- Antialiased mask, premultiplied alpha and geometry test harness.
