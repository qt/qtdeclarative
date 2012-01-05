/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qaccessiblequickitem.h"
#include "QtQuick/private/qquickitem_p.h"
#include "QtQuick/private/qquicktext_p.h"
#include "QtQuick/private/qquickaccessibleattached_p.h"

QT_BEGIN_NAMESPACE

QAccessibleQuickItem::QAccessibleQuickItem(QQuickItem *item)
    : QDeclarativeAccessible(item)
    , m_item(item)
{
}

int QAccessibleQuickItem::childCount() const
{
    return childItems().count();
}

QRect QAccessibleQuickItem::rect() const
{
    // ### no canvas in some cases.
    // ### Should we really check for 0 opacity?
    if (!m_item->canvas() ||!m_item->isVisible() || qFuzzyIsNull(m_item->opacity())) {
        return QRect();
    }

    QSizeF size = QSizeF(m_item->width(), m_item->height());
    // ### If the bounding rect fails, we first try the implicit size, then we go for the
    // parent size. WE MIGHT HAVE TO REVISIT THESE FALLBACKS.
    if (size.isEmpty()) {
        size = QSizeF(m_item->implicitWidth(), m_item->implicitHeight());
        if (size.isEmpty())
            // ### Seems that the above fallback is not enough, fallback to use the parent size...
            size = QSizeF(m_item->parentItem()->width(), m_item->parentItem()->height());
    }

    QRectF sceneRect = m_item->mapRectToScene(QRectF(QPointF(0, 0), size));
    QPoint screenPos = m_item->canvas()->mapToGlobal(sceneRect.topLeft().toPoint());

    QRect r = QRect(screenPos, sceneRect.size().toSize());

    if (!r.isValid()) {
        qWarning() << m_item->metaObject()->className() << m_item->property("accessibleText") << r;
    }
    return r;
}

QRect QAccessibleQuickItem::viewRect() const
{
    // ### no canvas in some cases.
    if (!m_item->canvas()) {
        return QRect();
    }

    QQuickCanvas *canvas = m_item->canvas();
    QPoint screenPos = canvas->mapToGlobal(QPoint(0,0));
    return QRect(screenPos, canvas->size());
}


bool QAccessibleQuickItem::clipsChildren() const
{
    return static_cast<QQuickItem *>(m_item)->clip();
}


QAccessibleInterface *QAccessibleQuickItem::parent() const
{
    QQuickItem *parent = m_item->parentItem();
    if (parent) {
        QQuickCanvas *canvas = m_item->canvas();
        // Jump out to the scene widget if the parent is the root item.
        // There are two root items, QQuickCanvas::rootItem and
        // QQuickView::declarativeRoot. The former is the true root item,
        // but is not a part of the accessibility tree. Check if we hit
        // it here and return an interface for the scene instead.
        if (parent == canvas->rootItem()) {
            return QAccessible::queryAccessibleInterface(canvas);
        } else {
            QDeclarativeAccessible *ancestor = new QAccessibleQuickItem(parent);
            return ancestor;
        }
    }
    return 0;
}

QAccessibleInterface *QAccessibleQuickItem::child(int index) const
{
    QList<QQuickItem *> children = childItems();

    if (index < 0 || index >= children.count())
        return 0;

    QQuickItem *child = children.at(index);
    if (!child) // FIXME can this happen?
        return 0;

    return new QAccessibleQuickItem(child);
}

int QAccessibleQuickItem::navigate(QAccessible::RelationFlag rel, int entry, QAccessibleInterface **target) const
{
    *target = 0;
    if (entry == 0) {
        *target = new QAccessibleQuickItem(m_item);
        return 0;
    }

    switch (rel) {
    case QAccessible::Child: { // FIMXE
        QList<QQuickItem *> children = childItems();
        const int childIndex = entry - 1;

        if (childIndex >= children.count())
            return -1;

        QQuickItem *child = children.at(childIndex);
        if (!child)
            return -1;

        *target = new QAccessibleQuickItem(child);
        return 0;
        break;}
    case QAccessible::Ancestor: { // FIMXE
        QQuickItem *parent = m_item->parentItem();
        if (parent) {
            QDeclarativeAccessible *ancestor = new QAccessibleQuickItem(parent);
            if (entry == 1) {
                QQuickCanvas *canvas = m_item->canvas();
                // Jump out to the scene widget if the parent is the root item.
                // There are two root items, QQuickCanvas::rootItem and
                // QQuickView::declarativeRoot. The former is the true root item,
                // but is not a part of the accessibility tree. Check if we hit
                // it here and return an interface for the scene instead.
                if (parent == canvas->rootItem()) {
                    *target = QAccessible::queryAccessibleInterface(canvas);
                } else {
                    *target = ancestor;
                }
                return 0;
            } else if (entry > 1) {
                int ret = ancestor->navigate(QAccessible::Ancestor, entry - 1, target);
                delete ancestor;
                return ret;
            }
        }
        return -1;
    break;}
    default: break;
    }

    return -1;
}

int QAccessibleQuickItem::indexOfChild(const QAccessibleInterface *iface) const
{
    QList<QQuickItem*> kids = childItems();
    int idx = kids.indexOf(static_cast<QQuickItem*>(iface->object()));
    if (idx != -1)
        ++idx;
    return idx;
}

QList<QQuickItem *> QAccessibleQuickItem::childItems() const
{
    if (role() == QAccessible::Button)
        return QList<QQuickItem *>();
    return m_item->childItems();
}

QFlags<QAccessible::StateFlag> QAccessibleQuickItem::state() const
{
    QAccessible::State state = QAccessible::Normal;

    if (m_item->hasActiveFocus()) {
        state |= QAccessible::Focused;
    }
    return state;
}


QAccessible::Role QAccessibleQuickItem::role() const
{
    // Workaround for setAccessibleRole() not working for
    // Text items. Text items are special since they are defined
    // entirely from C++ (setting the role from QML works.)
    if (qobject_cast<QQuickText*>(const_cast<QQuickItem *>(m_item)))
        return QAccessible::StaticText;

    QVariant v = QQuickAccessibleAttached::property(m_item, "role");
    bool ok;
    QAccessible::Role role = (QAccessible::Role)v.toInt(&ok);
    if (!ok)    // Not sure if this check is needed.
        role = QAccessible::Pane;
    return role;
}

bool QAccessibleQuickItem::isAccessible() const
{
    return m_item->d_func()->isAccessible;
}

QString QAccessibleQuickItem::text(QAccessible::Text textType) const
{
    // handles generic behavior not specific to an item
    switch (textType) {
    case QAccessible::Name: {
        QVariant accessibleName = QQuickAccessibleAttached::property(object(), "name");
        if (!accessibleName.isNull())
            return accessibleName.toString();
        break;}
    case QAccessible::Description: {
        QVariant accessibleDecription = QQuickAccessibleAttached::property(object(), "description");
        if (!accessibleDecription.isNull())
            return accessibleDecription.toString();
        break;}
#ifdef Q_ACCESSIBLE_QUICK_ITEM_ENABLE_DEBUG_DESCRIPTION
    case QAccessible::DebugDescription: {
        QString debugString;
        debugString = QString::fromAscii(object()->metaObject()->className()) + QLatin1Char(' ');
        debugString += isAccessible() ? QLatin1String("enabled") : QLatin1String("disabled");
        return debugString;
        break; }
#endif
    case QAccessible::Value:
    case QAccessible::Help:
    case QAccessible::Accelerator:
    default:
        break;
    }

    // the following blocks handles item-specific behavior
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
