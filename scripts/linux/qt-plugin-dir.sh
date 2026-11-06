#!/usr/bin/env bash
set -euo pipefail

if command -v qtpaths6 >/dev/null 2>&1; then
  qtpaths6 --plugin-dir
elif command -v qmake6 >/dev/null 2>&1; then
  qmake6 -query QT_INSTALL_PLUGINS
elif command -v qmake >/dev/null 2>&1; then
  qmake -query QT_INSTALL_PLUGINS
else
  printf 'Qt plugin directory could not be queried (qtpaths6/qmake6 missing).\n' >&2
  exit 1
fi
