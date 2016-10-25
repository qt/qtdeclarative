/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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

#include "qquickpopuppositioner_p_p.h"
#include "qquickpopup_p_p.h"

#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

static const QQuickItemPrivate::ChangeTypes AncestorChangeTypes = QQuickItemPrivate::Geometry
                                                                  | QQuickItemPrivate::Parent
                                                                  | QQuickItemPrivate::Children;

static const QQuickItemPrivate::ChangeTypes ItemChangeTypes = QQuickItemPrivate::Geometry
                                                             | QQuickItemPrivate::Parent;

QQuickPopupPositioner::QQuickPopupPositioner(QQuickPopupPrivate *popup) :
    m_parentItem(nullptr),
    m_popup(popup)
{
}

QQuickPopupPositioner::~QQuickPopupPositioner()
{
    if (m_parentItem) {
        QQuickItemPrivate::get(m_parentItem)->removeItemChangeListener(this, ItemChangeTypes);
        removeAncestorListeners(m_parentItem->parentItem());
    }
}

QQuickItem *QQuickPopupPositioner::parentItem() const
{
    return m_parentItem;
}

void QQuickPopupPositioner::setParentItem(QQuickItem *parent)
{
    if (m_parentItem == parent)
        return;

    if (m_parentItem) {
        QQuickItemPrivate::get(m_parentItem)->removeItemChangeListener(this, ItemChangeTypes);
        removeAncestorListeners(m_parentItem->parentItem());
    }

    m_parentItem = parent;

    if (!parent)
        return;

    QQuickItemPrivate::get(parent)->addItemChangeListener(this, ItemChangeTypes);
    addAncestorListeners(parent->parentItem());

    if (m_popup->popupItem->isVisible())
        m_popup->reposition();
}

void QQuickPopupPositioner::itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &)
{
    if (m_parentItem && m_popup->popupItem->isVisible())
        m_popup->reposition();
}

void QQuickPopupPositioner::itemParentChanged(QQuickItem *, QQuickItem *parent)
{
    addAncestorListeners(parent);
}

void QQuickPopupPositioner::itemChildRemoved(QQuickItem *item, QQuickItem *child)
{
    if (isAncestor(child))
        removeAncestorListeners(item);
}

void QQuickPopupPositioner::removeAncestorListeners(QQuickItem *item)
{
    if (item == m_parentItem)
        return;

    QQuickItem *p = item;
    while (p) {
        QQuickItemPrivate::get(p)->removeItemChangeListener(this, AncestorChangeTypes);
        p = p->parentItem();
    }
}

void QQuickPopupPositioner::addAncestorListeners(QQuickItem *item)
{
    if (item == m_parentItem)
        return;

    QQuickItem *p = item;
    while (p) {
        QQuickItemPrivate::get(p)->addItemChangeListener(this, AncestorChangeTypes);
        p = p->parentItem();
    }
}

// TODO: use QQuickItem::isAncestorOf() in dev/5.7
bool QQuickPopupPositioner::isAncestor(QQuickItem *item) const
{
    if (!m_parentItem)
        return false;

    QQuickItem *parent = m_parentItem;
    while (parent) {
        if (parent == item)
            return true;
        parent = parent->parentItem();
    }
    return false;
}

QT_END_NAMESPACE
