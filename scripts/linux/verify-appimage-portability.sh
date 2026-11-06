#!/usr/bin/env bash
set -euo pipefail

if [[ "$#" -ne 1 ]]; then
  printf 'Usage: %s <AppImage>\n' "${0##*/}" >&2
  exit 2
fi

appimage="$(realpath "$1")"
[[ -x "${appimage}" ]] || {
  printf 'AppImage is missing or not executable: %s\n' "${appimage}" >&2
  exit 1
}

max_glibc="2.35"
work_dir="$(mktemp -d)"
trap 'rm -rf "${work_dir}"' EXIT

(
  cd "${work_dir}"
  "${appimage}" --appimage-extract >/dev/null
)

appdir="${work_dir}/squashfs-root"
binary="${appdir}/usr/bin/EShot"
[[ -x "${binary}" ]] || {
  printf 'EShot binary was not found in AppImage: %s\n' "${binary}" >&2
  exit 1
}

isa_failure=""
while IFS= read -r -d '' elf; do
  needed="$(readelf --notes "${elf}" 2>/dev/null | grep -E 'x86 ISA needed:' || true)"
  if grep -Eq 'x86-64-v[234]' <<<"${needed}"; then
    isa_failure="${elf#"${appdir}/"}: ${needed#*x86 ISA needed: }"
    break
  fi
done < <(find "${appdir}" -type f -print0)

if [[ -n "${isa_failure}" ]]; then
  printf 'AppImage requires a non-baseline x86-64 ISA: %s\n' "${isa_failure}" >&2
  exit 1
fi

glibc_versions="${work_dir}/glibc-versions"
: >"${glibc_versions}"
while IFS= read -r -d '' elf; do
  symbols="$(objdump -T "${elf}" 2>/dev/null || true)"
  grep -oE 'GLIBC_[0-9]+\.[0-9]+' <<<"${symbols}" >>"${glibc_versions}" || true
done < <(find "${appdir}" -type f -print0)

highest_glibc="$(sed 's/^GLIBC_//' "${glibc_versions}" | sort -Vu | tail -n1)"

if [[ -z "${highest_glibc}" ]]; then
  printf 'Could not determine the AppImage glibc requirement.\n' >&2
  exit 1
fi

newest="$(printf '%s\n%s\n' "${max_glibc}" "${highest_glibc}" | sort -Vu | tail -n1)"
if [[ "${newest}" != "${max_glibc}" ]]; then
  printf 'AppImage requires GLIBC_%s; maximum supported is GLIBC_%s.\n' \
    "${highest_glibc}" "${max_glibc}" >&2
  exit 1
fi

printf 'AppImage portability verified: x86-64-baseline, GLIBC_%s or older.\n' \
  "${highest_glibc}"
