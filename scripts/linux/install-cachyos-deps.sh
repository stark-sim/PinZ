#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${script_dir}/runtime-common.sh"
read -r -a runtime_packages <<<"$(eshot_runtime_packages pacman)"

sudo pacman -S --needed \
  base-devel \
  cmake \
  ninja \
  curl \
  qt6-base \
  qt6-wayland \
  libx11 \
  libxkbcommon \
  desktop-file-utils \
  appstream \
  "${runtime_packages[@]}"
