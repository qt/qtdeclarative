// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGCURVESTROKENODE_P_H
#define QSGCURVESTROKENODE_P_H

#include <QtQuick/private/qtquickexports_p.h>
#include <QtQuick/qsgnode.h>

#include "qsgcurveabstractnode_p.h"
#include "qsgcurvestrokenode_p_p.h"

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

class Q_QUICK_PRIVATE_EXPORT QSGCurveStrokeNode : public QSGCurveAbstractNode
{
public:
    QSGCurveStrokeNode();

    void setColor(QColor col) override
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

    void appendTriangle(const std::array<QVector2D, 3> &v, // triangle vertices
                        const std::array<QVector2D, 3> &p, // curve points
                        const std::array<QVector2D, 3> &n); // vertex normals
    void appendTriangle(const std::array<QVector2D, 3> &v, // triangle vertices
                        const std::array<QVector2D, 2> &p, // line points
                        const std::array<QVector2D, 3> &n); // vertex normals

    void cookGeometry() override;

    static const QSGGeometry::AttributeSet &attributes();

    QVector<quint32> uncookedIndexes() const
    {
        return m_uncookedIndexes;
    }

    float debug() const
    {
        return m_debug;
    }

    void setDebug(float newDebug)
    {
        m_debug = newDebug;
    }

    void setLocalScale(float scale)
    {
        m_localScale = scale;
    }

    float localScale() const
    {
        return m_localScale;
    }

private:

    struct StrokeVertex
    {
        float x, y;
        float ax, ay;
        float bx, by;
        float cx, cy;
        float nx, ny; //normal vector: direction to move vertext to account for AA
    };

    void updateMaterial();

    static std::array<QVector2D, 3> curveABC(const std::array<QVector2D, 3> &p);

    QColor m_color;
    float m_strokeWidth = 0.0f;
    float m_debug = 0.0f;
    float m_localScale = 1.0f;

protected:
    QScopedPointer<QSGCurveStrokeMaterial> m_material;

    QVector<StrokeVertex> m_uncookedVertexes;
    QVector<quint32> m_uncookedIndexes;
};

QT_END_NAMESPACE

#endif // QSGCURVESTROKENODE_P_H
