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
#include "sghighlight.h"
#include "sgselectiontool.h"

#include <QtDeclarative/private/qdeclarativeinspectorservice_p.h>
#include <QtDeclarative/private/qsgitem_p.h>

#include <QtDeclarative/QSGView>
#include <QtDeclarative/QSGItem>
#include <QtGui/QMouseEvent>

#include <cfloat>

namespace QmlJSDebugger {

/*
 * Collects all the items at the given position, from top to bottom.
 */
static void collectItemsAt(QSGItem *item, const QPointF &pos, QSGItem *overlay,
                           QList<QSGItem *> &resultList)
{
    if (item == overlay)
        return;

    if (item->flags() & QSGItem::ItemClipsChildrenToShape) {
        if (!QRectF(0, 0, item->width(), item->height()).contains(pos))
            return;
    }

    QList<QSGItem *> children = QSGItemPrivate::get(item)->paintOrderChildItems();
    for (int i = children.count() - 1; i >= 0; --i) {
        QSGItem *child = children.at(i);
        collectItemsAt(child, item->mapToItem(child, pos), overlay, resultList);
    }

    if (!QRectF(0, 0, item->width(), item->height()).contains(pos))
        return;

    resultList.append(item);
}

/*
 * Returns the first visible item at the given position, or 0 when no such
 * child exists.
 */
static QSGItem *itemAt(QSGItem *item, const QPointF &pos, QSGItem *overlay)
{
    if (item == overlay)
        return 0;

    if (!item->isVisible() || item->opacity() == 0.0)
        return 0;

    if (item->flags() & QSGItem::ItemClipsChildrenToShape) {
        if (!QRectF(0, 0, item->width(), item->height()).contains(pos))
            return 0;
    }

    QList<QSGItem *> children = QSGItemPrivate::get(item)->paintOrderChildItems();
    for (int i = children.count() - 1; i >= 0; --i) {
        QSGItem *child = children.at(i);
        if (QSGItem *betterCandidate = itemAt(child, item->mapToItem(child, pos), overlay))
            return betterCandidate;
    }

    if (!(item->flags() & QSGItem::ItemHasContents))
        return 0;

    if (!QRectF(0, 0, item->width(), item->height()).contains(pos))
        return 0;

    return item;
}


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

QSGItem *SGViewInspector::topVisibleItemAt(const QPointF &pos) const
{
    QSGItem *root = m_view->rootItem();
    return itemAt(root, root->mapFromScene(pos), m_overlay);
}

QList<QSGItem *> SGViewInspector::itemsAt(const QPointF &pos) const
{
    QSGItem *root = m_view->rootItem();
    QList<QSGItem *> resultList;
    collectItemsAt(root, root->mapFromScene(pos), m_overlay, resultList);
    return resultList;
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
        m_selectedItems.append(item);
        m_highlightItems.insert(item, new SGSelectionHighlight(item, m_overlay));
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

bool SGViewInspector::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_view)
        return QObject::eventFilter(obj, event);

    return AbstractViewInspector::eventFilter(obj, event);
}

bool SGViewInspector::mouseMoveEvent(QMouseEvent *event)
{
    if (QSGItem *item = topVisibleItemAt(event->pos()))
        m_view->setToolTip(titleForItem(item));
    else
        m_view->setToolTip(QString());

    return AbstractViewInspector::mouseMoveEvent(event);
}

QString SGViewInspector::titleForItem(QSGItem *item) const
{
    QString className = QLatin1String(item->metaObject()->className());
    QString objectStringId = idStringForObject(item);

    className.remove(QRegExp(QLatin1String("_QMLTYPE_\\d+")));
    className.remove(QRegExp(QLatin1String("_QML_\\d+")));
    if (className.startsWith(QLatin1String("QSG")))
        className = className.mid(3);

    QString constructedName;

    if (!objectStringId.isEmpty()) {
        constructedName = objectStringId + QLatin1String(" (") + className + QLatin1Char(')');
    } else if (!item->objectName().isEmpty()) {
        constructedName = item->objectName() + QLatin1String(" (") + className + QLatin1Char(')');
    } else {
        constructedName = className;
    }

    return constructedName;
}

} // namespace QmlJSDebugger
