#pragma once

#include <QRect>
#include <QSize>
#include <QVector>

struct CaptureMonitorGeometry
{
    QRect logical;
    QRect physical;
    qreal scale = 1.0;
};

QRect physicalRectFromLogical(const QRect &logicalRect, qreal scale);
QRect snapshotRectFromLogical(const QRect &logicalRect,
                              const QSize &logicalCanvasSize,
                              const QSize &snapshotSize,
                              qreal fallbackScale = 1.0);
QRect portalCropRect(const QRect &screenPhysicalRect, const QRect &virtualPhysicalRect);

QRect displayRectFromPhysical(const QRect &physicalRect,
                              const QVector<CaptureMonitorGeometry> &monitors);
