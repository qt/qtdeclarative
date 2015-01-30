/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

#include "qquickcontrol_p.h"
#include "qquickcontrol_p_p.h"
#include "qquickabstractapplicationwindow_p.h"
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Control
    \inherits Item
    \instantiates QQuickControl
    \inqmlmodule QtQuick.Controls
    \brief A basic control type.

    TODO
*/

QQuickControlPrivate::QQuickControlPrivate() :
    style(Q_NULLPTR), attributes(QQuickControl::Attr_Count), background(Q_NULLPTR), padding(Q_NULLPTR)
{
}

void QQuickControlPrivate::mirrorChange()
{
    Q_Q(QQuickControl);
    q->mirrorChange();
}

void QQuickControlPrivate::resolveStyle(QQuickStyle *res)
{
    Q_Q(QQuickControl);
    if (!res && q->testAttribute(QQuickControl::Attr_HasStyle))
        return;

    // lookup parent style
    if (!res) {
        QQuickItem *item = q->parentItem();
        while (!res && item) {
            QQuickControl *control = qobject_cast<QQuickControl *>(item);
            if (control && control->testAttribute(QQuickControl::Attr_HasStyle))
                res = control->style();
            item = item->parentItem();
        }
    }

    // fallback to window or global style
    if (!res) {
        QQuickAbstractApplicationWindow *aw = qobject_cast<QQuickAbstractApplicationWindow *>(window);
        if (aw)
            res = aw->style();
        if (!res)
            res = QQuickStyle::instance(qmlEngine(q));
    }

    if (style != res) {
        style = res;
        emit q->styleChanged();
    }
}

QQuickControl::QQuickControl(QQuickItem *parent) :
    QQuickItem(*(new QQuickControlPrivate), parent)
{
}

QQuickControl::QQuickControl(QQuickControlPrivate &dd, QQuickItem *parent) :
    QQuickItem(dd, parent)
{
}

/*!
    \qmlpropertygroup QtQuickControls2::Control::padding
    \qmlproperty real QtQuickControls2::Control::padding.top
    \qmlproperty real QtQuickControls2::Control::padding.left
    \qmlproperty real QtQuickControls2::Control::padding.right
    \qmlproperty real QtQuickControls2::Control::padding.bottom

    This property holds the padding.
*/
QQuickPadding *QQuickControl::padding() const
{
    Q_D(const QQuickControl);
    if (!d->padding)
        d->padding = new QQuickPadding(const_cast<QQuickControl *>(this));
    return d->padding;
}

/*!
    \qmlproperty Style QtQuickControls2::Control::style

    This property holds the style.
*/
QQuickStyle *QQuickControl::style() const
{
    Q_D(const QQuickControl);
    if (!d->style)
        const_cast<QQuickControl *>(this)->d_func()->resolveStyle();
    return d->style;
}

void QQuickControl::setStyle(QQuickStyle *style)
{
    Q_D(QQuickControl);
    if (d->style != style) {
        setAttribute(Attr_HasStyle, style);
        d->resolveStyle(style);

        QEvent change(QEvent::StyleChange);
        foreach (QQuickControl *control, findChildren<QQuickControl *>())
            QCoreApplication::sendEvent(control, &change);
    }
}

void QQuickControl::resetStyle()
{
    setStyle(Q_NULLPTR);
}

/*!
    \qmlproperty Item QtQuickControls2::Control::background

    This property holds the background item.
*/
QQuickItem *QQuickControl::background() const
{
    Q_D(const QQuickControl);
    return d->background;
}

void QQuickControl::setBackground(QQuickItem *background)
{
    Q_D(QQuickControl);
    if (d->background != background) {
        delete d->background;
        d->background = background;
        if (background) {
            background->setParentItem(this);
            if (qFuzzyIsNull(background->z()))
                background->setZ(-1);
        }
        emit backgroundChanged();
    }
}

bool QQuickControl::testAttribute(Attribute attribute) const
{
    Q_D(const QQuickControl);
    return d->attributes.testBit(attribute);
}

void QQuickControl::setAttribute(Attribute attribute, bool on)
{
    Q_D(QQuickControl);
    d->attributes.setBit(attribute, on);
}

bool QQuickControl::event(QEvent *event)
{
    Q_D(QQuickControl);
    if (event->type() == QEvent::StyleChange)
        d->resolveStyle();
    return QQuickItem::event(event);
}

void QQuickControl::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickControl);
    QQuickItem::itemChange(change, data);
    if (change == ItemSceneChange || change == ItemParentHasChanged)
        d->resolveStyle();
}

void QQuickControl::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickControl);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    if (d->background) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(d->background);
        if (!p->widthValid || qFuzzyCompare(d->background->width(), oldGeometry.width()))
            d->background->setWidth(newGeometry.width());
        if (!p->heightValid || qFuzzyCompare(d->background->height(), oldGeometry.height()))
            d->background->setHeight(newGeometry.height());
    }
}

bool QQuickControl::isMirrored() const
{
    Q_D(const QQuickControl);
    return d->isMirrored();
}

void QQuickControl::mirrorChange()
{
}

QT_END_NAMESPACE
