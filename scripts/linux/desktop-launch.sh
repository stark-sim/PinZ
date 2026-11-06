#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
log_dir="${XDG_CACHE_HOME:-${HOME}/.cache}/eshot"
mkdir -p "${log_dir}"

if ! bash "${repo_root}/scripts/linux/install-user.sh" "$@" \
  >"${log_dir}/install.log" 2>&1; then
  if command -v kdialog >/dev/null 2>&1; then
    kdialog --error "EShot kurulamadı. Ayrıntılar: ${log_dir}/install.log"
  elif command -v zenity >/dev/null 2>&1; then
    zenity --error --title="EShot" \
      --text="EShot could not be installed. Details: ${log_dir}/install.log"
  fi
  exit 1
fi
