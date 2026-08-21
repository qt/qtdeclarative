// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsgrhiinternaltextnode_p.h"

#include <private/qquadpath_p.h>
#include <private/qsgcurvefillnode_p.h>
#include <private/qsgcurvestrokenode_p.h>
#include <private/qsgcurveprocessor_p.h>

#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

QSGRhiInternalTextNode::QSGRhiInternalTextNode(QSGRenderContext *renderContext)
    : QSGInternalTextNode(renderContext)
{
}

// Dash pattern arrays {dash, gap, ...} in units of pen width.
static constexpr qreal dashPattern[] = { 4, 2 };
static constexpr qreal dotPattern[] = { 1, 2 };
static constexpr qreal dashDotPattern[] = { 4, 2, 1, 2 };
static constexpr qreal dashDotDotPattern[] = { 4, 2, 1, 2, 1, 2 };

// Returns the dash pattern for a given underline style.
static QSpan<const qreal> dashPatternForStyle(QTextCharFormat::UnderlineStyle style)
{
    switch (style) {
    case QTextCharFormat::DashUnderline:
        return dashPattern;
    case QTextCharFormat::DotLine:
        return dotPattern;
    case QTextCharFormat::DashDotLine:
        return dashDotPattern;
    case QTextCharFormat::DashDotDotLine:
        return dashDotDotPattern;
    default:
        return {};
    }
}

// Builds a QQuadPath of dashed line segments along y = cy, from x0 to x1.
static QQuadPath buildDashedPath(qreal x0, qreal x1, qreal cy, QSpan<const qreal> pattern,
                                 qreal penWidth)
{
    Q_ASSERT(!pattern.isEmpty());

    QQuadPath path;
    qreal x = x0;
    int idx = 0;
    bool drawing = true; // pattern starts with a dash (drawn segment)

    while (x < x1) {
        // Round segment length to nearest pixel to avoid uneven dot sizes
        const qreal rawLen = pattern[idx % pattern.size()] * penWidth;
        const qreal segLen = qMax(qreal(1), qreal(qRound(rawLen)));
        const qreal segEnd = qMin(x + segLen, x1);

        if (drawing && segEnd > x) {
            path.moveTo(QVector2D(qRound(x), cy));
            path.lineTo(QVector2D(qRound(segEnd), cy));
        }

        x += segLen;
        drawing = !drawing;
        ++idx;
    }

    return path;
}

static constexpr qreal goldenRatio = 1.6180339887498949;

// Builds a QQuadPath of a wavy line along y = cy, from x0 to x1.
// Uses quadratic Bezier curves, adapted from generateWavyPixmap() in QPainter.
static QQuadPath buildWavyPath(qreal x0, qreal x1, qreal cy, qreal penWidth)
{
    const qreal radius = qMax(qreal(1), penWidth);
    const qreal halfPeriod = qMax(qreal(2), radius * goldenRatio);

    QQuadPath path;
    path.moveTo(QVector2D(x0, cy));

    qreal x = x0;
    qreal dir = 1.0; // alternates between +1 and -1

    while (x < x1) {
        const qreal nextX = qMin(x + halfPeriod, x1);
        const qreal fraction = (nextX - x) / halfPeriod;
        const qreal peakY = cy + dir * radius * fraction;
        const qreal ctrlX = (x + nextX) / 2.0;

        path.quadTo(QVector2D(ctrlX, peakY), QVector2D(nextX, cy));

        x = nextX;
        dir = -dir;
    }

    return path;
}

// Helper: creates a QSGCurveStrokeNode from a QQuadPath and appends it.
static void strokePathToNode(QSGNode *parent, const QQuadPath &path, const QColor &color,
                             qreal penWidth)
{
    QSGCurveStrokeNode *node = new QSGCurveStrokeNode;
    node->setColor(color);
    node->setStrokeWidth(penWidth);

    QSGCurveProcessor::processStroke(
            path, 2, penWidth, false, Qt::MiterJoin, Qt::FlatCap,
            [&node](const std::array<QVector2D, 3> &s, const std::array<QVector2D, 3> &p,
                    const std::array<QVector2D, 3> &n, const std::array<float, 3> &ex,
                    QSGCurveStrokeNode::TriangleFlags /*f*/) {
                node->appendTriangle(s, std::array<QVector2D, 2>{ p.at(0), p.at(2) }, n, ex);
            });
    node->cookGeometry();
    parent->appendChildNode(node);
}

void QSGRhiInternalTextNode::addDecorationNode(const QRectF &rect,
                                               const QColor &color,
                                               QTextCharFormat::UnderlineStyle style,
                                               RecycleBin *recycleBin)
{
    if (rect.width() <= 0)
        return;

    if (recycleBin != nullptr)
        recycleBin->stopReusing();

    const qreal penWidth = rect.height();
    const QPointF c = rect.center();

    if (style == QTextCharFormat::WaveUnderline) {
        QQuadPath path = buildWavyPath(rect.left(), rect.right(), c.y(), penWidth);
        // Limit pen width so the wave doesn't get too fat
        const qreal radius = qMax(qreal(1), penWidth);
        const qreal maxPenWidth = 0.8 * radius;
        strokePathToNode(this, path, color, qMin(penWidth, maxPenWidth));
        return;
    }

    if (style != QTextCharFormat::SingleUnderline) {
        // Dashed styles
        QSpan<const qreal> pattern = dashPatternForStyle(style);
        if (!pattern.isEmpty()) {
            QQuadPath path = buildDashedPath(rect.left(), rect.right(), c.y(), pattern, penWidth);
            strokePathToNode(this, path, color, penWidth);
            return;
        }
    }

    // SingleUnderline: solid line
    QQuadPath path;
    path.moveTo(QVector2D(rect.left(), c.y()));
    path.lineTo(QVector2D(rect.right(), c.y()));
    strokePathToNode(this, path, color, penWidth);
}

QT_END_NAMESPACE
