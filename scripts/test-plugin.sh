#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
sdk_dir="${AE_SDK_PATH:-${1:-$project_dir/sdk/AdobeAfterEffectsSDK_26.5_MacOS}}"
xcrun clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I "$sdk_dir/Examples/Headers" -I "$sdk_dir/Examples/Headers/SP" \
  "$project_dir/tests/plugin_tests.cpp" -o "$project_dir/build/plugin_tests"
"$project_dir/build/plugin_tests" "$project_dir/build/Rounded Corners.plugin/Contents/MacOS/RoundedCorners"
