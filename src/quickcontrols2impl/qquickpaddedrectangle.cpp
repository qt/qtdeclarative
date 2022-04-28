/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qquickpaddedrectangle_p.h"

#include <QtQuick/private/qsgadaptationlayer_p.h>

QT_BEGIN_NAMESPACE

QQuickPaddedRectangle::QQuickPaddedRectangle(QQuickItem *parent) :
    QQuickRectangle(parent)
{
}

qreal QQuickPaddedRectangle::padding() const
{
    return m_padding;
}

void QQuickPaddedRectangle::setPadding(qreal padding)
{
    if (!qFuzzyCompare(m_padding, padding)) {
        m_padding = padding;
        update();
        emit paddingChanged();
        if (m_hasTopPadding)
            emit topPaddingChanged();
        if (!m_hasLeftPadding)
            emit leftPaddingChanged();
        if (!m_hasRightPadding)
            emit rightPaddingChanged();
        if (!m_hasBottomPadding)
            emit bottomPaddingChanged();
    }
}

void QQuickPaddedRectangle::resetPadding()
{
    setPadding(0);
}

qreal QQuickPaddedRectangle::topPadding() const
{
    return m_hasTopPadding ? m_topPadding : m_padding;
}

void QQuickPaddedRectangle::setTopPadding(qreal padding)
{
    setTopPadding(padding, true);
}

void QQuickPaddedRectangle::resetTopPadding()
{
    setTopPadding(0, false);
}

qreal QQuickPaddedRectangle::leftPadding() const
{
    return m_hasLeftPadding ? m_leftPadding : m_padding;
}

void QQuickPaddedRectangle::setLeftPadding(qreal padding)
{
    setLeftPadding(padding, true);
}

void QQuickPaddedRectangle::resetLeftPadding()
{
    setLeftPadding(0, false);
}

qreal QQuickPaddedRectangle::rightPadding() const
{
    return m_hasRightPadding ? m_rightPadding : m_padding;
}

void QQuickPaddedRectangle::setRightPadding(qreal padding)
{
    setRightPadding(padding, true);
}

void QQuickPaddedRectangle::resetRightPadding()
{
    setRightPadding(0, false);
}

qreal QQuickPaddedRectangle::bottomPadding() const
{
    return m_hasBottomPadding ? m_bottomPadding : m_padding;
}

void QQuickPaddedRectangle::setBottomPadding(qreal padding)
{
    setBottomPadding(padding, true);
}

void QQuickPaddedRectangle::resetBottomPadding()
{
    setBottomPadding(0, false);
}

void QQuickPaddedRectangle::setTopPadding(qreal padding, bool has)
{
    qreal oldPadding = topPadding();
    m_hasTopPadding = has;
    m_topPadding = padding;
    if (!qFuzzyCompare(oldPadding, padding)) {
        update();
        emit topPaddingChanged();
    }
}

void QQuickPaddedRectangle::setLeftPadding(qreal padding, bool has)
{
    qreal oldPadding = leftPadding();
    m_hasLeftPadding = has;
    m_leftPadding = padding;
    if (!qFuzzyCompare(oldPadding, padding)) {
        update();
        emit leftPaddingChanged();
    }
}

void QQuickPaddedRectangle::setRightPadding(qreal padding, bool has)
{
    qreal oldPadding = rightPadding();
    m_hasRightPadding = has;
    m_rightPadding = padding;
    if (!qFuzzyCompare(oldPadding, padding)) {
        update();
        emit rightPaddingChanged();
    }
}

void QQuickPaddedRectangle::setBottomPadding(qreal padding, bool has)
{
    qreal oldPadding = bottomPadding();
    m_hasBottomPadding = has;
    m_bottomPadding = padding;
    if (!qFuzzyCompare(oldPadding, padding)) {
        update();
        emit bottomPaddingChanged();
    }
}

QSGNode *QQuickPaddedRectangle::updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
{
    QSGTransformNode *transformNode = static_cast<QSGTransformNode *>(node);
    if (!transformNode)
        transformNode = new QSGTransformNode;

    QSGInternalRectangleNode *rectNode = static_cast<QSGInternalRectangleNode *>(QQuickRectangle::updatePaintNode(transformNode->firstChild(), data));

    if (rectNode) {
        if (!transformNode->firstChild())
            transformNode->appendChildNode(rectNode);

        qreal top = topPadding();
        qreal left = leftPadding();
        qreal right = rightPadding();
        qreal bottom = bottomPadding();

        if (!qFuzzyIsNull(top) || !qFuzzyIsNull(left) || !qFuzzyIsNull(right) || !qFuzzyIsNull(bottom)) {
            QMatrix4x4 m;
            m.translate(left, top);
            transformNode->setMatrix(m);

            qreal w = qMax<qreal>(0.0, width() -left-right);
            qreal h = qMax<qreal>(0.0, height() -top-bottom);

            rectNode->setRect(QRectF(0, 0, w, h));
            rectNode->update();
        }
    }
    return transformNode;
}

QT_END_NAMESPACE

#include "moc_qquickpaddedrectangle_p.cpp"
