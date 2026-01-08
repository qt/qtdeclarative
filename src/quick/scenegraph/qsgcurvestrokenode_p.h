// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGCURVESTROKENODE_P_H
#define QSGCURVESTROKENODE_P_H

#include <QtQuick/qtquickexports.h>
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

class Q_QUICK_EXPORT QSGCurveStrokeNode : public QSGCurveAbstractNode
{
public:
    QSGCurveStrokeNode();

    void setColor(QColor col) override
    {
        m_color = col;
        markDirty(DirtyMaterial);
    }

    QColor color() const
    {
        return m_color;
    }

    void setCosmeticStroke(bool c)
    {
        m_cosmetic = c;
        markDirty(DirtyMaterial);
    }

    void setStrokeWidth(float width)
    {
        m_strokeWidth = width;
        markDirty(DirtyMaterial);
    }

    float strokeWidth() const
    {
        // Negative stroke width would not normally mean anything;
        // here we use it to mean that the stroke is cosmetic.
        return (m_cosmetic ? -1.0 : 1.0) * qAbs(m_strokeWidth);
    }

    enum class TriangleFlag {
        None = 0,
        Line = 1 << 0,
    };
    Q_DECLARE_FLAGS(TriangleFlags, TriangleFlag)

    static constexpr std::array<float, 3> defaultExtrusions() { return {1.0f, 1.0f, 1.0f}; }

    void appendTriangle(const std::array<QVector2D, 3> &vtx,    // triangle vertices
                        const std::array<QVector2D, 3> &ctl,    // curve points
                        const std::array<QVector2D, 3> &normal, // vertex normals
                        const std::array<float, 3> &extrusions = defaultExtrusions());
    void appendTriangle(const std::array<QVector2D, 3> &vtx,    // triangle vertices
                        const std::array<QVector2D, 2> &ctl,    // line points
                        const std::array<QVector2D, 3> &normal, // vertex normals
                        const std::array<float, 3> &extrusions = defaultExtrusions());

    void cookGeometry() override;

    static const QSGGeometry::AttributeSet &attributes();

    static bool expandingStrokeEnabled();

    QList<quint32> uncookedIndexes() const
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
        float nx, ny;       // normal vector: direction to move vertex to account for AA
        float extrusion;    // stroke width multiplier (* uniform strokeWidth)
    };

    void updateMaterial();

    static std::array<QVector2D, 3> curveABC(const std::array<QVector2D, 3> &p);

    static const bool envStrokeExpanding;
    QColor m_color;
    ushort m_cosmetic = false; // packs alongside QColor; could be turned into flags if needed
    float m_strokeWidth = 0.0f;
    float m_debug = 0.0f;
    float m_localScale = 1.0f;

protected:
    QScopedPointer<QSGCurveStrokeMaterial> m_material;

    QList<StrokeVertex> m_uncookedVertexes;
    QList<quint32> m_uncookedIndexes;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGCurveStrokeNode::TriangleFlags)

QT_END_NAMESPACE

#endif // QSGCURVESTROKENODE_P_H
