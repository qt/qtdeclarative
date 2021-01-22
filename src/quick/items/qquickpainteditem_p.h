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

#ifndef QQUICKPAINTEDITEM_P_P_H
#define QQUICKPAINTEDITEM_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquickpainteditem.h"
#include "qquickitem_p.h"
#include "qquickpainteditem.h"
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class QQuickPaintedItemTextureProvider;
class QSGPainterNode;

class Q_QUICK_PRIVATE_EXPORT QQuickPaintedItemPrivate : public QQuickItemPrivate
{
public:
    QQuickPaintedItemPrivate();

    QSize contentsSize;
    qreal contentsScale;
    QColor fillColor;
    QQuickPaintedItem::RenderTarget renderTarget;
    QQuickPaintedItem::PerformanceHints performanceHints;
    QSize textureSize;

    QRect dirtyRect;

    bool opaquePainting: 1;
    bool antialiasing: 1;
    bool mipmap: 1;

    mutable QQuickPaintedItemTextureProvider *textureProvider;
    QSGPainterNode *node;
};

QT_END_NAMESPACE

#endif // QQUICKPAINTEDITEM_P_P_H
