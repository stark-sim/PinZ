#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
installer="${repo_root}/scripts/linux/packagekit-install.sh"

fixture="$(mktemp -d)"
trap 'rm -rf "${fixture}"' EXIT
cat >"${fixture}/gdbus" <<'EOF'
#!/usr/bin/env bash
printf '%s\n' "$*" >>"${ESHOT_GDBUS_LOG}"
if [[ "$1" == introspect ]]; then
  printf 'method InstallPackageNames\n'
fi
EOF
chmod +x "${fixture}/gdbus"

ESHOT_GDBUS_LOG="${fixture}/gdbus.log" PATH="${fixture}:${PATH}" \
  bash "${installer}" ffmpeg tesseract

grep -F 'introspect --session --dest org.freedesktop.PackageKit' "${fixture}/gdbus.log" >/dev/null
grep -F 'org.freedesktop.PackageKit.Modify.InstallPackageNames 0 ['"'"'ffmpeg'"'"','"'"'tesseract'"'"']' \
  "${fixture}/gdbus.log" >/dev/null

set +e
ESHOT_GDBUS_LOG="${fixture}/gdbus.log" PATH="${fixture}:${PATH}" \
  bash "${installer}" 'unsafe package'
status=$?
set -e
[[ "${status}" -eq 1 ]]

cat >"${fixture}/gdbus" <<'EOF'
#!/usr/bin/env bash
exit 1
EOF
chmod +x "${fixture}/gdbus"

set +e
PATH="${fixture}:${PATH}" bash "${installer}" ffmpeg
status=$?
set -e
[[ "${status}" -eq 2 ]]

printf 'packagekit-installer tests passed\n'
