/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaccessibledeclarativeitem.h"

#include <QtQuick1/qdeclarativeitem.h>
#include <QtQuick1/private/qdeclarativeaccessibleattached_p.h>

QT_BEGIN_NAMESPACE

QAccessibleDeclarativeItem::QAccessibleDeclarativeItem(QGraphicsObject *item, QGraphicsView *view)
    :QDeclarativeAccessible(item)
    ,m_item(item)
    ,m_view(view)
{

}

int QAccessibleDeclarativeItem::childCount() const
{
    QList<QGraphicsItem *> children = m_item->childItems();
    return children.count();
}

QRect QAccessibleDeclarativeItem::rect() const
{
    QRectF sceneRect = m_item->sceneTransform().mapRect(m_item->boundingRect());
    QPoint pos = m_view->mapFromScene(m_view->mapToGlobal(sceneRect.topLeft().toPoint()));
    QSize size = sceneRect.size().toSize();
    return QRect(pos, size);
}

QRect QAccessibleDeclarativeItem::viewRect() const
{
    QPoint screenPos = m_view->mapToGlobal(m_view->pos());
    return QRect(screenPos, m_view->size());
}

bool QAccessibleDeclarativeItem::clipsChildren() const
{
    return static_cast<QDeclarativeItem *>(m_item)->clip();
}

static inline bool isAncestor(const QObject *ancestorCandidate, const QObject *child)
{
    while (child) {
        if (child == ancestorCandidate)
            return true;
        child = child->parent();
    }
    return false;
}


QAccessibleInterface *QAccessibleDeclarativeItem::parent() const
{
    QGraphicsItem *parent = m_item->parentItem();
    QGraphicsObject *parentObj = parent ? parent->toGraphicsObject() : 0;
    if (parent && !parentObj)
        qWarning("Can not make QGraphicsItems accessible");
    QAccessibleInterface *ancestor = (parentObj
             ? new QAccessibleDeclarativeItem(parentObj, m_view)
             : QAccessible::queryAccessibleInterface(m_view));
    return ancestor;
}

QAccessibleInterface *QAccessibleDeclarativeItem::child(int index) const
{
    QList<QGraphicsItem *> children = m_item->childItems();

    if (index >= children.count())
        return 0;

    QGraphicsItem *child = children.at(index);
    QGraphicsObject *childObject = qobject_cast<QGraphicsObject *>(child);
    if (!childObject)
        return 0;

    return new QAccessibleDeclarativeItem(childObject, m_view);
}

int QAccessibleDeclarativeItem::navigate(QAccessible::RelationFlag rel, int entry, QAccessibleInterface **target) const
{
    Q_UNUSED(rel);
    Q_UNUSED(entry);
    *target = 0;
    return -1;
}

/*! \reimp */
QAccessibleInterface *QAccessibleDeclarativeItem::focusChild() const
{
    QGraphicsObject *focusObject = 0;
    if (m_item->hasFocus()) {
        focusObject = m_item->toGraphicsObject();
    } else {
        if (QGraphicsScene *scene = m_view->scene()) {
            if (QGraphicsItem *focusItem = scene->focusItem()) {
                if (m_item->isAncestorOf(focusItem)) {
                    focusObject = focusItem->toGraphicsObject();
                }
            }
        }
    }
    if (focusObject)
        return new QAccessibleDeclarativeItem(focusObject, m_view); //###queryAccessibleInterface?
    return 0;
}

int QAccessibleDeclarativeItem::indexOfChild(const QAccessibleInterface *iface) const
{
    // ### No QAccessibleInterfaces are created with a QGraphicsItem.
    // However, we want to support QML, not QGraphicsView in general.
    // And since the UI is written in QML, this means we can assume that *all*
    // QGraphicsItems are actually QGraphicsObjects

    const QGraphicsObject *childObj = static_cast<QGraphicsObject*>(iface->object());
    if (m_item == childObj)
        return 0;

    QList<QGraphicsItem*> kids = m_item->childItems();
    int index = kids.indexOf(const_cast<QGraphicsItem*>(static_cast<const QGraphicsItem*>(childObj)));
    if (index != -1) {
        ++index;
    }
    return index;
}

QAccessible::State QAccessibleDeclarativeItem::state() const
{
    QAccessible::State state;
    state.focused = m_item->hasFocus();
    return state;
}

QAccessible::Role QAccessibleDeclarativeItem::role() const
{
    // ### Workaround for setAccessibleRole() not working.
    // Text items are special since they are defined
    // entirely from C++ (setting the role from QML works.)
//    if (qobject_cast<QDeclarative1Text*>(m_item))
//        return QAccessible::StaticText;

    QVariant v = QDeclarativeAccessibleAttached::property(m_item, "role");
    bool ok;
    QAccessible::Role role = (QAccessible::Role)v.toInt(&ok);
    if (!ok)    // Not sure if this check is needed.
        role = QAccessible::Pane;
    return role;
}

bool QAccessibleDeclarativeItem::isAccessible() const
{
    return true;
}

QString QAccessibleDeclarativeItem::text(QAccessible::Text textType) const
{
    // handles generic behaviour not specific to an item
    switch (textType) {
    case QAccessible::Name: {
        QVariant accessibleName = QDeclarativeAccessibleAttached::property(object(), "name");
        if (!accessibleName.isNull())
            return accessibleName.toString();
        break;}
    case QAccessible::Description: {
        QVariant accessibleDecription = QDeclarativeAccessibleAttached::property(object(), "description");
        if (!accessibleDecription.isNull())
            return accessibleDecription.toString();
        break;}
    case QAccessible::Value:
    case QAccessible::Help:
    case QAccessible::Accelerator:
    default:
        break;
    }

    // the following blocks handles item-specific behaviour
    if (role() == QAccessible::EditableText) {
        if (textType == QAccessible::Value) {
            QVariant text = object()->property("text");
            return text.toString();
        } else if (textType == QAccessible::Name) {
            return object()->objectName();
        }
    } else {
        if (textType == QAccessible::Name) {
            QVariant text = object()->property("text");
            return text.toString();
        }
    }


    return QString();
}

QT_END_NAMESPACE
