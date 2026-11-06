#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
root_dir="${repo_root}/linux-root"

"${repo_root}/scripts/linux/check-linux-runtime.sh"
"${repo_root}/scripts/linux/build-linux.sh"

desktop-file-validate "${repo_root}/packaging/linux/io.github.benoks.EShot.desktop"
appstreamcli validate --no-net "${repo_root}/packaging/linux/io.github.benoks.EShot.metainfo.xml"

rm -rf "${root_dir}"
DESTDIR="${root_dir}" cmake --install "${repo_root}/build-linux" --prefix /usr
desktop-file-validate "${root_dir}/usr/share/applications/io.github.benoks.EShot.desktop"
appstreamcli validate-tree --no-net "${root_dir}"

QT_QPA_PLATFORM=offscreen "${repo_root}/dist-linux/bin/EShot" --test-gif
"${repo_root}/scripts/linux/package-linux.sh"
"${repo_root}/scripts/linux/build-appimage.sh"

appimage="${repo_root}/packages/EShot-v$(sed -n 's/^project(EShot VERSION \([0-9][0-9.]*\).*/\1/p' "${repo_root}/CMakeLists.txt" | head -n1)-x86_64.AppImage"
HOME="${repo_root}/appimage-test-home" ESHOT_SKIP_RUNTIME_SETUP=1 \
  "${appimage}" --appimage-extract-and-run --test-gif

echo
echo "Linux verification passed:"
echo "  ${repo_root}/dist-linux/bin/EShot"
echo "  ${repo_root}/packages"
