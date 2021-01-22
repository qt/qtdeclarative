/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
