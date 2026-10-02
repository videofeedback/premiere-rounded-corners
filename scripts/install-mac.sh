#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
source_bundle="$project_dir/build/Rounded Corners.plugin"
plugin_dir="$HOME/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore"
destination="$plugin_dir/Rounded Corners.plugin"
if [[ ! -f "$source_bundle/Contents/MacOS/RoundedCorners" ]]; then
  echo 'Build the plugin first with scripts/build-mac.sh.' >&2
  exit 1
fi
/usr/bin/codesign --verify --strict "$source_bundle"
# Keep earlier builds outside Adobe's scanned plugin folder.
if [[ -e "$destination" ]]; then
  backup_dir="$project_dir/build/install-backups/$(date +%Y%m%d-%H%M%S)"
  mkdir -p "$backup_dir"
  mv "$destination" "$backup_dir/"
fi
mkdir -p "$plugin_dir"
/usr/bin/ditto "$source_bundle" "$destination"
/usr/bin/codesign --verify --strict "$destination"
echo "Installed: $destination"
echo 'Save your project and restart Premiere to load Rounded Corners.'
