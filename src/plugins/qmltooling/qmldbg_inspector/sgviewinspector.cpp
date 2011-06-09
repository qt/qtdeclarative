/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "sgviewinspector.h"

#include "qdeclarativeinspectorprotocol.h"
#include "sgselectiontool.h"

#include <QtDeclarative/private/qdeclarativeinspectorservice_p.h>
#include <QtDeclarative/private/qdeclarativedebughelper_p.h>

#include <QtDeclarative/QSGView>
#include <QtDeclarative/QSGItem>
#include <QtDeclarative/QSGPaintedItem>
#include <QtGui/QMouseEvent>

#include <cfloat>

namespace QmlJSDebugger {

class SGSelectionHighlight : public QSGPaintedItem
{
public:
    SGSelectionHighlight(QSGItem *parent) : QSGPaintedItem(parent)
    { }

    void paint(QPainter *painter)
    {
        painter->setPen(QColor(108, 141, 221));
        painter->drawRect(QRect(0, 0, width() - 1, height() - 1));
    }
};


SGViewInspector::SGViewInspector(QSGView *view, QObject *parent) :
    AbstractViewInspector(parent),
    m_view(view),
    m_overlay(new QSGItem),
    m_selectionTool(new SGSelectionTool(this)),
    m_designMode(true)
{
    // Try to make sure the overlay is always on top
    m_overlay->setZ(FLT_MAX);

    // Make sure mouse hover events are received
    m_view->setMouseTracking(true);

    if (QSGItem *root = view->rootItem())
        m_overlay->setParentItem(root);

    view->installEventFilter(this);
    setCurrentTool(m_selectionTool);
}

void SGViewInspector::changeCurrentObjects(const QList<QObject*> &objects)
{
    QList<QSGItem*> items;
    foreach (QObject *obj, objects)
        if (QSGItem *item = qobject_cast<QSGItem*>(obj))
            items << item;

    syncSelectedItems(items);
}

void SGViewInspector::reloadView()
{
    // TODO
    emit reloadRequested();
}

void SGViewInspector::reparentQmlObject(QObject *object, QObject *newParent)
{
    if (!newParent)
        return;

    object->setParent(newParent);
    QSGItem *newParentItem = qobject_cast<QSGItem*>(newParent);
    QSGItem *item = qobject_cast<QSGItem*>(object);
    if (newParentItem && item)
        item->setParentItem(newParentItem);
}

void SGViewInspector::changeTool(InspectorProtocol::Tool tool)
{
    switch (tool) {
    case InspectorProtocol::ColorPickerTool:
        // TODO
        emit colorPickerActivated();
        break;
    case InspectorProtocol::SelectMarqueeTool:
        // TODO
        emit marqueeSelectToolActivated();
        break;
    case InspectorProtocol::SelectTool:
        setCurrentTool(m_selectionTool);
        emit selectToolActivated();
        break;
    case InspectorProtocol::ZoomTool:
        // TODO
        emit zoomToolActivated();
        break;
    }
}

QDeclarativeEngine *SGViewInspector::declarativeEngine() const
{
    return m_view->engine();
}

QWidget *SGViewInspector::viewWidget() const
{
    return m_view;
}

QList<QSGItem*> SGViewInspector::selectedItems() const
{
    QList<QSGItem *> selection;
    foreach (const QWeakPointer<QSGItem> &selectedItem, m_selectedItems) {
        if (selectedItem)
            selection << selectedItem.data();
    }
    return selection;
}

void SGViewInspector::setSelectedItems(const QList<QSGItem *> &items)
{
    if (!syncSelectedItems(items))
        return;

    QList<QObject*> objectList;
    foreach (QSGItem *item, items)
        objectList << item;

    sendCurrentObjects(objectList);
}

bool SGViewInspector::syncSelectedItems(const QList<QSGItem *> &items)
{
    bool selectionChanged = false;

    // Disconnect and remove items that are no longer selected
    foreach (const QWeakPointer<QSGItem> &item, m_selectedItems) {
        if (!item) // Don't see how this can happen due to handling of destroyed()
            continue;
        if (items.contains(item.data()))
            continue;

        selectionChanged = true;
        item.data()->disconnect(this);
        m_selectedItems.removeOne(item);
        delete m_highlightItems.take(item.data());
    }

    // Connect and add newly selected items
    foreach (QSGItem *item, items) {
        if (m_selectedItems.contains(item))
            continue;

        selectionChanged = true;
        connect(item, SIGNAL(destroyed(QObject*)), this, SLOT(removeFromSelectedItems(QObject*)));
        connect(item, SIGNAL(xChanged()), this, SLOT(adjustSelectionHighlight()));
        connect(item, SIGNAL(yChanged()), this, SLOT(adjustSelectionHighlight()));
        connect(item, SIGNAL(widthChanged()), this, SLOT(adjustSelectionHighlight()));
        connect(item, SIGNAL(heightChanged()), this, SLOT(adjustSelectionHighlight()));
        connect(item, SIGNAL(rotationChanged()), this, SLOT(adjustSelectionHighlight()));
        m_selectedItems.append(item);
        m_highlightItems.insert(item, new SGSelectionHighlight(m_overlay));
        adjustSelectionHighlight(item);
    }

    return selectionChanged;
}

void SGViewInspector::removeFromSelectedItems(QObject *object)
{
    if (QSGItem *item = qobject_cast<QSGItem*>(object)) {
        if (m_selectedItems.removeOne(item))
            delete m_highlightItems.take(item);
    }
}

void SGViewInspector::adjustSelectionHighlight(QSGItem *item)
{
    if (!item)
        item = static_cast<QSGItem*>(sender());

    SGSelectionHighlight *highlight = m_highlightItems.value(item);

    highlight->setSize(QSizeF(item->width(), item->height()));
    highlight->setPos(m_overlay->mapFromItem(item->parentItem(), item->pos()));
}

bool SGViewInspector::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_view)
        return QObject::eventFilter(obj, event);

    return AbstractViewInspector::eventFilter(obj, event);
}

} // namespace QmlJSDebugger
