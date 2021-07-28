/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qquicktooltippopupitem_p_p.h"

#include <QtQuickTemplates2/private/qquickpopup_p.h>

QT_BEGIN_NAMESPACE

QQuickToolTipPopupItemPrivate::QQuickToolTipPopupItemPrivate(QQuickPopup *popup)
    : QQuickPopupItemPrivate(popup)
{
}

qreal QQuickToolTipPopupItemPrivate::getContentWidth() const
{
    auto textItem = qobject_cast<QQuickText*>(contentItem);
    if (textItem)
        return textItem->contentWidth();

    return contentItem ? contentItem->implicitWidth() : 0;
}

void QQuickToolTipPopupItemPrivate::updateContentWidth()
{
    Q_Q(QQuickToolTipPopupItem);
    // Don't need to calculate the implicit contentWidth if an explicit one was set.
    if (hasContentWidth)
        return;

    auto textItem = qobject_cast<QQuickText*>(contentItem);
    if (!textItem) {
        // It's not a Text item, so use the base contentWidth logic (i.e. use implicitWidth).
        QQuickPopupItemPrivate::updateContentWidth();
        return;
    }

    const qreal oldContentWidth = contentWidth;
    const qreal newContentWidth = textItem->contentWidth();
    if (qFuzzyCompare(oldContentWidth, newContentWidth))
        return;

    contentWidth = newContentWidth;
    q->contentSizeChange(QSizeF(contentWidth, contentHeight), QSizeF(oldContentWidth, contentHeight));
    emit q->contentWidthChanged();
}

QQuickToolTipPopupItem::QQuickToolTipPopupItem(QQuickPopup *popup)
    : QQuickPopupItem(*(new QQuickToolTipPopupItemPrivate(popup)))
{
}

void QQuickToolTipPopupItem::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickToolTipPopupItem);
    QQuickPopupItem::contentItemChange(newItem, oldItem);

    // Text's implicitWidth does not account for newlines and hence is too large,
    // so we need to listen to contentWidth's change signals.
    auto oldTextItem = qobject_cast<QQuickText*>(oldItem);
    if (oldTextItem) {
        QObjectPrivate::disconnect(oldTextItem, &QQuickText::contentWidthChanged,
            d, &QQuickToolTipPopupItemPrivate::updateContentWidth);
    }

    auto newTextItem = qobject_cast<QQuickText*>(newItem);
    if (newTextItem) {
        QObjectPrivate::connect(newTextItem, &QQuickText::contentWidthChanged,
            d, &QQuickToolTipPopupItemPrivate::updateContentWidth);
    }
}

QT_END_NAMESPACE
