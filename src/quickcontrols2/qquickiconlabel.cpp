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

#include "qquickiconlabel_p.h"
#include "qquickiconlabel_p_p.h"

#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

QQuickIconLabelPrivate::QQuickIconLabelPrivate()
    : icon(nullptr),
      label(nullptr),
      display(QQuickIconLabel::TextBesideIcon),
      spacing(0),
      mirrored(false),
      topPadding(0),
      leftPadding(0),
      rightPadding(0),
      bottomPadding(0)
{
}

void QQuickIconLabelPrivate::updateImplicitSize()
{
    Q_Q(QQuickIconLabel);
    if (!componentComplete)
        return;

    const bool showIcon = icon && display != QQuickIconLabel::TextOnly;
    const bool showText = label && display != QQuickIconLabel::IconOnly;
    const qreal horizontalPadding = leftPadding + rightPadding;
    const qreal verticalPadding = topPadding + bottomPadding;
    const qreal iconImplicitWidth = showIcon ? icon->implicitWidth() : 0;
    const qreal iconImplicitHeight = showIcon ? icon->implicitHeight() : 0;
    const qreal textImplicitWidth = showText ? label->implicitWidth() : 0;
    const qreal textImplicitHeight = showText ? label->implicitHeight() : 0;
    const qreal effectiveSpacing = showText && showIcon && icon->implicitWidth() > 0 ? spacing : 0;
    const qreal implicitWidth = iconImplicitWidth + textImplicitWidth + effectiveSpacing + horizontalPadding;
    const qreal implicitHeight = qMax(iconImplicitHeight, textImplicitHeight) + verticalPadding;
    q->setImplicitSize(implicitWidth, implicitHeight);
}

void QQuickIconLabelPrivate::layout()
{
    if (!componentComplete)
        return;

    const qreal horizontalPadding = leftPadding + rightPadding;
    const qreal verticalPadding = topPadding + bottomPadding;
    const qreal availableWidth = width - horizontalPadding;
    const qreal availableHeight = height - verticalPadding;
    const qreal horizontalCenter = width / 2;
    const qreal verticalCenter = height / 2;

    switch (display) {
    case QQuickIconLabel::IconOnly:
        if (icon) {
            icon->setSize(QSizeF(qMin(icon->implicitWidth(), availableWidth),
                                 qMin(icon->implicitHeight(), availableHeight)));
            icon->setPosition(QPointF(horizontalCenter - icon->width() / 2,
                                      verticalCenter - icon->height() / 2));
            icon->setVisible(true);
        }
        if (label)
            label->setVisible(false);
        break;
    case QQuickIconLabel::TextOnly:
        if (label) {
            label->setSize(QSizeF(qMin(label->implicitWidth(), availableWidth),
                                  qMin(label->implicitHeight(), availableHeight)));
            label->setPosition(QPointF(horizontalCenter - label->width() / 2,
                                       verticalCenter - label->height() / 2));
            label->setVisible(true);
        }
        if (icon)
            icon->setVisible(false);
        break;
    case QQuickIconLabel::TextBesideIcon:
    default:
        // Work out the sizes first, as the positions depend on them.
        qreal iconWidth = 0;
        qreal textWidth = 0;
        if (icon) {
            icon->setSize(QSizeF(qMin(icon->implicitWidth(), availableWidth),
                                 qMin(icon->implicitHeight(), availableHeight)));
            iconWidth = icon->width();
        }
        qreal effectiveSpacing = 0;
        if (label) {
            if (iconWidth > 0)
                effectiveSpacing = spacing;
            label->setSize(QSizeF(qMin(label->implicitWidth(), availableWidth - iconWidth - effectiveSpacing),
                                  qMin(label->implicitHeight(), availableHeight)));
            textWidth = label->width();
        }

        const qreal combinedWidth = iconWidth + effectiveSpacing + textWidth;
        const qreal contentX = horizontalCenter - combinedWidth / 2;
        if (icon) {
            icon->setPosition(QPointF(mirrored ? contentX + combinedWidth - iconWidth : contentX,
                                      verticalCenter - icon->height() / 2));
            icon->setVisible(true);
        }
        if (label) {
            label->setPosition(QPointF(mirrored ? contentX : contentX + combinedWidth - label->width(),
                                       verticalCenter - label->height() / 2));
            label->setVisible(true);
        }
        break;
    }
}

static const QQuickItemPrivate::ChangeTypes itemChangeTypes =
    QQuickItemPrivate::ImplicitWidth
    | QQuickItemPrivate::ImplicitHeight
    | QQuickItemPrivate::Destroyed;

void QQuickIconLabelPrivate::watchChanges(QQuickItem *item)
{
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    itemPrivate->addItemChangeListener(this, itemChangeTypes);
}

void QQuickIconLabelPrivate::unwatchChanges(QQuickItem* item)
{
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    itemPrivate->removeItemChangeListener(this, itemChangeTypes);
}

void QQuickIconLabelPrivate::itemImplicitWidthChanged(QQuickItem *)
{
    updateImplicitSize();
    layout();
}

void QQuickIconLabelPrivate::itemImplicitHeightChanged(QQuickItem *)
{
    updateImplicitSize();
    layout();
}

void QQuickIconLabelPrivate::itemDestroyed(QQuickItem *item)
{
    unwatchChanges(item);
    if (item == icon)
        icon = nullptr;
    else if (item == label)
        label = nullptr;
}

QQuickIconLabel::QQuickIconLabel(QQuickItem *parent)
    : QQuickItem(*(new QQuickIconLabelPrivate), parent)
{
}

QQuickIconLabel::~QQuickIconLabel()
{
    Q_D(QQuickIconLabel);
    if (d->icon)
        d->unwatchChanges(d->icon);
    if (d->label)
        d->unwatchChanges(d->label);
}

QQuickItem *QQuickIconLabel::icon() const
{
    Q_D(const QQuickIconLabel);
    return d->icon;
}

void QQuickIconLabel::setIcon(QQuickItem *icon)
{
    Q_D(QQuickIconLabel);
    if (d->icon == icon)
        return;

    if (d->icon)
        d->unwatchChanges(d->icon);

    d->icon = icon;
    if (icon) {
        icon->setParentItem(this);
        d->watchChanges(icon);
    }

    d->updateImplicitSize();
    d->layout();
}

QQuickItem *QQuickIconLabel::label() const
{
    Q_D(const QQuickIconLabel);
    return d->label;
}

void QQuickIconLabel::setLabel(QQuickItem *label)
{
    Q_D(QQuickIconLabel);
    if (d->label == label)
        return;

    if (d->label)
        d->unwatchChanges(d->label);

    d->label = label;
    if (label) {
        label->setParentItem(this);
        d->watchChanges(label);
    }

    d->updateImplicitSize();
    d->layout();
}

QQuickIconLabel::Display QQuickIconLabel::display() const
{
    Q_D(const QQuickIconLabel);
    return d->display;
}

void QQuickIconLabel::setDisplay(Display display)
{
    Q_D(QQuickIconLabel);
    if (d->display == display)
        return;

    d->display = display;
    d->updateImplicitSize();
    d->layout();
}

qreal QQuickIconLabel::spacing() const
{
    Q_D(const QQuickIconLabel);
    return d->spacing;
}

void QQuickIconLabel::setSpacing(qreal spacing)
{
    Q_D(QQuickIconLabel);
    if (qFuzzyCompare(d->spacing, spacing))
        return;

    d->spacing = spacing;
    d->updateImplicitSize();
    d->layout();
}

bool QQuickIconLabel::isMirrored() const
{
    Q_D(const QQuickIconLabel);
    return d->mirrored;
}

void QQuickIconLabel::setMirrored(bool mirrored)
{
    Q_D(QQuickIconLabel);
    if (d->mirrored == mirrored)
        return;

    d->mirrored = mirrored;
    d->updateImplicitSize();
    d->layout();
}

qreal QQuickIconLabel::topPadding() const
{
    Q_D(const QQuickIconLabel);
    return d->topPadding;
}

void QQuickIconLabel::setTopPadding(qreal padding)
{
    Q_D(QQuickIconLabel);
    if (qFuzzyCompare(d->topPadding, padding))
        return;

    d->topPadding = padding;
    d->updateImplicitSize();
    d->layout();
}

void QQuickIconLabel::resetTopPadding()
{
    setTopPadding(0);
}

qreal QQuickIconLabel::leftPadding() const
{
    Q_D(const QQuickIconLabel);
    return d->leftPadding;
}

void QQuickIconLabel::setLeftPadding(qreal padding)
{
    Q_D(QQuickIconLabel);
    if (qFuzzyCompare(d->leftPadding, padding))
        return;

    d->leftPadding = padding;
    d->updateImplicitSize();
    d->layout();
}

void QQuickIconLabel::resetLeftPadding()
{
    setLeftPadding(0);
}

qreal QQuickIconLabel::rightPadding() const
{
    Q_D(const QQuickIconLabel);
    return d->rightPadding;
}

void QQuickIconLabel::setRightPadding(qreal padding)
{
    Q_D(QQuickIconLabel);
    if (qFuzzyCompare(d->rightPadding, padding))
        return;

    d->rightPadding = padding;
    d->updateImplicitSize();
    d->layout();
}

void QQuickIconLabel::resetRightPadding()
{
    setRightPadding(0);
}

qreal QQuickIconLabel::bottomPadding() const
{
    Q_D(const QQuickIconLabel);
    return d->bottomPadding;
}

void QQuickIconLabel::setBottomPadding(qreal padding)
{
    Q_D(QQuickIconLabel);
    if (qFuzzyCompare(d->bottomPadding, padding))
        return;

    d->bottomPadding = padding;
    d->updateImplicitSize();
    d->layout();
}

void QQuickIconLabel::resetBottomPadding()
{
    setBottomPadding(0);
}

void QQuickIconLabel::componentComplete()
{
    Q_D(QQuickIconLabel);
    QQuickItem::componentComplete();
    d->updateImplicitSize();
    d->layout();
}

void QQuickIconLabel::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickIconLabel);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    d->layout();
}

QT_END_NAMESPACE
