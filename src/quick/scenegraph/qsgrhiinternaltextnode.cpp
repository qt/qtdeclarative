// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgrhiinternaltextnode_p.h"

#include <private/qquadpath_p.h>
#include <private/qsgcurvefillnode_p.h>
#include <private/qsgcurvestrokenode_p.h>
#include <private/qsgcurveprocessor_p.h>

QT_BEGIN_NAMESPACE

QSGRhiInternalTextNode::QSGRhiInternalTextNode(QSGRenderContext *renderContext)
    : QSGInternalTextNode(renderContext)
{
}

void QSGRhiInternalTextNode::addDecorationNode(const QRectF &rect, const QColor &color)
{
    QSGCurveStrokeNode *node = new QSGCurveStrokeNode;
    node->setColor(color);
    node->setStrokeWidth(rect.height());

    QQuadPath path;
    QPointF c = rect.center();
    path.moveTo(QVector2D(rect.left(), c.y()));
    path.lineTo(QVector2D(rect.right(), c.y()));

    QSGCurveProcessor::processStroke(path, 2, rect.height(), Qt::MiterJoin, Qt::FlatCap,
                                     [&node](const std::array<QVector2D, 3> &s,
                                             const std::array<QVector2D, 3> &p,
                                             const std::array<QVector2D, 3> &n,
                                             bool isLine) {
                                         Q_ASSERT(isLine);
                                         node->appendTriangle(s, std::array<QVector2D, 2>{p.at(0), p.at(2)}, n);
                                     });
    node->cookGeometry();
    appendChildNode(node);
}

QT_END_NAMESPACE
