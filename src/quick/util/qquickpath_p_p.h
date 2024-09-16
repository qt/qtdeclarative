// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKPATH_P_H
#define QQUICKPATH_P_H

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

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_path);

#include "qquickpath_p.h"

#include <qqml.h>
#include <QtCore/QStringList>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickPathPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickPath)

public:
    static QQuickPathPrivate* get(QQuickPath *path) { return path->d_func(); }
    static const QQuickPathPrivate* get(const QQuickPath *path) { return path->d_func(); }

    QQuickPathPrivate()
        : componentComplete(true), isShapePath(false), simplify(false)
          , processPending(false), asynchronous(false), useCustomPath(false)
    {}

    QPainterPath _path;
    QList<QQuickPathElement*> _pathElements;
    mutable QVector<QPointF> _pointCache;
    QList<QQuickPath::AttributePoint> _attributePoints;
    QStringList _attributes;
    QList<QQuickCurve*> _pathCurves;
    QList<QQuickPathText*> _pathTexts;
    mutable QQuickCachedBezier prevBez;
    QQmlNullableValue<qreal> startX;
    QQmlNullableValue<qreal> startY;
    qreal pathLength = 0;
    QSizeF scale = QSizeF(1, 1);
    bool closed = false;
    bool componentComplete : 1;
    bool isShapePath : 1;
    bool simplify : 1;
    bool processPending : 1;
    bool asynchronous : 1;
    bool useCustomPath : 1;
};

QT_END_NAMESPACE

#endif
