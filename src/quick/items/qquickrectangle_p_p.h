// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKRECTANGLE_P_P_H
#define QQUICKRECTANGLE_P_P_H

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

#include "qquickitem_p.h"
#include <QtCore/qmetaobject.h>
#include <private/qlazilyallocated_p.h>

QT_BEGIN_NAMESPACE

class QQuickGradient;
class QQuickRectangle;
class QQuickRectanglePrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickRectangle)

public:
    QQuickRectanglePrivate() :
    color(Qt::white), gradient(QJSValue::UndefinedValue), pen(0), radius(0)
    {
    }

    ~QQuickRectanglePrivate()
    {
    }

    QColor color;
    QJSValue gradient;
    QQuickPen *pen;
    qreal radius;

    struct ExtraData {
        ExtraData()
            : topLeftRadius(-1.),
              topRightRadius(-1.),
              bottomLeftRadius(-1.),
              bottomRightRadius(-1.)
        {
        }

        qreal topLeftRadius;
        qreal topRightRadius;
        qreal bottomLeftRadius;
        qreal bottomRightRadius;
    };
    QLazilyAllocated<ExtraData> extraRectangle;

    static int doUpdateSlotIdx;

    void maybeSetImplicitAntialiasing();
};

QT_END_NAMESPACE

#endif // QQUICKRECTANGLE_P_P_H
