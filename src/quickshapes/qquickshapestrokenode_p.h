// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSHAPESTROKENODE_P_H
#define QQUICKSHAPESTROKENODE_P_H

#include <QtQuick/qsgnode.h>

#include "qquickshapegenericrenderer_p.h"
#include "qquickshapestrokenode_p_p.h"

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

QT_BEGIN_NAMESPACE

class QQuickShapeStrokeNode : public QSGGeometryNode
{
public:
    QQuickShapeStrokeNode();

    void setColor(QColor col)
    {
        m_color = col;
    }

    QColor color() const
    {
        return m_color;
    }

    void setStrokeWidth(float width)
    {
        m_strokeWidth = width;
    }

    float strokeWidth() const
    {
        return m_strokeWidth;
    }

    void appendTriangle(const QVector2D &v0, const QVector2D &v1, const QVector2D &v2, // triangle vertices
                        const QVector2D &p0, const QVector2D &p1, const QVector2D &p2); // curve points

    void appendTriangle(const QVector2D &v0, const QVector2D &v1, const QVector2D &v2, // triangle vertices
                        const QVector2D &p0, const QVector2D &p1); // line points

    void cookGeometry();

    static const QSGGeometry::AttributeSet &attributes();

    QVector<quint32> uncookedIndexes() const
    {
        return m_uncookedIndexes;
    }

private:

    struct StrokeVertex
    {
        float x, y;
        float ax, ay;
        float bx, by;
        float cx, cy;
        float H, G; //depressed cubic parameters
        float offset; //mapping between depressed and happy cubic
    };

    void updateMaterial();

    static QVector3D HGforPoint(QVector2D A, QVector2D B, QVector2D C, QVector2D p);
    static std::array<QVector2D, 3> curveABC(QVector2D p0, QVector2D p1, QVector2D p2);

    QColor m_color;
    float m_strokeWidth = 0.0f;

protected:
    QScopedPointer<QQuickShapeStrokeMaterial> m_material;

    QVector<StrokeVertex> m_uncookedVertexes;
    QVector<quint32> m_uncookedIndexes;
};

QT_END_NAMESPACE

#endif // QQUICKSHAPESTROKENODE_P_H
