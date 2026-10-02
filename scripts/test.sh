#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$project_dir/build"
xcrun clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I "$project_dir/src" "$project_dir/tests/mask_tests.cpp" \
  -o "$project_dir/build/mask_tests"
"$project_dir/build/mask_tests"
