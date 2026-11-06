#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
dist_dir="${repo_root}/dist-linux"
package_dir="${repo_root}/packages"
version="$(sed -n 's/^project(EShot VERSION \([0-9][0-9.]*\).*/\1/p' "${repo_root}/CMakeLists.txt" | head -n1)"
version="${version:-0.0.0}"
tar_path="${package_dir}/PinZ-v${version}-linux-x64.tar.gz"
deb_root="${package_dir}/deb/pinz_${version}_amd64"
deb_path="${package_dir}/pinz_${version}_amd64.deb"

if [[ ! -x "${dist_dir}/bin/EShot" ]]; then
  "${repo_root}/scripts/linux/build-linux.sh"
fi

mkdir -p "${package_dir}"
tar -C "${dist_dir}" -czf "${tar_path}" .

if command -v dpkg-deb >/dev/null 2>&1; then
  rm -rf "${deb_root}"
  install -Dm755 "${dist_dir}/bin/EShot" "${deb_root}/usr/bin/EShot"
  install -Dm755 "${dist_dir}/bin/eshot-launcher" "${deb_root}/usr/bin/eshot-launcher"
  ln -s eshot-launcher "${deb_root}/usr/bin/eshot"
  ln -s eshot-launcher "${deb_root}/usr/bin/pinz"
  install -d "${deb_root}/usr/share/applications"
  sed -e 's/^Name=EShot$/Name=PinZ/' \
    -e 's/^Comment=.*/Comment=Fast screenshot and pinning tool for GNOME on Linux/' \
    -e 's/^Exec=eshot-launcher/Exec=pinz/' \
    -e 's/^Icon=.*/Icon=pinz/' \
    -e 's/^Name=Quit EShot$/Name=Quit PinZ/' \
    "${repo_root}/packaging/linux/io.github.benoks.EShot.desktop" \
    >"${deb_root}/usr/share/applications/io.github.benoks.EShot.desktop"
  chmod 0644 "${deb_root}/usr/share/applications/io.github.benoks.EShot.desktop"
  install -Dm644 "${repo_root}/docs/images/pinZ-icon.svg" \
    "${deb_root}/usr/share/icons/hicolor/scalable/apps/pinz.svg"
  install -Dm644 "${repo_root}/packaging/linux/io.github.benoks.EShot.svg" \
    "${deb_root}/usr/share/icons/hicolor/scalable/apps/io.github.benoks.EShot.svg"
  # Keep the legacy versioned icon for runtime notifications.
  install -Dm644 "${repo_root}/packaging/linux/io.github.benoks.EShot.svg" \
    "${deb_root}/usr/share/icons/hicolor/scalable/apps/io.github.benoks.EShot-v4.svg"
  install -Dm644 "${repo_root}/LICENSE" "${deb_root}/usr/share/doc/pinz/copyright"
  install -Dm644 "${repo_root}/README.md" "${deb_root}/usr/share/doc/pinz/README.md"
  # Mirror the CMake install layout: runtime helper scripts go to lib/eshot.
  install -Dm755 "${repo_root}/scripts/linux/runtime-common.sh" \
    "${deb_root}/usr/lib/eshot/runtime-common.sh"
  install -Dm755 "${repo_root}/scripts/linux/install-runtime-deps.sh" \
    "${deb_root}/usr/lib/eshot/install-runtime-deps.sh"
  install -Dm755 "${repo_root}/scripts/linux/packagekit-install.sh" \
    "${deb_root}/usr/lib/eshot/packagekit-install.sh"

  installed_size="$(du -sk "${deb_root}/usr" | cut -f1)"
  mkdir -p "${deb_root}/DEBIAN"
  cat > "${deb_root}/DEBIAN/control" <<EOF
Package: pinz
Version: ${version}
Section: graphics
Priority: optional
Architecture: amd64
Installed-Size: ${installed_size}
Maintainer: stark-sim <67146986+stark-sim@users.noreply.github.com>
Depends: libc6, libstdc++6, libqt6core6t64 | libqt6core6, libqt6gui6, libqt6widgets6, libqt6network6, libqt6dbus6, libqt6svg6, qt6-qpa-plugins, qt6-wayland, libx11-6, libxcb-cursor0, libsecret-1-0, ffmpeg, tesseract-ocr, gstreamer1.0-tools, gstreamer1.0-pipewire, gstreamer1.0-pulseaudio, gstreamer1.0-plugins-good, gstreamer1.0-plugins-bad, gstreamer1.0-plugins-ugly, gstreamer1.0-libav, xdg-desktop-portal, xdg-desktop-portal-gnome | xdg-desktop-portal-gtk | xdg-desktop-portal-kde | xdg-desktop-portal-wlr
Description: Fast screenshot and pinning tool for GNOME on Linux
 PinZ captures screen regions, annotates and pins images, runs local OCR,
 uploads or saves output, and records GIF or MP4 captures. This transitional
 package keeps the internal EShot executable and application ID while exposing
 the PinZ package name and desktop branding.
EOF
  dpkg-deb --root-owner-group --build "${deb_root}" "${deb_path}"
  dpkg-deb --info "${deb_path}" >/dev/null
  if command -v desktop-file-validate >/dev/null 2>&1; then
    desktop-file-validate "${deb_root}/usr/share/applications/io.github.benoks.EShot.desktop"
  fi
  if command -v appstreamcli >/dev/null 2>&1; then
    appstreamcli validate-tree --no-net "${deb_root}"
  fi
fi

echo
echo "Packaged PinZ Linux:"
echo "  ${tar_path}"
if [[ -f "${deb_path}" ]]; then
  echo "  ${deb_path}"
fi
if compgen -G "${package_dir}/PinZ-v${version}-*.AppImage" >/dev/null; then
  compgen -G "${package_dir}/PinZ-v${version}-*.AppImage" | sed 's/^/  /'
fi
