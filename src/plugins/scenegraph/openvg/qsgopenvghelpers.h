// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGOPENVGHELPERS_H
#define QSGOPENVGHELPERS_H

#include <QtGui/QPainterPath>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <VG/openvg.h>

QT_BEGIN_NAMESPACE

namespace QSGOpenVGHelpers {

VGPath qPainterPathToVGPath(const QPainterPath &path);
void qDrawTiled(VGImage image, const QSize imageSize, const QRectF &targetRect, const QPointF offset, float scaleX, float scaleY);
void qDrawBorderImage(VGImage image, const QSizeF &textureSize, const QRectF &targetRect, const QRectF &innerTargetRect, const QRectF &subSourceRect);
void qDrawSubImage(VGImage image, const QRectF &sourceRect, const QPointF &destOffset);
const QVector<VGfloat> qColorToVGColor(const QColor &color, float opacity = 1.0f);
VGImageFormat qImageFormatToVGImageFormat(QImage::Format format);
QImage::Format qVGImageFormatToQImageFormat(VGImageFormat format);

};

QT_END_NAMESPACE

#endif // QSGOPENVGHELPERS_H
