#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
sdk_dir="${AE_SDK_PATH:-${1:-}}"
if [[ -z "$sdk_dir" ]]; then
  echo 'Usage: bash scripts/build-mac.sh "/path/to/AfterEffectsSDK"' >&2
  echo 'Download the After Effects C++ SDK from https://developer.adobe.com/after-effects/' >&2
  exit 2
fi
if [[ -f "$sdk_dir/Examples/Headers/AE_Effect.h" ]]; then
  examples_dir="$sdk_dir/Examples"
elif [[ -f "$sdk_dir/Headers/AE_Effect.h" ]]; then
  examples_dir="$sdk_dir"
else
  echo "No Examples/Headers/AE_Effect.h found under: $sdk_dir" >&2
  exit 2
fi
bundle_dir="$project_dir/build/Rounded Corners.plugin"
mkdir -p "$bundle_dir/Contents/MacOS" "$bundle_dir/Contents/Resources"
includes=(-I "$examples_dir/Headers" -I "$examples_dir/Headers/SP" -I "$examples_dir/Util" -I "$examples_dir/Resources")
xcrun clang++ -std=c++17 -O3 -Wall -Wextra -Wno-unused-parameter \
  -arch arm64 -arch x86_64 -mmacosx-version-min=12.0 -bundle \
  "${includes[@]}" -I "$project_dir/src" \
  "$project_dir/src/RoundedCorners.cpp" \
  -o "$bundle_dir/Contents/MacOS/RoundedCorners"
xcrun Rez -useDF -d __MACH__=1 "${includes[@]}" \
  "$project_dir/resources/RoundedCornersPiPL.r" \
  -o "$bundle_dir/Contents/Resources/RoundedCorners.rsrc"
cp "$project_dir/resources/Info.plist" "$bundle_dir/Contents/Info.plist"
/usr/bin/codesign --force --sign - "$bundle_dir"
/usr/bin/codesign --verify --strict "$bundle_dir"
echo "Built: $bundle_dir"
