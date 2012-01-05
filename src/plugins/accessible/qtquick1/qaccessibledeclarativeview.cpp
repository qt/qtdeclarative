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

#include "qaccessibledeclarativeview.h"
#include "qdeclarativeaccessible.h"
#include "qaccessibledeclarativeitem.h"


#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

QAccessibleDeclarativeView::QAccessibleDeclarativeView(QWidget *widget)
    :QAccessibleWidget(widget)
{
    m_view = static_cast<QDeclarativeView *>(widget);
}

int QAccessibleDeclarativeView::childCount() const
{
    return 1;
}

QAccessibleInterface *QAccessibleDeclarativeView::child(int index) const
{
    if (index == 0) {
        QDeclarativeItem *declarativeRoot = m_view->accessibleRootItem();
        return new QAccessibleDeclarativeItem(declarativeRoot, m_view);
    }
    return 0;
}

int QAccessibleDeclarativeView::navigate(QAccessible::RelationFlag rel, int entry, QAccessibleInterface **target) const
{
    if (rel == QAccessible::Child) {
        *target = child(entry - 1);
        return *target ? 0 : -1;
    }
    return QAccessibleWidget::navigate(rel, entry, target);
}

QAccessibleInterface *QAccessibleDeclarativeView::childAt(int x, int y) const
{
    return child(0); // return the top-level QML item
}

int QAccessibleDeclarativeView::indexOfChild(const QAccessibleInterface *iface) const
{
    if (iface) {
        QDeclarativeItem *declarativeRoot = m_view->accessibleRootItem();
        if (declarativeRoot == iface->object())
            return 1;
    }
    return -1;
}

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
