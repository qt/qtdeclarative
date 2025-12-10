// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKELLIPSESHAPE_P_P_H
#define QQUICKELLIPSESHAPE_P_P_H

#include "qquickellipseshape_p.h"
#include <QtQml/private/qqmlpropertyutils_p.h>
#include <QtQuickShapes/private/qquickshape_p_p.h>

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

class Q_QUICKSHAPESDESIGNHELPERS_EXPORT QQuickEllipseShapePrivate : public QQuickShapePrivate
{
    Q_DECLARE_PUBLIC(QQuickEllipseShape)

public:
    QQuickEllipseShapePrivate();
    ~QQuickEllipseShapePrivate() override;

    struct RoundedCorner
    {
        void update(qreal diff, QVector2D a, QVector2D b, QVector2D c, qreal alpha, qreal radius)
        {
            A = a;
            B = b;
            C = c;
            this->alpha = alpha;
            this->radius = radius;
            this->diff = diff;
        }

        void reset()
        {
            A = QVector2D();
            B = QVector2D();
            C = QVector2D();
            alpha = 0;
            radius = 0;
            diff = 0;
        }

        // A - a point, where the rounding corner is located
        QVector2D A;
        // B - tangent point of the rounded corner arc, which is located on the ellipse's arc
        // for the rounded corner at the ellipse's center this is a point on the edge defined by the
        // end angle
        // for the rounded corner at the begin angle this is a point on the outer arc of the ellipse
        QVector2D B;
        // C - tangent point of the rounded corner arc, which is located on the ellipse's edge
        // for the rounded corner at the ellipse's center this is a point on the edge defined by the
        // begin angle
        QVector2D C;
        qreal alpha = 0; // angle between AB and AC
        qreal radius = 0; // rounded corner radius
        // not a rounded corner data, but a helper used to compare
        // currently calculated radius to the previously calculated radius
        qreal diff = 0;
    };

    enum class RoundedCornerIndex { Center, InnerEnd, OuterEnd, InnerBegin, OuterBegin };

    // helper, to avoid typing static_cast<int>(RoundedCornerIndex) every time
    class RoundedCornerArray
    {
    public:
        RoundedCorner &operator[](RoundedCornerIndex index)
        {
            return array[static_cast<int>(index)];
        }

        void reset()
        {
            for (auto &rc : array)
                rc.reset();
        }

    private:
        RoundedCorner array[5];
    } roundedCorners;

    void addLine(QVector2D point);
    void addArc(QVector2D point, QVector2D arcRadius, QQuickPathArc::ArcDirection dir,
                bool largeArc = false);

    qreal getBorderOffset() const;

    // calculates rounded corner at the ellipse center
    void roundCenter(QVector2D center, QVector2D ellipseRadius);
    // runs loop where begin and end rounded corners are calculated
    void roundBeginEnd(QVector2D center, QVector2D ellipseRadius);
    // calculates rounded corners on the outer arc
    bool roundOuter(QVector2D center, QVector2D ellipseRadius, qreal deg, qreal arcAngle1,
                    qreal arcAngle2, RoundedCornerIndex index);
    // calculates rounded corners on the inner arc
    bool roundInner(QVector2D center, QVector2D ellipseRadius, qreal deg, qreal arcAngle1,
                    qreal arcAngle2, RoundedCornerIndex index);

    // starts path at the center rounded corner and draws center rounded corner
    void drawCenterCorner();
    // connects outer and inner arcs with line and draws end rounded corner
    void drawInnerEndCorner();
    // starts path at the begin rounded corner and draws begin rounded corner
    void drawInnerBeginCorner();
    // connects previous rounded corner (center or begin) with line and draws begin rounded corner
    void drawOuterBeginCorner();

    // draw outer arc path from begin rounded corner to the end rounded corner
    void drawOuterArcRounded(QVector2D center, QVector2D ellipseRadius);
    // draw inner arc path from end rounded corner to the begin rounded corner
    void drawInnerArcRounded(QVector2D center, QVector2D ellipseRadius);

    // draws outer arc when no rounded corners involved
    void drawOuterArc(QVector2D center, QVector2D ellipseRadius);
    // draws full inner arc (no rounded corners)
    void drawFullInnerArc(QVector2D center, QVector2D ellipseRadius);

    // draws an ellipse when inner radius is not zero
    void drawWithInnerRadius(QVector2D center, QVector2D ellipseRadius);
    // draws an ellipse when inner radius is greater than zero
    void drawWithoutInnerRadius(QVector2D center, QVector2D ellipseRadius);

    void updatePath();

    QQuickShapePath *path = nullptr;

    qreal startAngle = 0;
    qreal sweepAngle = 360;
    qreal innerArcRatio = 0;
    qreal cornerRadius = 10;
    QQuickEllipseShape::BorderMode borderMode = QQuickEllipseShape::BorderMode::Inside;
};

QT_END_NAMESPACE

#endif // QQUICKELLIPSE1SHAPE_P_P_H
