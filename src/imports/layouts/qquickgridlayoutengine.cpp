/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Layouts module of the Qt Toolkit.
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

Qt::Alignment QQuickGridLayoutEngine::alignment(QQuickItem *quickItem) const
{
    if (QGridLayoutItem *item = findLayoutItem(quickItem))
        return item->alignment();
    return {};
}

QT_END_NAMESPACE
