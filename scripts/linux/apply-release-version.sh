#!/usr/bin/env bash
set -euo pipefail

tag="${1:-}"
cmake_file="${2:-CMakeLists.txt}"

if [[ ! "${tag}" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  printf 'Release tags must use vMAJOR.MINOR.PATCH, for example v3.0.5\n' >&2
  exit 1
fi

version="${tag#v}"
sed -i -E \
  "s/^project\(EShot VERSION [0-9]+\.[0-9]+\.[0-9]+/project(EShot VERSION ${version}/" \
  "${cmake_file}"
grep -F "project(EShot VERSION ${version}" "${cmake_file}" >/dev/null
printf '%s\n' "${version}"
