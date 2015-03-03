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
#include "qquickstyle_p_p.h"

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
    hasStyle(false), style(Q_NULLPTR), background(Q_NULLPTR),
    topPadding(0), leftPadding(0), rightPadding(0), bottomPadding(0)
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
    res = QQuickStylePrivate::resolve(q, res);
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
    \qmlproperty real QtQuickControls2::Control::topPadding
    \qmlproperty real QtQuickControls2::Control::leftPadding
    \qmlproperty real QtQuickControls2::Control::rightPadding
    \qmlproperty real QtQuickControls2::Control::bottomPadding

    These properties hold the padding.
*/
qreal QQuickControl::topPadding() const
{
    Q_D(const QQuickControl);
    return d->topPadding;
}

void QQuickControl::setTopPadding(qreal padding)
{
    Q_D(QQuickControl);
    if (!qFuzzyCompare(d->topPadding, padding)) {
        d->topPadding = padding;
        emit topPaddingChanged();
    }
}

qreal QQuickControl::leftPadding() const
{
    Q_D(const QQuickControl);
    return d->leftPadding;
}

void QQuickControl::setLeftPadding(qreal padding)
{
    Q_D(QQuickControl);
    if (!qFuzzyCompare(d->leftPadding, padding)) {
        d->leftPadding = padding;
        emit leftPaddingChanged();
    }
}

qreal QQuickControl::rightPadding() const
{
    Q_D(const QQuickControl);
    return d->rightPadding;
}

void QQuickControl::setRightPadding(qreal padding)
{
    Q_D(QQuickControl);
    if (!qFuzzyCompare(d->rightPadding, padding)) {
        d->rightPadding = padding;
        emit rightPaddingChanged();
    }
}

qreal QQuickControl::bottomPadding() const
{
    Q_D(const QQuickControl);
    return d->bottomPadding;
}

void QQuickControl::setBottomPadding(qreal padding)
{
    Q_D(QQuickControl);
    if (!qFuzzyCompare(d->bottomPadding, padding)) {
        d->bottomPadding = padding;
        emit bottomPaddingChanged();
    }
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
        d->hasStyle = style;
        d->resolveStyle(style);

        QEvent change(QEvent::StyleChange);
        foreach (QQuickItem *item, findChildren<QQuickItem *>()) {
            if (qobject_cast<QQuickStylable *>(item))
                QCoreApplication::sendEvent(item, &change);
        }
    }
}

bool QQuickControl::hasStyle() const
{
    Q_D(const QQuickControl);
    return d->hasStyle;
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
