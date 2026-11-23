// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTARSHAPE_P_P_H
#define QQUICKSTARSHAPE_P_P_H

#include "qquickstarshape_p.h"
#include <QtQml/private/qqmlpropertyutils_p.h>
#include <QtQuickShapes/private/qquickshape_p_p.h>
#include <qpoint.h>
#include <qvectornd.h>
#include <vector>

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

QT_BEGIN_NAMESPACE

class Q_QUICKSHAPESDESIGNHELPERS_EXPORT QQuickStarShapePrivate : public QQuickShapePrivate
{
    Q_DECLARE_PUBLIC(QQuickStarShape)

public:
    QQuickStarShapePrivate();
    ~QQuickStarShapePrivate() override;

    void updatePath();
    void updatePoints();
    void constructPolygonPath();
    void constructRoundedPolygonPath();

    static constexpr int pointCountMin = 3;
    static constexpr int pointCountMax = 60;

    qreal cornerRadius = 10;
    int pointCount = 6;
    qreal ratio = 0.5;

    std::vector<QVector2D> points;

    QQuickShapePath *path = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKSTARSHAPE_P_P_H
