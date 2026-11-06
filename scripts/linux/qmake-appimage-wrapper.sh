#!/usr/bin/env bash
set -euo pipefail

if [[ "${1:-}" == "-query" && "${2:-}" == "QT_INSTALL_PLUGINS" ]]; then
  printf '%s\n' "${ESHOT_QT_PLUGIN_DIR:?ESHOT_QT_PLUGIN_DIR is not set}"
  exit 0
fi

if [[ "${1:-}" == "-query" && $# -eq 1 ]]; then
  "${ESHOT_REAL_QMAKE:-/usr/bin/qmake6}" -query \
    | sed "s|^QT_INSTALL_PLUGINS:.*$|QT_INSTALL_PLUGINS:${ESHOT_QT_PLUGIN_DIR:?ESHOT_QT_PLUGIN_DIR is not set}|"
  exit 0
fi

exec "${ESHOT_REAL_QMAKE:-/usr/bin/qmake6}" "$@"
