#pragma once

#include "CaptureGeometry.h"

#include <QRect>
#include <QVector>
#include <QtGlobal>

QRect overlayLocalWindowRect(const QRect &physicalWindowRect,
                             const QVector<CaptureMonitorGeometry> &monitors,
                             const QRect &virtualDesktopRect);

QVector<QRect> windowsForCaptureOverlay(quintptr overlayWindowId,
                                        const QVector<CaptureMonitorGeometry> &monitors,
                                        const QRect &virtualDesktopRect);
