// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickuniversalprogressbar_p.h"

#include <QtCore/qmath.h>
#include <QtCore/qeasingcurve.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qsgadaptationlayer_p.h>
#include <QtQuick/qsgrectanglenode.h>
#include <QtQuickControls2Impl/private/qquickanimatednode_p.h>

QT_BEGIN_NAMESPACE

static const int QupbPhaseCount = 4;
static const int EllipseCount = 5;
static const int QupbInterval = 167;
static const int QupbTotalDuration = 3917;
static const int VisibleDuration = 3000;
static const qreal EllipseDiameter = 4;
static const qreal EllipseOffset = 4;
static const qreal ContainerAnimationStartPosition = -34; // absolute
static const qreal ContainerAnimationEndPosition = 0.435222; // relative
static const qreal EllipseAnimationWellPosition = 0.333333333333333; // relative
static const qreal EllipseAnimationEndPosition = 0.666666666666667; // relative

class QQuickUniversalProgressBarNode : public QQuickAnimatedNode
{
public:
    QQuickUniversalProgressBarNode(QQuickUniversalProgressBar *item);

    void updateCurrentTime(int time) override;
    void sync(QQuickItem *item) override;

private:
    struct Phase {
        Phase() = default;
        Phase(int d, qreal f, qreal t) : duration(d), from(f), to(t) { }
        int duration = 0;
        qreal from = 0;
        qreal to = 0;
    };

    bool m_indeterminate = false;
    Phase m_borderPhases[QupbPhaseCount];
    Phase m_ellipsePhases[QupbPhaseCount];
};

QQuickUniversalProgressBarNode::QQuickUniversalProgressBarNode(QQuickUniversalProgressBar *item)
    : QQuickAnimatedNode(item)
{
    setLoopCount(Infinite);
    setDuration(QupbTotalDuration);

    m_borderPhases[0] = Phase( 500, -50,   0);
    m_borderPhases[1] = Phase(1500,   0,   0);
    m_borderPhases[2] = Phase(1000,   0, 100);
    m_borderPhases[3] = Phase( 917, 100, 100);

    m_ellipsePhases[0] = Phase(1000,                            0, EllipseAnimationWellPosition);
    m_ellipsePhases[1] = Phase(1000, EllipseAnimationWellPosition, EllipseAnimationWellPosition);
    m_ellipsePhases[2] = Phase(1000, EllipseAnimationWellPosition, EllipseAnimationEndPosition);
    m_ellipsePhases[3] = Phase(1000, EllipseAnimationWellPosition, EllipseAnimationEndPosition);
}

void QQuickUniversalProgressBarNode::updateCurrentTime(int time)
{
    QSGRectangleNode *geometryNode = static_cast<QSGRectangleNode *>(firstChild());
    Q_ASSERT(!geometryNode || geometryNode->type() == QSGNode::GeometryNodeType);
    if (!geometryNode)
        return;

    QSGTransformNode *gridNode = static_cast<QSGTransformNode *>(geometryNode->firstChild());
    Q_ASSERT(!gridNode || gridNode->type() == QSGNode::TransformNodeType);
    if (!gridNode)
        return;

    qreal width = geometryNode->rect().width();
    {
        qreal from = ContainerAnimationStartPosition;
        qreal to = from + ContainerAnimationEndPosition * width;
        qreal progress = static_cast<qreal>(time) / QupbTotalDuration;
        qreal dx = from + (to - from) * progress;

        QMatrix4x4 matrix;
        matrix.translate(dx, 0);
        gridNode->setMatrix(matrix);
    }

    int nodeIndex = 0;
    QSGTransformNode *borderNode = static_cast<QSGTransformNode *>(gridNode->firstChild());
    while (borderNode) {
        Q_ASSERT(borderNode->type() == QSGNode::TransformNodeType);

        QSGTransformNode *ellipseNode = static_cast<QSGTransformNode *>(borderNode->firstChild());
        Q_ASSERT(ellipseNode->type() == QSGNode::TransformNodeType);

        QSGOpacityNode *opacityNode = static_cast<QSGOpacityNode *>(ellipseNode->firstChild());
        Q_ASSERT(opacityNode->type() == QSGNode::OpacityNodeType);

        int begin = nodeIndex * QupbInterval;
        int end = VisibleDuration + nodeIndex * QupbInterval;

        bool visible = time >= begin && time <= end;
        opacityNode->setOpacity(visible ? 1.0 : 0.0);

        if (visible) {
            {
                int phaseIndex, remain = time, elapsed = 0;
                for (phaseIndex = 0; phaseIndex < QupbPhaseCount - 1; ++phaseIndex) {
                    if (remain <= m_borderPhases[phaseIndex].duration + begin)
                        break;
                    remain -= m_borderPhases[phaseIndex].duration;
                    elapsed += m_borderPhases[phaseIndex].duration;
                }

                const Phase &phase = m_borderPhases[phaseIndex];

                qreal pos = time - elapsed - begin;
                qreal progress = pos / phase.duration;
                qreal dx = phase.from + (phase.to - phase.from) * progress;

                QMatrix4x4 matrix;
                matrix.translate(dx, 0);
                borderNode->setMatrix(matrix);
            }

            {
                QEasingCurve curve(QEasingCurve::BezierSpline);
                curve.addCubicBezierSegment(QPointF(0.4, 0.0), QPointF(0.6, 1.0), QPointF(1.0, 1.0));

                int phaseIndex, remain = time, elapsed = 0;
                for (phaseIndex = 0; phaseIndex < QupbPhaseCount - 1; ++phaseIndex) {
                    if (remain <= m_ellipsePhases[phaseIndex].duration + begin)
                        break;
                    remain -= m_ellipsePhases[phaseIndex].duration;
                    elapsed += m_ellipsePhases[phaseIndex].duration;
                }

                const Phase &phase = m_ellipsePhases[phaseIndex];

                qreal from = phase.from * width;
                qreal to = phase.to * width;
                qreal pos = time - elapsed - begin;
                qreal progress = curve.valueForProgress(pos / phase.duration);
                qreal dx = from + (to - from) * progress;

                QMatrix4x4 matrix;
                matrix.translate(dx, 0);
                ellipseNode->setMatrix(matrix);
            }
        }

        borderNode = static_cast<QSGTransformNode *>(borderNode->nextSibling());
        ++nodeIndex;
    }
}

void QQuickUniversalProgressBarNode::sync(QQuickItem *item)
{
    QQuickUniversalProgressBar *bar = static_cast<QQuickUniversalProgressBar *>(item);
    if (m_indeterminate != bar->isIndeterminate()) {
        m_indeterminate = bar->isIndeterminate();
        if (m_indeterminate)
            start();
        else
            stop();
    }

    QQuickItemPrivate *d = QQuickItemPrivate::get(item);

    QRectF bounds = item->boundingRect();
    bounds.setHeight(item->implicitHeight());
    bounds.moveTop((item->height() - bounds.height()) / 2.0);
    if (!m_indeterminate)
        bounds.setWidth(bar->progress() * bounds.width());

    QSGRectangleNode *geometryNode = static_cast<QSGRectangleNode *>(firstChild());
    if (!geometryNode) {
        geometryNode = item->window()->createRectangleNode();
        appendChildNode(geometryNode);
    }
    geometryNode->setRect(bounds);
    geometryNode->setColor(m_indeterminate ? Qt::transparent : bar->color());

    if (!m_indeterminate) {
        while (QSGNode *node = geometryNode->firstChild())
            delete node;
        return;
    }

    QSGTransformNode *gridNode = static_cast<QSGTransformNode *>(geometryNode->firstChild());
    if (!gridNode) {
        gridNode = new QSGTransformNode;
        geometryNode->appendChildNode(gridNode);
    }
    Q_ASSERT(gridNode->type() == QSGNode::TransformNodeType);

    QSGNode *borderNode = gridNode->firstChild();
    for (int i = 0; i < EllipseCount; ++i) {
        if (!borderNode) {
            borderNode = new QSGTransformNode;
            gridNode->appendChildNode(borderNode);

            QSGTransformNode *ellipseNode = new QSGTransformNode;
            borderNode->appendChildNode(ellipseNode);

            QSGOpacityNode *opacityNode = new QSGOpacityNode;
            ellipseNode->appendChildNode(opacityNode);

            QSGInternalRectangleNode *rectNode = d->sceneGraphContext()->createInternalRectangleNode();
            rectNode->setAntialiasing(true);
            rectNode->setRadius(EllipseDiameter / 2);
            opacityNode->appendChildNode(rectNode);
        }
        Q_ASSERT(borderNode->type() == QSGNode::TransformNodeType);

        QSGNode *ellipseNode = borderNode->firstChild();
        Q_ASSERT(ellipseNode->type() == QSGNode::TransformNodeType);

        QSGNode *opacityNode = ellipseNode->firstChild();
        Q_ASSERT(opacityNode->type() == QSGNode::OpacityNodeType);

        QSGInternalRectangleNode *rectNode = static_cast<QSGInternalRectangleNode *>(opacityNode->firstChild());
        Q_ASSERT(rectNode->type() == QSGNode::GeometryNodeType);

        rectNode->setRect(QRectF((EllipseCount - i - 1) * (EllipseDiameter + EllipseOffset), (item->height() - EllipseDiameter) / 2, EllipseDiameter, EllipseDiameter));
        rectNode->setColor(bar->color());
        rectNode->update();

        borderNode = borderNode->nextSibling();
    }
}

QQuickUniversalProgressBar::QQuickUniversalProgressBar(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

QColor QQuickUniversalProgressBar::color() const
{
    return m_color;
}

void QQuickUniversalProgressBar::setColor(const QColor &color)
{
    if (m_color == color)
        return;

    m_color = color;
    update();
}

qreal QQuickUniversalProgressBar::progress() const
{
    return m_progress;
}

void QQuickUniversalProgressBar::setProgress(qreal progress)
{
    if (progress == m_progress)
        return;

    m_progress = progress;
    update();
}

bool QQuickUniversalProgressBar::isIndeterminate() const
{
    return m_indeterminate;
}

void QQuickUniversalProgressBar::setIndeterminate(bool indeterminate)
{
    if (indeterminate == m_indeterminate)
        return;

    m_indeterminate = indeterminate;
    setClip(m_indeterminate);
    update();
}

void QQuickUniversalProgressBar::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    QQuickItem::itemChange(change, data);
    if (change == ItemVisibleHasChanged)
        update();
}

QSGNode *QQuickUniversalProgressBar::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    QQuickUniversalProgressBarNode *node = static_cast<QQuickUniversalProgressBarNode *>(oldNode);
    if (isVisible() && width() > 0 && height() > 0) {
        if (!node)
            node = new QQuickUniversalProgressBarNode(this);
        node->sync(this);
    } else {
        delete node;
        node = nullptr;
    }
    return node;
}

QT_END_NAMESPACE

#include "moc_qquickuniversalprogressbar_p.cpp"
