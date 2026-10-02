# Development and verification

## Build

The supported build recipe targets macOS 12+, arm64 and x86_64, using Adobe After
Effects SDK 26.5. Extract the SDK outside the repository or into the ignored `sdk/`
directory. Set `AE_SDK_PATH` or pass the SDK root as the first build-script argument.

```sh
bash scripts/test.sh
bash scripts/build-mac.sh "/path/to/AdobeAfterEffectsSDK_26.5_MacOS"
bash scripts/test-plugin.sh "/path/to/AdobeAfterEffectsSDK_26.5_MacOS"
```

The plugin is written to `build/Rounded Corners.plugin`, with a PiPL resource and
ad-hoc code signature. No Adobe SDK headers are vendored into the repository.

## What the tests verify

`tests/mask_tests.cpp` runs with AddressSanitizer and UndefinedBehaviorSanitizer:

- Independent signed-distance oracle across portrait and wide frame dimensions.
- Asymmetric edge insets and known arc centers after letterbox removal.
- Circular boundary samples, antialiasing and numerical area.
- Non-square pixels and unequal horizontal/vertical preview scaling.
- Empty/crossed frame bounds, zero and maximum radius, nonfinite input.
- Premultiplied alpha multiplication.

`tests/plugin_tests.cpp` loads the actual compiled binary and invokes Adobe SDK
entry points using test callbacks:

- Effect version, stable parameter IDs, seven visible animatable sliders.
- Synthetic letterboxed frames and independent edge controls.
- Exact unchanged pixel values inside the fully opaque frame.
- Transparent bars, corner pixels and crossed frames.
- Padded and negative row strides, render cancellation and invalid dimensions.

The harness itself runs with sanitizers; the loaded plugin is the optimized release
build. These checks do not replace running the plugin inside Premiere.

## Reproduce the documentation images

Requirements: a C++17 compiler, Python 3 and Pillow. On macOS:

```sh
mkdir -p build/docs
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror \
  -I src tools/render_docs.cpp -o build/render_docs
build/render_docs build/docs
python3 tools/compose_docs.py
```

If Pillow is missing, install it into a local virtual environment:

```sh
python3 -m venv build/docs-venv
build/docs-venv/bin/pip install -r tools/requirements-docs.txt
build/docs-venv/bin/python tools/compose_docs.py
```

`render_docs.cpp` generates synthetic source imagery and processes it through
`src/RoundedMask.hpp`. `compose_docs.py` adds labels and layout to that output.
The images are clearly labeled as renderer illustrations, not Premiere screenshots.
The examples use no user footage or personal project files. `DOCS_FONT` can point
to a local TrueType font when neither Arial nor DejaVu Sans is available.

## Host verification still needed

- Discovery after restart in current Premiere Pro and Premiere Pro Beta versions.
- Full, half and quarter playback/paused resolutions.
- Video and image sources, existing alpha, and footage with baked-in letterboxing.
- Animated radius, frame dimensions and asymmetric edges.
- Direct export and Adobe Media Encoder export over a visible background.
- Non-square pixels, interlaced sources and upstream buffer-resizing effects.

## Adobe references

- [After Effects C++ SDK](https://developer.adobe.com/after-effects/)
- [Premiere hosting differences](https://ae-plugins.docsforadobe.dev/ppro/basic-host-differences/)
- [Premiere pixel formats](https://ae-plugins.docsforadobe.dev/ppro/bigger-differences/)
- [Parameter flags](https://ae-plugins.docsforadobe.dev/effect-basics/PF_ParamDef/)
- [PiPL resources](https://ae-plugins.docsforadobe.dev/intro/pipl-resources/)
