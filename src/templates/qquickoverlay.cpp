/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#include "qquickoverlay_p.h"
#include "qquickpanel_p.h"
#include <QtQml/qqmlinfo.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

QQuickOverlay::QQuickOverlay(QQuickItem *parent)
    : QQuickItem(parent), m_modalPanels(0)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setFiltersChildMouseEvents(true);
    setVisible(false);
}

void QQuickOverlay::itemChange(ItemChange change, const ItemChangeData &data)
{
    QQuickItem::itemChange(change, data);

    QQuickItem *panelItem = const_cast<QQuickItem *>(data.item);
    QQuickPanel *panel = Q_NULLPTR;
    if (change == ItemChildAddedChange || change == ItemChildRemovedChange)
        panel = qobject_cast<QQuickPanel *>(panelItem->parent());
    if (!panel)
        return;

    if (change == ItemChildAddedChange) {
        if (QQuickPanel *prevPanel = m_panels.value(panelItem)) {
            qmlInfo(panel).nospace() << "Panel is sharing item " << panelItem << " with " << prevPanel
                                     << ". This is not supported and strange things are about to happen.";
            return;
        }

        m_panels.insert(panelItem, panel);
        if (panel->isModal())
            ++m_modalPanels;

        connect(this, &QQuickOverlay::pressed, panel, &QQuickPanel::pressedOutside);
        connect(this, &QQuickOverlay::released, panel, &QQuickPanel::releasedOutside);

        if (!isVisible())
            setVisible(true);
    } else if (change == ItemChildRemovedChange) {
        Q_ASSERT(panel == m_panels.value(panelItem));

        disconnect(this, &QQuickOverlay::pressed, panel, &QQuickPanel::pressedOutside);
        disconnect(this, &QQuickOverlay::released, panel, &QQuickPanel::releasedOutside);

        if (panel->isModal())
            --m_modalPanels;
        m_panels.remove(panelItem);
        if (m_panels.isEmpty())
            setVisible(false);
    }
}

void QQuickOverlay::keyPressEvent(QKeyEvent *event)
{
    event->setAccepted(m_modalPanels > 0);
}

void QQuickOverlay::keyReleaseEvent(QKeyEvent *event)
{
    event->setAccepted(m_modalPanels > 0);
}

void QQuickOverlay::mousePressEvent(QMouseEvent *event)
{
    event->setAccepted(m_modalPanels > 0);
    emit pressed();
}

void QQuickOverlay::mouseMoveEvent(QMouseEvent *event)
{
    event->setAccepted(m_modalPanels > 0);
}

void QQuickOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    event->setAccepted(m_modalPanels > 0);
    emit released();
}

bool QQuickOverlay::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    if (m_modalPanels == 0)
        return false;
    // TODO Filter touch events
    if (event->type() != QEvent::MouseButtonPress)
        return false;
    while (item->parentItem() != this)
        item = item->parentItem();

    bool modalBlocked = false;
    const QQuickItemPrivate *priv = QQuickItemPrivate::get(this);
    const QList<QQuickItem *> &sortedChildren = priv->paintOrderChildItems();
    for (int i = sortedChildren.count() - 1; i >= 0; --i) {
        QQuickItem *panelItem = sortedChildren[i];
        if (panelItem == item)
            break;

        QQuickPanel *panel = m_panels.value(panelItem);
        if (panel) {
            emit panel->pressedOutside();

            if (!modalBlocked && panel->isModal())
                modalBlocked = true;
        }
    }

    return modalBlocked;
}

QT_END_NAMESPACE
