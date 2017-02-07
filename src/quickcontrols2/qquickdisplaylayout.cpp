/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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

#include "qquickdisplaylayout_p.h"
#include "qquickdisplaylayout_p_p.h"

#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

QQuickDisplayLayoutPrivate::QQuickDisplayLayoutPrivate()
    : icon(nullptr),
      text(nullptr),
      display(QQuickDisplayLayout::TextBesideIcon),
      spacing(0),
      mirrored(false),
      topPadding(0),
      leftPadding(0),
      rightPadding(0),
      bottomPadding(0)
{
}

void QQuickDisplayLayoutPrivate::updateImplicitSize()
{
    Q_Q(QQuickDisplayLayout);
    if (!q->isComponentComplete())
        return;

    const bool showIcon = icon && display != QQuickDisplayLayout::TextOnly;
    const bool showText = text && display != QQuickDisplayLayout::IconOnly;
    const qreal horizontalPadding = leftPadding + rightPadding;
    const qreal verticalPadding = topPadding + bottomPadding;
    const qreal iconImplicitWidth = showIcon ? icon->implicitWidth() : 0;
    const qreal iconImplicitHeight = showIcon ? icon->implicitHeight() : 0;
    const qreal textImplicitWidth = showText ? text->implicitWidth() : 0;
    const qreal textImplicitHeight = showText ? text->implicitHeight() : 0;
    const qreal effectiveSpacing = showText && showIcon ? spacing : 0;
    const qreal implicitWidth = iconImplicitWidth + textImplicitWidth + effectiveSpacing + horizontalPadding;
    const qreal implicitHeight = qMax(iconImplicitHeight, textImplicitHeight) + verticalPadding;
    q->setImplicitSize(implicitWidth, implicitHeight);
}

void QQuickDisplayLayoutPrivate::layout()
{
    Q_Q(QQuickDisplayLayout);
    if (!q->isComponentComplete())
        return;

    const qreal horizontalPadding = leftPadding + rightPadding;
    const qreal verticalPadding = topPadding + bottomPadding;
    const qreal w = q->width();
    const qreal h = q->height();
    const qreal availableWidth = w - horizontalPadding;
    const qreal availableHeight = h - verticalPadding;
    const qreal horizontalCenter = w / 2;
    const qreal verticalCenter = h / 2;

    switch (display) {
    case QQuickDisplayLayout::IconOnly:
        if (icon) {
            icon->setWidth(qMin(icon->implicitWidth(), availableWidth));
            icon->setHeight(qMin(icon->implicitHeight(), availableHeight));
            icon->setX(horizontalCenter - icon->width() / 2);
            icon->setY(verticalCenter - icon->height() / 2);
            icon->setVisible(true);
        }
        if (text)
            text->setVisible(false);
        break;
    case QQuickDisplayLayout::TextOnly:
        if (text) {
            text->setWidth(qMin(text->implicitWidth(), availableWidth));
            text->setHeight(qMin(text->implicitHeight(), availableHeight));
            text->setX(horizontalCenter - text->width() / 2);
            text->setY(verticalCenter - text->height() / 2);
            text->setVisible(true);
        }
        if (icon)
            icon->setVisible(false);
        break;
    case QQuickDisplayLayout::TextBesideIcon:
    default:
        // Work out the sizes first, as the positions depend on them.
        qreal iconWidth = 0;
        qreal textWidth = 0;
        if (icon) {
            icon->setWidth(qMin(icon->implicitWidth(), availableWidth));
            icon->setHeight(qMin(icon->implicitHeight(), availableHeight));
            iconWidth = icon->width();
        }
        if (text) {
            text->setWidth(qMin(text->implicitWidth(), availableWidth - iconWidth - spacing));
            text->setHeight(qMin(text->implicitHeight(), availableHeight));
            textWidth = text->width();
        }

        const qreal combinedWidth = iconWidth + spacing + textWidth;
        const qreal contentX = horizontalCenter - combinedWidth / 2;
        if (icon) {
            icon->setX(mirrored ? contentX + combinedWidth - iconWidth : contentX);
            icon->setY(verticalCenter - icon->height() / 2);
            icon->setVisible(true);
        }
        if (text) {
            text->setX(mirrored ? contentX : contentX + combinedWidth - text->width());
            text->setY(verticalCenter - text->height() / 2);
            text->setVisible(true);
        }
        break;
    }
}

static const QQuickItemPrivate::ChangeTypes itemChangeTypes =
    QQuickItemPrivate::ImplicitWidth
    | QQuickItemPrivate::ImplicitHeight
    | QQuickItemPrivate::Destroyed;

void QQuickDisplayLayoutPrivate::watchChanges(QQuickItem *item)
{
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    itemPrivate->addItemChangeListener(this, itemChangeTypes);
}

void QQuickDisplayLayoutPrivate::unwatchChanges(QQuickItem* item)
{
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    itemPrivate->removeItemChangeListener(this, itemChangeTypes);
}

void QQuickDisplayLayoutPrivate::itemImplicitWidthChanged(QQuickItem *)
{
    updateImplicitSize();
}

void QQuickDisplayLayoutPrivate::itemImplicitHeightChanged(QQuickItem *)
{
    updateImplicitSize();
}

void QQuickDisplayLayoutPrivate::itemDestroyed(QQuickItem *item)
{
    unwatchChanges(item);
    if (item == icon)
        icon = nullptr;
    else if (item == text)
        text = nullptr;
}

QQuickDisplayLayout::QQuickDisplayLayout(QQuickItem *parent)
    : QQuickItem(*(new QQuickDisplayLayoutPrivate), parent)
{
}

QQuickDisplayLayout::~QQuickDisplayLayout()
{
    Q_D(QQuickDisplayLayout);
    if (d->icon)
        d->unwatchChanges(d->icon);
    if (d->text)
        d->unwatchChanges(d->text);
}

QQuickItem *QQuickDisplayLayout::icon() const
{
    Q_D(const QQuickDisplayLayout);
    return d->icon;
}

void QQuickDisplayLayout::setIcon(QQuickItem *icon)
{
    Q_D(QQuickDisplayLayout);
    if (d->icon == icon)
        return;

    if (d->icon)
        d->unwatchChanges(d->icon);

    d->icon = icon;
    if (d->icon) {
        d->icon->setParentItem(this);
        d->watchChanges(d->icon);
    }

    d->updateImplicitSize();
    d->layout();

    emit iconChanged();
}

QQuickItem *QQuickDisplayLayout::text() const
{
    Q_D(const QQuickDisplayLayout);
    return d->text;
}

void QQuickDisplayLayout::setText(QQuickItem *text)
{
    Q_D(QQuickDisplayLayout);
    if (d->text == text)
        return;

    if (d->text)
        d->unwatchChanges(d->text);

    d->text = text;
    if (d->text) {
        d->text->setParentItem(this);
        d->watchChanges(d->text);
    }

    d->updateImplicitSize();
    d->layout();

    emit textChanged();
}

QQuickDisplayLayout::Display QQuickDisplayLayout::display() const
{
    Q_D(const QQuickDisplayLayout);
    return d->display;
}

void QQuickDisplayLayout::setDisplay(Display display)
{
    Q_D(QQuickDisplayLayout);
    if (d->display == display)
        return;

    d->display = display;
    d->updateImplicitSize();
    d->layout();
    emit displayChanged();
}

qreal QQuickDisplayLayout::spacing() const
{
    Q_D(const QQuickDisplayLayout);
    return d->spacing;
}

void QQuickDisplayLayout::setSpacing(qreal spacing)
{
    Q_D(QQuickDisplayLayout);
    if (qFuzzyCompare(d->spacing, spacing))
        return;

    d->spacing = spacing;
    d->updateImplicitSize();
    d->layout();
    emit spacingChanged();
}

bool QQuickDisplayLayout::isMirrored() const
{
    Q_D(const QQuickDisplayLayout);
    return d->mirrored;
}

void QQuickDisplayLayout::setMirrored(bool mirrored)
{
    Q_D(QQuickDisplayLayout);
    if (d->mirrored == mirrored)
        return;

    d->mirrored = mirrored;
    d->updateImplicitSize();
    d->layout();
    emit mirroredChanged();
}

qreal QQuickDisplayLayout::topPadding() const
{
    Q_D(const QQuickDisplayLayout);
    return d->topPadding;
}

void QQuickDisplayLayout::setTopPadding(qreal padding)
{
    Q_D(QQuickDisplayLayout);
    if (qFuzzyCompare(d->topPadding, padding))
        return;

    d->topPadding = padding;
    d->updateImplicitSize();
    d->layout();
    emit topPaddingChanged();
}

void QQuickDisplayLayout::resetTopPadding()
{
    setTopPadding(0);
}

qreal QQuickDisplayLayout::leftPadding() const
{
    Q_D(const QQuickDisplayLayout);
    return d->leftPadding;
}

void QQuickDisplayLayout::setLeftPadding(qreal padding)
{
    Q_D(QQuickDisplayLayout);
    if (qFuzzyCompare(d->leftPadding, padding))
        return;

    d->leftPadding = padding;
    d->updateImplicitSize();
    d->layout();
    emit leftPaddingChanged();
}

void QQuickDisplayLayout::resetLeftPadding()
{
    setLeftPadding(0);
}

qreal QQuickDisplayLayout::rightPadding() const
{
    Q_D(const QQuickDisplayLayout);
    return d->rightPadding;
}

void QQuickDisplayLayout::setRightPadding(qreal padding)
{
    Q_D(QQuickDisplayLayout);
    if (qFuzzyCompare(d->rightPadding, padding))
        return;

    d->rightPadding = padding;
    d->updateImplicitSize();
    d->layout();
    emit rightPaddingChanged();
}

void QQuickDisplayLayout::resetRightPadding()
{
    setRightPadding(0);
}

qreal QQuickDisplayLayout::bottomPadding() const
{
    Q_D(const QQuickDisplayLayout);
    return d->bottomPadding;
}

void QQuickDisplayLayout::setBottomPadding(qreal padding)
{
    Q_D(QQuickDisplayLayout);
    if (qFuzzyCompare(d->bottomPadding, padding))
        return;

    d->bottomPadding = padding;
    d->updateImplicitSize();
    d->layout();
    emit bottomPaddingChanged();
}

void QQuickDisplayLayout::resetBottomPadding()
{
    setBottomPadding(0);
}

void QQuickDisplayLayout::componentComplete()
{
    Q_D(QQuickDisplayLayout);
    QQuickItem::componentComplete();
    d->updateImplicitSize();
    d->layout();
}

void QQuickDisplayLayout::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickDisplayLayout);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    d->layout();
}

QT_END_NAMESPACE
