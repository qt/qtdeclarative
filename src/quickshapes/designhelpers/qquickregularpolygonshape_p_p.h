// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKREGULARPOLYGONSHAPE_P_P_H
#define QQUICKREGULARPOLYGONSHAPE_P_P_H

#include "qquickregularpolygonshape_p.h"
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

class Q_QUICKSHAPESDESIGNHELPERS_EXPORT QQuickRegularPolygonShapePrivate : public QQuickShapePrivate
{
    Q_DECLARE_PUBLIC(QQuickRegularPolygonShape)

public:
    QQuickRegularPolygonShapePrivate();
    ~QQuickRegularPolygonShapePrivate() override;

    void updatePath();
    void updatePoints();
    void updateBisectors();
    void constructPolygonPath();
    void constructRoundedPolygonPath();

    static constexpr int sideCountMin = 3;
    static constexpr int sideCountMax = 100;

    qreal cornerRadius = 10;
    int sideCount = 6;

    std::vector<QVector2D> points;
    std::vector<QVector2D> bisectors;

    QQuickShapePath *path = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKREGULARPOLYGONSHAPE_P_P_H
