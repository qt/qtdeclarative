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

#include "qaccessiblequickview.h"

#include <QtQuick/qquickitem.h>

#include "qaccessiblequickitem.h"
#include "qdeclarativeaccessible.h"

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

QAccessibleQuickView::QAccessibleQuickView(QQuickView *object)
    :QAccessibleObject(object)
{
    m_view = static_cast<QQuickView *>(object);
}

int QAccessibleQuickView::childCount() const
{
    return m_view->rootItem() ? 1 : 0;
}

QAccessibleInterface *QAccessibleQuickView::parent() const
{
    // FIXME: for now we assume to be a top level window...
    return QAccessible::queryAccessibleInterface(qApp);
}

QAccessibleInterface *QAccessibleQuickView::child(int index) const
{
    if (index == 0) {
        QQuickItem *declarativeRoot = m_view->rootObject();
        return new QAccessibleQuickItem(declarativeRoot);
    }
    return 0;
}

QAccessible::Role QAccessibleQuickView::role() const
{
    return QAccessible::Window; // FIXME
}

QAccessible::State QAccessibleQuickView::state() const
{
    return QAccessible::Normal; // FIXME
}

QRect QAccessibleQuickView::rect() const
{
    return QRect(m_view->x(), m_view->y(), m_view->width(), m_view->height());
}

int QAccessibleQuickView::navigate(QAccessible::RelationFlag rel, int entry, QAccessibleInterface **target) const
{
    switch (rel) {
    case QAccessible::Child:
        *target = child(entry - 1);
    case QAccessible::Ancestor:
        *target = parent();
    default:
        *target = 0;
    }
    return *target ? 0 : -1;
}

QString QAccessibleQuickView::text(QAccessible::Text text) const
{
#ifdef Q_ACCESSIBLE_QUICK_ITEM_ENABLE_DEBUG_DESCRIPTION
    if (text == QAccessible::DebugDescription) {
        return QString::fromAscii(object()->metaObject()->className()) ;
    }
#endif
    return m_view->windowTitle();
}

QAccessibleInterface *QAccessibleQuickView::childAt(int x, int y) const
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    return child(0); // return the top-level QML item
}

int QAccessibleQuickView::indexOfChild(const QAccessibleInterface *iface) const
{
    if (iface) {
        QQuickItem *declarativeRoot = m_view->rootObject();
        if (declarativeRoot == iface->object())
            return 1;
    }
    return -1;

}

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
