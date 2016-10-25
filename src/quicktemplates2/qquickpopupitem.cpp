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

#include "qquickpopupitem_p_p.h"
#include "qquickapplicationwindow_p.h"
#include "qquickcontrol_p_p.h"
#include "qquickpopup_p_p.h"

QT_BEGIN_NAMESPACE

class QQuickPopupItemPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickPopupItem)

public:
    QQuickPopupItemPrivate(QQuickPopup *popup);

    void implicitWidthChanged() override;
    void implicitHeightChanged() override;

    void resolveFont() override;

    QQuickItem *getContentItem() override;

    QQuickPopup *popup;
};

QQuickPopupItemPrivate::QQuickPopupItemPrivate(QQuickPopup *popup) : popup(popup)
{
    isTabFence = true;
}

void QQuickPopupItemPrivate::implicitWidthChanged()
{
    QQuickControlPrivate::implicitWidthChanged();
    emit popup->implicitWidthChanged();
}

void QQuickPopupItemPrivate::implicitHeightChanged()
{
    QQuickControlPrivate::implicitHeightChanged();
    emit popup->implicitHeightChanged();
}

void QQuickPopupItemPrivate::resolveFont()
{
    if (QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(popup->window()))
        inheritFont(window->font());
}

QQuickItem *QQuickPopupItemPrivate::getContentItem()
{
    Q_Q(QQuickPopupItem);
    if (!contentItem)
        contentItem = new QQuickItem(q);
    return contentItem;
}

QQuickPopupItem::QQuickPopupItem(QQuickPopup *popup) :
    QQuickControl(*(new QQuickPopupItemPrivate(popup)), nullptr)
{
    setParent(popup);
    setVisible(false);
    setFlag(ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::AllButtons);

    // TODO: switch to QStyleHints::useHoverEffects in Qt 5.8
    setHoverEnabled(true);
    // setAcceptHoverEvents(QGuiApplication::styleHints()->useHoverEffects());
    // connect(QGuiApplication::styleHints(), &QStyleHints::useHoverEffectsChanged, this, &QQuickItem::setAcceptHoverEvents);
}

bool QQuickPopupItem::childMouseEventFilter(QQuickItem *child, QEvent *event)
{
    Q_D(QQuickPopupItem);
    return d->popup->childMouseEventFilter(child, event);
}

void QQuickPopupItem::focusInEvent(QFocusEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->focusInEvent(event);
}

void QQuickPopupItem::focusOutEvent(QFocusEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->focusOutEvent(event);
}

void QQuickPopupItem::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->keyPressEvent(event);
}

void QQuickPopupItem::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->keyReleaseEvent(event);
}

void QQuickPopupItem::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->mousePressEvent(event);
}

void QQuickPopupItem::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->mouseMoveEvent(event);
}

void QQuickPopupItem::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->mouseReleaseEvent(event);
}

void QQuickPopupItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->mouseDoubleClickEvent(event);
}

void QQuickPopupItem::mouseUngrabEvent()
{
    Q_D(QQuickPopupItem);
    d->popup->mouseUngrabEvent();
}

void QQuickPopupItem::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickPopupItem);
    d->popup->wheelEvent(event);
}

void QQuickPopupItem::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickPopupItem);
    QQuickControl::contentItemChange(newItem, oldItem);
    d->popup->contentItemChange(newItem, oldItem);
}

void QQuickPopupItem::fontChange(const QFont &newFont, const QFont &oldFont)
{
    Q_D(QQuickPopupItem);
    QQuickControl::fontChange(newFont, oldFont);
    d->popup->fontChange(newFont, oldFont);
}

void QQuickPopupItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickPopupItem);
    QQuickControl::geometryChanged(newGeometry, oldGeometry);
    d->popup->geometryChanged(newGeometry, oldGeometry);
}

void QQuickPopupItem::localeChange(const QLocale &newLocale, const QLocale &oldLocale)
{
    Q_D(QQuickPopupItem);
    QQuickControl::localeChange(newLocale, oldLocale);
    d->popup->localeChange(newLocale, oldLocale);
}

void QQuickPopupItem::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickPopupItem);
    QQuickControl::itemChange(change, data);
    d->popup->itemChange(change, data);
}

void QQuickPopupItem::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    Q_D(QQuickPopupItem);
    QQuickControl::paddingChange(newPadding, oldPadding);
    d->popup->paddingChange(newPadding, oldPadding);
}

QFont QQuickPopupItem::defaultFont() const
{
    Q_D(const QQuickPopupItem);
    return d->popup->defaultFont();
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickPopupItem::accessibleRole() const
{
    Q_D(const QQuickPopupItem);
    return d->popup->accessibleRole();
}
#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE
