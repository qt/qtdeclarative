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

#include <qnamespace.h>
#include "qdeclarativeaccessible.h"

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE


QString Q_GUI_EXPORT qTextBeforeOffsetFromString(int offset, QAccessible2::BoundaryType boundaryType,
        int *startOffset, int *endOffset, const QString& text);
QString Q_GUI_EXPORT qTextAtOffsetFromString(int offset, QAccessible2::BoundaryType boundaryType,
        int *startOffset, int *endOffset, const QString& text);
QString Q_GUI_EXPORT qTextAfterOffsetFromString(int offset, QAccessible2::BoundaryType boundaryType,
        int *startOffset, int *endOffset, const QString& text);

QDeclarativeAccessible::QDeclarativeAccessible(QObject *object)
    :QAccessibleObject(object)
{
}

QDeclarativeAccessible::~QDeclarativeAccessible()
{
}

QFlags<QAccessible::RelationFlag> QDeclarativeAccessible::relationTo(const QAccessibleInterface *) const
{
    return QAccessible::Unrelated;
}

QAccessibleInterface *QDeclarativeAccessible::childAt(int x, int y) const
{
    // Note that this function will disregard stacking order.
    // (QAccessibleQuickView::childAt() does this correctly and more efficient)

    // If the item clips its children, we can return early if the coordinate is outside its rect
    if (clipsChildren()) {
        if (!rect().contains(x, y))
            return 0;
    }

    for (int i = childCount() - 1; i >= 0; --i) {
        QAccessibleInterface *childIface = child(i);
        if (childIface && !childIface->state().invisible) {
            if (childIface->rect().contains(x, y))
                return childIface;
        }
        delete childIface;
    }
    return 0;
}

QAccessible::State QDeclarativeAccessible::state() const
{
    QAccessible::State state;

    //QRect viewRect(QPoint(0, 0), m_implementation->size());
    //QRect itemRect(m_item->scenePos().toPoint(), m_item->boundingRect().size().toSize());

    QRect viewRect_ = viewRect();
    QRect itemRect = rect();

   // qDebug() << "viewRect" << viewRect << "itemRect" << itemRect;
    // error case:
    if (viewRect_.isNull() || itemRect.isNull()) {
        state.invisible = true;
    }

    if (!viewRect_.intersects(itemRect)) {
        state.offscreen = true;
        // state.invisible = true; // no set at this point to ease development
    }

    if (!object()->property("visible").toBool() || qFuzzyIsNull(object()->property("opacity").toDouble())) {
        state.invisible = true;
    }

    if ((role() == QAccessible::CheckBox || role() == QAccessible::RadioButton) && object()->property("checked").toBool()) {
        state.checked = true;
    }

    if (role() == QAccessible::EditableText)
        state.focusable = true;

    //qDebug() << "state?" << m_item->property("state").toString() << m_item->property("status").toString() << m_item->property("visible").toString();

    return state;
}

QStringList QDeclarativeAccessible::actionNames() const
{
    QStringList actions;
    switch (role()) {
    case QAccessible::PushButton:
        actions << QAccessibleActionInterface::pressAction();
        break;
    case QAccessible::RadioButton:
    case QAccessible::CheckBox:
        actions << QAccessibleActionInterface::checkAction();
        break;
    default:
        break;
    }
    return actions;
}

void QDeclarativeAccessible::doAction(const QString &actionName)
{
    if (role() == QAccessible::PushButton && actionName == QAccessibleActionInterface::pressAction()) {
        QMetaObject::invokeMethod(object(), "accessibleAction");
    }
    if ((role() == QAccessible::CheckBox || role() == QAccessible::RadioButton) && actionName == QAccessibleActionInterface::checkAction()) {
        bool checked = object()->property("checked").toBool();
        object()->setProperty("checked",  QVariant(!checked));
    }
}

QStringList QDeclarativeAccessible::keyBindingsForAction(const QString &actionName) const
{
    Q_UNUSED(actionName)
    return QStringList();
}

QVariant QDeclarativeAccessible::invokeMethod(QAccessible::Method method, const QVariantList&)
{
    Q_UNUSED(method)
    return QVariant();
}

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
