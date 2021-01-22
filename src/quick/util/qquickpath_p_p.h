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

class Q_QUICK_PRIVATE_EXPORT QQuickPathPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickPath)

public:
    static QQuickPathPrivate* get(QQuickPath *path) { return path->d_func(); }
    static const QQuickPathPrivate* get(const QQuickPath *path) { return path->d_func(); }

    QQuickPathPrivate() {}

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
    bool componentComplete = true;
    bool isShapePath = false;
};

QT_END_NAMESPACE

#endif
