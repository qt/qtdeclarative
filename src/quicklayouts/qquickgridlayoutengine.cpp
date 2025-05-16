// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qquickitem.h"
#include "qquickgridlayoutengine_p.h"
#include "qquicklayout_p.h"

QT_BEGIN_NAMESPACE

void QQuickGridLayoutEngine::setAlignment(QQuickItem *quickItem, Qt::Alignment alignment)
{
    if (QQuickGridLayoutItem *item = findLayoutItem(quickItem)) {
        item->setAlignment(alignment);
        invalidate();
    }
}

void QQuickGridLayoutEngine::setStretchFactor(QQuickItem *quickItem, int stretch,
                                              Qt::Orientation orientation)
{
    Q_ASSERT(stretch >= -1);    // -1 is reset
    if (QGridLayoutItem *item = findLayoutItem(quickItem)) {
        item->setStretchFactor(stretch, orientation);
        invalidate();
    }
}

QT_END_NAMESPACE
