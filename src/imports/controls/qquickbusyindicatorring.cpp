/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickbusyindicatorring_p.h"

#include <QtCore/qset.h>
#include <QtGui/qpainter.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/qsgsimplerectnode.h>
#include <QtQuick/qquickwindow.h>

QT_BEGIN_NAMESPACE

class QQuickBusyIndicatorAnimatorJob : public QQuickAnimatorJob
{
public:
    QQuickBusyIndicatorAnimatorJob();
    ~QQuickBusyIndicatorAnimatorJob();

    void updateCurrentTime(int time) Q_DECL_OVERRIDE;
    void writeBack() Q_DECL_OVERRIDE;
    void nodeWasDestroyed() Q_DECL_OVERRIDE;
};

static const int circles = 10;
static const int animationDuration = 100 * circles * 2;

QQuickBusyIndicatorRing::QQuickBusyIndicatorRing(QQuickItem *parent) :
    QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents);
    setImplicitWidth(116);
    setImplicitHeight(116);
}

QQuickBusyIndicatorRing::~QQuickBusyIndicatorRing()
{
}

static QPointF moveBy(const QPointF &pos, qreal rotation, qreal distance)
{
    return pos - QTransform().rotate(rotation).map(QPointF(0, distance));
}

QSGNode *QQuickBusyIndicatorRing::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    QQuickItemPrivate *d = QQuickItemPrivate::get(this);

    if (!oldNode)
        oldNode = new QSGSimpleRectNode(boundingRect(), Qt::transparent);
    static_cast<QSGSimpleRectNode *>(oldNode)->setRect(boundingRect());

    QSGTransformNode *rootTransformNode = static_cast<QSGTransformNode *>(oldNode->firstChild());
    if (!rootTransformNode) {
        rootTransformNode = new QSGTransformNode;
        oldNode->appendChildNode(rootTransformNode);
    }
    Q_ASSERT(rootTransformNode->type() == QSGNode::TransformNodeType);

    const qreal w = width();
    const qreal h = height();
    const qreal sz = qMin(w, h);
    const qreal dx = (w - sz) / 2;
    const qreal dy = (h - sz) / 2;
    const int circleRadius = sz / 12;

    QSGTransformNode *transformNode = static_cast<QSGTransformNode *>(rootTransformNode->firstChild());
    for (int i = 0; i < circles; ++i) {
        if (!transformNode) {
            transformNode = new QSGTransformNode;
            rootTransformNode->appendChildNode(transformNode);

            QSGOpacityNode *opacityNode = new QSGOpacityNode;
            transformNode->appendChildNode(opacityNode);

            QSGRectangleNode *rectNode = d->sceneGraphContext()->createRectangleNode();
            rectNode->setAntialiasing(true);
            rectNode->setColor(QColor("#353637"));
            rectNode->setPenColor(QColor("#353637"));
            opacityNode->appendChildNode(rectNode);
        }

        QSGNode *opacityNode = transformNode->firstChild();
        Q_ASSERT(opacityNode->type() == QSGNode::OpacityNodeType);

        QSGRectangleNode *rectNode = static_cast<QSGRectangleNode *>(opacityNode->firstChild());
        Q_ASSERT(rectNode->type() == QSGNode::GeometryNodeType);

        QPointF pos = QPointF(sz / 2 - circleRadius, sz / 2 - circleRadius);
        pos = moveBy(pos, 360 / circles * i, sz / 2 - circleRadius);

        QMatrix4x4 m;
        m.translate(dx + pos.x(), dy + pos.y());
        transformNode->setMatrix(m);

        rectNode->setRect(QRectF(QPointF(), QSizeF(circleRadius * 2, circleRadius * 2)));
        rectNode->setRadius(circleRadius);
        rectNode->update();

        transformNode = static_cast<QSGTransformNode *>(transformNode->nextSibling());
    }

    return oldNode;
}

QQuickBusyIndicatorAnimator::QQuickBusyIndicatorAnimator(QObject *parent) :
    QQuickAnimator(parent)
{
    setDuration(animationDuration);
    setLoops(QQuickAnimator::Infinite);
}

QString QQuickBusyIndicatorAnimator::propertyName() const
{
    return QString();
}

QQuickAnimatorJob *QQuickBusyIndicatorAnimator::createJob() const
{
    return new QQuickBusyIndicatorAnimatorJob;
}

QQuickBusyIndicatorAnimatorJob::QQuickBusyIndicatorAnimatorJob()
{
}

QQuickBusyIndicatorAnimatorJob::~QQuickBusyIndicatorAnimatorJob()
{
}

void QQuickBusyIndicatorAnimatorJob::updateCurrentTime(int time)
{
    if (!m_target)
        return;

    QSGNode *childContainerNode = QQuickItemPrivate::get(m_target)->childContainerNode();
    QSGSimpleRectNode *rootRectNode = static_cast<QSGSimpleRectNode*>(childContainerNode->firstChild());
    if (!rootRectNode)
        return;

    Q_ASSERT(rootRectNode->type() == QSGNode::GeometryNodeType);

    QSGTransformNode *rootTransformNode = static_cast<QSGTransformNode*>(rootRectNode->firstChild());
    Q_ASSERT(rootTransformNode->type() == QSGNode::TransformNodeType);

    const qreal percentageComplete = time / qreal(animationDuration);
    const qreal firstPhaseProgress = percentageComplete <= 0.5 ? percentageComplete * 2 : 0;
    const qreal secondPhaseProgress = percentageComplete > 0.5 ? (percentageComplete - 0.5) * 2 : 0;

    QSGTransformNode *transformNode = static_cast<QSGTransformNode*>(rootTransformNode->firstChild());
    Q_ASSERT(transformNode->type() == QSGNode::TransformNodeType);
    for (int i = 0; i < circles; ++i) {
        QSGOpacityNode *opacityNode = static_cast<QSGOpacityNode*>(transformNode->firstChild());
        Q_ASSERT(opacityNode->type() == QSGNode::OpacityNodeType);

        QSGRectangleNode *rectNode = static_cast<QSGRectangleNode*>(opacityNode->firstChild());
        Q_ASSERT(rectNode->type() == QSGNode::GeometryNodeType);

        const bool fill = (firstPhaseProgress > qreal(i) / circles) || (secondPhaseProgress > 0 && secondPhaseProgress < qreal(i) / circles);
        rectNode->setPenWidth(fill ? 0 : 1);
        rectNode->setColor(fill ? QColor("#353637") : QColor("transparent"));
        rectNode->update();

        transformNode = static_cast<QSGTransformNode*>(transformNode->nextSibling());
    }
}

void QQuickBusyIndicatorAnimatorJob::writeBack()
{
}

void QQuickBusyIndicatorAnimatorJob::nodeWasDestroyed()
{
}

QT_END_NAMESPACE
