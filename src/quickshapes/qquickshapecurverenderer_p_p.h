// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QQUICKSHAPECURVERENDERER_P_P_H
#define QQUICKSHAPECURVERENDERER_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuickShapes/private/qquickshapesglobal_p.h>
#include <QtGui/qvector2d.h>
#include <QPainterPath>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcShapeCurveRenderer);

struct QtPathVertex
{
    QVector2D point;
    int id;
    quint32 binX = 0;
    quint32 binY = 0;
};

struct QtPathEdge
{
    quint32 startIndex;
    quint32 endIndex;
    int id;
};

struct QtPathTriangle
{
    QtPathTriangle(quint32 i1, quint32 i2, quint32 i3, int d) : v1Index(i1), v2Index(i2), v3Index(i3), id(d) {}
    quint32 v1Index;
    quint32 v2Index;
    quint32 v3Index;

    quint32 adjacentTriangle1 = quint32(-1); // Adjacent to v1-v2
    quint32 adjacentTriangle2 = quint32(-1); // Adjacent to v2-v3
    quint32 adjacentTriangle3 = quint32(-1); // Adjacent to v3-v1

    // Used by triangulator
    quint32 lastSeenVertex = quint32(-1);

    // Should this triangle be rendered? Set to false for triangles connecting to super-triangle
    bool isValid = true;

    int id;
};

constexpr bool operator==(const QtPathTriangle& lhs, const QtPathTriangle& rhs)
{
    return lhs.id == rhs.id
            && lhs.v1Index == rhs.v1Index
            && lhs.v2Index == rhs.v2Index
            && lhs.v3Index == rhs.v3Index;
}

class QBezier;
Q_QUICKSHAPES_PRIVATE_EXPORT QPolygonF qt_toQuadratics(const QBezier &b, qreal errorLimit = 0.2);
Q_QUICKSHAPES_PRIVATE_EXPORT QList<QtPathTriangle> qtDelaunayTriangulator(const QList<QtPathVertex> &vertices, const QList<QtPathEdge> &edges, const QPainterPath &path);

QT_END_NAMESPACE

#endif //QQUICKSHAPECURVERENDERER_P_P_H
