#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
prefix="${ESHOT_USER_PREFIX:-${HOME}/.local/opt/EShot}"
data_home="${XDG_DATA_HOME:-${HOME}/.local/share}"
desktop_dir="${data_home}/applications"
icon_dir="${data_home}/icons/hicolor/scalable/apps"
pixmap_dir="${data_home}/pixmaps"
desktop_file="${desktop_dir}/io.github.benoks.EShot.desktop"
log_dir="${XDG_CACHE_HOME:-${HOME}/.cache}/eshot"

"${repo_root}/scripts/linux/build-linux.sh"

rm -rf "${prefix}.new"
mkdir -p "${prefix}.new" "${desktop_dir}" "${icon_dir}" "${pixmap_dir}" "${log_dir}"
cp -a "${repo_root}/dist-linux/." "${prefix}.new/"
rm -rf "${prefix}"
mv "${prefix}.new" "${prefix}"

escaped_launcher="${prefix}/bin/eshot-launcher"
escaped_launcher="${escaped_launcher//\\/\\\\}"
escaped_launcher="${escaped_launcher//\"/\\\"}"
awk -v executable="${escaped_launcher}" '
  /^Exec=eshot-launcher/ {
    args = substr($0, length("Exec=eshot-launcher") + 1)
    print "Exec=\"" executable "\"" args
    next
  }
  { print }
' "${repo_root}/packaging/linux/io.github.benoks.EShot.desktop" >"${desktop_file}.new"
mv -f "${desktop_file}.new" "${desktop_file}"
chmod 0644 "${desktop_file}"
rm -f "${icon_dir}/io.github.benoks.EShot.png" \
  "${icon_dir}/io.github.benoks.EShot-v4.png" \
  "${pixmap_dir}/io.github.benoks.EShot.png" \
  "${pixmap_dir}/io.github.benoks.EShot-v4.png"
install -Dm644 "${repo_root}/packaging/linux/io.github.benoks.EShot.svg" \
  "${icon_dir}/io.github.benoks.EShot.svg"
install -Dm644 "${repo_root}/packaging/linux/io.github.benoks.EShot.svg" \
  "${icon_dir}/io.github.benoks.EShot-v4.svg"
install -Dm644 "${repo_root}/packaging/linux/io.github.benoks.EShot.svg" \
  "${pixmap_dir}/io.github.benoks.EShot.svg"
install -Dm644 "${repo_root}/packaging/linux/io.github.benoks.EShot.svg" \
  "${pixmap_dir}/io.github.benoks.EShot-v4.svg"

if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database "${desktop_dir}" >/dev/null 2>&1 || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  gtk-update-icon-cache -f -t "${data_home}/icons/hicolor" >/dev/null 2>&1 || true
fi
if command -v kbuildsycoca6 >/dev/null 2>&1; then
  kbuildsycoca6 --noincremental >/dev/null 2>&1 || true
fi

if [[ "${1:-}" != "--install-only" ]]; then
  nohup "${prefix}/bin/eshot-launcher" \
    >"${log_dir}/eshot.log" 2>&1 </dev/null &
fi

printf 'EShot installed for this user: %s\n' "${prefix}"
