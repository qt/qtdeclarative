// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickbasicprogressbar_p.h"

#include <QtCore/qeasingcurve.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qsgadaptationlayer_p.h>
#include <QtQuickControls2Impl/private/qquickanimatednode_p.h>

QT_BEGIN_NAMESPACE

static const int Blocks = 4;
static const int BlockWidth = 16;
static const int BlockRestingSpacing = 4;
static const int BlockMovingSpacing = 48;
static const int BlockSpan = Blocks * (BlockWidth + BlockRestingSpacing) - BlockRestingSpacing;
static const int QbpbTotalDuration = 4000;
static const int SecondPhaseStart = QbpbTotalDuration * 0.4;
static const int ThirdPhaseStart = QbpbTotalDuration * 0.6;

static inline qreal blockStartX(int blockIndex)
{
    return ((blockIndex + 1) * -BlockWidth) - (blockIndex * BlockMovingSpacing);
}

static inline qreal blockRestX(int blockIndex, qreal availableWidth)
{
    const qreal spanRightEdgePos = availableWidth / 2 + BlockSpan / 2.0;
    return spanRightEdgePos - (blockIndex + 1) * BlockWidth - (blockIndex * BlockRestingSpacing);
}

static inline qreal blockEndX(int blockIndex, qreal availableWidth)
{
    return availableWidth - blockStartX(Blocks - 1 - blockIndex) - BlockWidth;
}

class QQuickBasicProgressBarNode : public QQuickAnimatedNode
{
public:
    QQuickBasicProgressBarNode(QQuickBasicProgressBar *item);

    void updateCurrentTime(int time) override;
    void sync(QQuickItem *item) override;

private:
    bool m_indeterminate = false;
    qreal m_pixelsPerSecond = 0;
};

QQuickBasicProgressBarNode::QQuickBasicProgressBarNode(QQuickBasicProgressBar *item)
    : QQuickAnimatedNode(item),
      m_pixelsPerSecond(item->width())
{
    setLoopCount(Infinite);
    setDuration(QbpbTotalDuration);
}

void QQuickBasicProgressBarNode::updateCurrentTime(int time)
{
    QSGTransformNode *transformNode = static_cast<QSGTransformNode*>(firstChild());
    for (int i = 0; i < Blocks; ++i) {
        Q_ASSERT(transformNode->type() == QSGNode::TransformNodeType);

        QMatrix4x4 m;
        const qreal restX = blockRestX(i, m_pixelsPerSecond);
        const qreal timeInSeconds = time / 1000.0;

        if (time < SecondPhaseStart) {
            // Move into the resting position for the first phase.
            QEasingCurve easingCurve(QEasingCurve::InQuad);
            const qreal easedCompletion = easingCurve.valueForProgress(time / qreal(SecondPhaseStart));
            const qreal distance = m_pixelsPerSecond * (easedCompletion * (SecondPhaseStart / 1000.0));
            const qreal position = blockStartX(i) + distance;
            const qreal destination = restX;
            m.translate(qMin(position, destination), 0);
        } else if (time < ThirdPhaseStart) {
            // Stay in the same position for the second phase.
            m.translate(restX, 0);
        } else {
            // Move out of view for the third phase.
            const int thirdPhaseSubKickoff = (BlockMovingSpacing / m_pixelsPerSecond) * 1000;
            const int subphase = (time - ThirdPhaseStart) / thirdPhaseSubKickoff;
            // If we're not at this subphase yet, don't try to animate movement,
            // because it will be incorrect.
            if (subphase < i)
                return;

            const qreal timeSinceSecondPhase = timeInSeconds - (ThirdPhaseStart / 1000.0);
            // We only want to start keeping track of time once our subphase has started,
            // otherwise we move too much because we account for time that has already elapsed.
            // For example, if we were 60 milliseconds into the third subphase:
            //
            //      0 ..... 1 ..... 2 ...
            //         100     100     60
            //
            // i == 0, timeSinceOurKickoff == 260
            // i == 1, timeSinceOurKickoff == 160
            // i == 2, timeSinceOurKickoff ==  60
            const qreal timeSinceOurKickoff = timeSinceSecondPhase - (thirdPhaseSubKickoff / 1000.0 * i);
            const qreal position = restX + (m_pixelsPerSecond * (timeSinceOurKickoff));
            const qreal destination = blockEndX(i, m_pixelsPerSecond);
            m.translate(qMin(position, destination), 0);
        }

        transformNode->setMatrix(m);

        transformNode = static_cast<QSGTransformNode*>(transformNode->nextSibling());
    }
}

void QQuickBasicProgressBarNode::sync(QQuickItem *item)
{
    QQuickBasicProgressBar *bar = static_cast<QQuickBasicProgressBar *>(item);
    if (m_indeterminate != bar->isIndeterminate()) {
        m_indeterminate = bar->isIndeterminate();
        if (m_indeterminate)
            start();
        else
            stop();
    }
    m_pixelsPerSecond = item->width();

    QQuickItemPrivate *d = QQuickItemPrivate::get(item);

    QMatrix4x4 m;
    m.translate(0, (item->height() - item->implicitHeight()) / 2);
    setMatrix(m);

    if (m_indeterminate) {
        if (childCount() != Blocks) {
            // This was previously a regular progress bar; remove the old nodes.
            removeAllChildNodes();
        }

        QSGTransformNode *transformNode = static_cast<QSGTransformNode*>(firstChild());
        for (int i = 0; i < Blocks; ++i) {
            if (!transformNode) {
                transformNode = new QSGTransformNode;
                appendChildNode(transformNode);
            }

            QSGInternalRectangleNode *rectNode = static_cast<QSGInternalRectangleNode*>(transformNode->firstChild());
            if (!rectNode) {
                rectNode = d->sceneGraphContext()->createInternalRectangleNode();
                transformNode->appendChildNode(rectNode);
            }

            QMatrix4x4 m;
            m.translate(blockStartX(i), 0);
            transformNode->setMatrix(m);

            // Set the color here too in case it was set after component completion,
            // as updateCurrentTime doesn't sync it.
            rectNode->setColor(bar->color());
            rectNode->setRect(QRectF(QPointF(0, 0), QSizeF(BlockWidth, item->implicitHeight())));
            rectNode->update();

            transformNode = static_cast<QSGTransformNode *>(transformNode->nextSibling());
        }
    } else {
        if (childCount() > 1) {
            // This was previously an indeterminate progress bar; remove the old nodes.
            removeAllChildNodes();
        }

        QSGInternalRectangleNode *rectNode = static_cast<QSGInternalRectangleNode *>(firstChild());
        if (!rectNode) {
            rectNode = d->sceneGraphContext()->createInternalRectangleNode();
            appendChildNode(rectNode);
        }

        // Always set the color, not just when creating the rectangle node, so that we respect
        // changes that are made after component completion.
        rectNode->setColor(bar->color());
        rectNode->setRect(QRectF(QPointF(0, 0), QSizeF(bar->progress() * item->width(), item->implicitHeight())));
        rectNode->update();
    }
}

QQuickBasicProgressBar::QQuickBasicProgressBar(QQuickItem *parent) :
    QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

qreal QQuickBasicProgressBar::progress() const
{
    return m_progress;
}

void QQuickBasicProgressBar::setProgress(qreal progress)
{
    if (progress == m_progress)
        return;

    m_progress = progress;
    update();
}

bool QQuickBasicProgressBar::isIndeterminate() const
{
    return m_indeterminate;
}

void QQuickBasicProgressBar::setIndeterminate(bool indeterminate)
{
    if (indeterminate == m_indeterminate)
        return;

    m_indeterminate = indeterminate;
    setClip(m_indeterminate);
    update();
}

QColor QQuickBasicProgressBar::color() const
{
    return m_color;
}

void QQuickBasicProgressBar::setColor(const QColor &color)
{
    if (color == m_color)
        return;

    m_color = color;
    update();
}

void QQuickBasicProgressBar::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    QQuickItem::itemChange(change, data);
    if (change == ItemVisibleHasChanged)
        update();
}

QSGNode *QQuickBasicProgressBar::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    QQuickBasicProgressBarNode *node = static_cast<QQuickBasicProgressBarNode *>(oldNode);
    if (isVisible() && width() > 0 && height() > 0) {
        if (!node)
            node = new QQuickBasicProgressBarNode(this);
        node->sync(this);
    } else {
        delete node;
        node = nullptr;
    }
    return node;
}

QT_END_NAMESPACE

#include "moc_qquickbasicprogressbar_p.cpp"
