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

QT_BEGIN_NAMESPACE

/*!
    \qmltype Control
    \inherits Item
    \instantiates QQuickControl
    \inqmlmodule QtQuick.Controls
    \brief A user interface control.

    Control is the base type of user interface controls.
*/

QQuickControlPrivate::QQuickControlPrivate() :
    hasTopPadding(false), hasLeftPadding(false), hasRightPadding(false), hasBottomPadding(false),
    padding(0), topPadding(0), leftPadding(0), rightPadding(0), bottomPadding(0),
    layoutDirection(Qt::LeftToRight), background(Q_NULLPTR)
{
}

void QQuickControlPrivate::mirrorChange()
{
    Q_Q(QQuickControl);
    q->mirrorChange();
}

void QQuickControlPrivate::setTopPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    qreal oldPadding = q->topPadding();
    topPadding = value;
    hasTopPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding))) {
        emit q->topPaddingChanged();
        emit q->availableHeightChanged();
        q->paddingChange(QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding),
                         QMarginsF(leftPadding, oldPadding, rightPadding, bottomPadding));
    }
}

void QQuickControlPrivate::setLeftPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    qreal oldPadding = q->leftPadding();
    leftPadding = value;
    hasLeftPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding))) {
        emit q->leftPaddingChanged();
        emit q->availableWidthChanged();
        q->paddingChange(QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding),
                         QMarginsF(oldPadding, topPadding, rightPadding, bottomPadding));
    }
}

void QQuickControlPrivate::setRightPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    qreal oldPadding = q->rightPadding();
    rightPadding = value;
    hasRightPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding))) {
        emit q->rightPaddingChanged();
        emit q->availableWidthChanged();
        q->paddingChange(QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding),
                         QMarginsF(leftPadding, topPadding, oldPadding, bottomPadding));
    }
}

void QQuickControlPrivate::setBottomPadding(qreal value, bool reset)
{
    Q_Q(QQuickControl);
    qreal oldPadding = q->bottomPadding();
    bottomPadding = value;
    hasBottomPadding = !reset;
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding))) {
        emit q->bottomPaddingChanged();
        emit q->availableHeightChanged();
        q->paddingChange(QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding),
                         QMarginsF(leftPadding, topPadding, rightPadding, oldPadding));
    }
}

void QQuickControlPrivate::resizeBackground()
{
    Q_Q(QQuickControl);
    if (background) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(background);
        if (!p->widthValid && qFuzzyIsNull(background->x())) {
            background->setWidth(q->width());
            p->widthValid = false;
        }
        if (!p->heightValid && qFuzzyIsNull(background->y())) {
            background->setHeight(q->height());
            p->heightValid = false;
        }
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
    \qmlproperty real QtQuickControls2::Control::availableWidth

    This property holds the width available after deducting horizontal padding.

    \sa padding, leftPadding, rightPadding
*/
qreal QQuickControl::availableWidth() const
{
    return width() - leftPadding() - rightPadding();
}

/*!
    \qmlproperty real QtQuickControls2::Control::availableHeight

    This property holds the height available after deducting vertical padding.

    \sa padding, topPadding, bottomPadding
*/
qreal QQuickControl::availableHeight() const
{
    return height() - topPadding() - bottomPadding();
}

/*!
    \qmlproperty real QtQuickControls2::Control::padding

    This property holds the default padding.

    \sa availableWidth, availableHeight, topPadding, leftPadding, rightPadding, bottomPadding
*/
qreal QQuickControl::padding() const
{
    Q_D(const QQuickControl);
    return d->padding;
}

void QQuickControl::setPadding(qreal padding)
{
    Q_D(QQuickControl);
    if (qFuzzyCompare(d->padding, padding))
        return;
    QMarginsF oldPadding(leftPadding(), topPadding(), rightPadding(), bottomPadding());
    d->padding = padding;
    emit paddingChanged();
    QMarginsF newPadding(leftPadding(), topPadding(), rightPadding(), bottomPadding());
    if (!qFuzzyCompare(newPadding.top(), oldPadding.top()))
        emit topPaddingChanged();
    if (!qFuzzyCompare(newPadding.left(), oldPadding.left()))
        emit leftPaddingChanged();
    if (!qFuzzyCompare(newPadding.right(), oldPadding.right()))
        emit rightPaddingChanged();
    if (!qFuzzyCompare(newPadding.bottom(), oldPadding.bottom()))
        emit bottomPaddingChanged();
    if (!qFuzzyCompare(newPadding.top(), oldPadding.top()) || !qFuzzyCompare(newPadding.bottom(), oldPadding.bottom()))
        emit availableHeightChanged();
    if (!qFuzzyCompare(newPadding.left(), oldPadding.left()) || !qFuzzyCompare(newPadding.right(), oldPadding.right()))
        emit availableWidthChanged();
    paddingChange(newPadding, oldPadding);
}

void QQuickControl::resetPadding()
{
    setPadding(0);
}

/*!
    \qmlproperty real QtQuickControls2::Control::topPadding

    This property holds the top padding.

    \sa padding, bottomPadding, availableHeight
*/
qreal QQuickControl::topPadding() const
{
    Q_D(const QQuickControl);
    if (d->hasTopPadding)
        return d->topPadding;
    return d->padding;
}

void QQuickControl::setTopPadding(qreal padding)
{
    Q_D(QQuickControl);
    d->setTopPadding(padding);
}

void QQuickControl::resetTopPadding()
{
    Q_D(QQuickControl);
    d->setTopPadding(0, true);
}

/*!
    \qmlproperty real QtQuickControls2::Control::leftPadding

    This property holds the left padding.

    \sa padding, rightPadding, availableWidth
*/
qreal QQuickControl::leftPadding() const
{
    Q_D(const QQuickControl);
    if (d->hasLeftPadding)
        return d->leftPadding;
    return d->padding;
}

void QQuickControl::setLeftPadding(qreal padding)
{
    Q_D(QQuickControl);
    d->setLeftPadding(padding);
}

void QQuickControl::resetLeftPadding()
{
    Q_D(QQuickControl);
    d->setLeftPadding(0, true);
}

/*!
    \qmlproperty real QtQuickControls2::Control::rightPadding

    This property holds the right padding.

    \sa padding, leftPadding, availableWidth
*/
qreal QQuickControl::rightPadding() const
{
    Q_D(const QQuickControl);
    if (d->hasRightPadding)
        return d->rightPadding;
    return d->padding;
}

void QQuickControl::setRightPadding(qreal padding)
{
    Q_D(QQuickControl);
    d->setRightPadding(padding);
}

void QQuickControl::resetRightPadding()
{
    Q_D(QQuickControl);
    d->setRightPadding(0, true);
}

/*!
    \qmlproperty real QtQuickControls2::Control::bottomPadding

    This property holds the bottom padding.

    \sa padding, topPadding, availableHeight
*/
qreal QQuickControl::bottomPadding() const
{
    Q_D(const QQuickControl);
    if (d->hasBottomPadding)
        return d->bottomPadding;
    return d->padding;
}

void QQuickControl::setBottomPadding(qreal padding)
{
    Q_D(QQuickControl);
    d->setBottomPadding(padding);
}

void QQuickControl::resetBottomPadding()
{
    Q_D(QQuickControl);
    d->setBottomPadding(0, true);
}

/*!
    \qmlproperty enumeration QtQuickControls2::Control::layoutDirection

    This property holds the layout direction of the control.

    Possible values:
    \list
    \li Qt.LeftToRight (default) - Items are laid out from left to right. If the width of the row is explicitly set,
    the left anchor remains to the left of the row.
    \li Qt.RightToLeft - Items are laid out from right to left. If the width of the row is explicitly set,
    the right anchor remains to the right of the row.
    \endlist

    \sa effectiveLayoutDirection
*/
Qt::LayoutDirection QQuickControl::layoutDirection() const
{
    Q_D(const QQuickControl);
    return d->layoutDirection;
}

/*!
    \qmlproperty enumeration QtQuickControls2::Control::effectiveLayoutDirection
    \readonly

    This property holds the effective layout direction of the control.

    When using the attached property \l {LayoutMirroring::enabled}{LayoutMirroring::enabled}
    for locale layouts, the visual layout direction of the control will be mirrored. However,
    the \l layoutDirection property will remain unchanged.

    \sa layoutDirection, {LayoutMirroring}{LayoutMirroring}
*/
Qt::LayoutDirection QQuickControl::effectiveLayoutDirection() const
{
    Q_D(const QQuickControl);
    if (d->isMirrored())
        return d->layoutDirection == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft;
    return d->layoutDirection;
}

void QQuickControl::setLayoutDirection(Qt::LayoutDirection direction)
{
    Q_D(QQuickControl);
    if (d->layoutDirection != direction) {
        d->layoutDirection = direction;
        emit layoutDirectionChanged();
        mirrorChange();
    }
}

/*!
    \qmlproperty bool QtQuickControls2::Control::mirrored
    \readonly

    This property holds whether the control is mirrored.

    This property is provided for convenience. A control is considered mirrored
    when its visual layout direction is right-to-left.

    \sa effectiveLayoutDirection, {LayoutMirroring}{LayoutMirroring}
*/
bool QQuickControl::isMirrored() const
{
    return effectiveLayoutDirection() == Qt::RightToLeft;
}

/*!
    \qmlproperty Item QtQuickControls2::Control::background

    This property holds the background item.

    \note If the background item has no explicit size specified, it automatically
          follows the control's size. In most cases, there is no need to specify
          width or height for a background item.
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
            if (isComponentComplete())
                d->resizeBackground();
        }
        emit backgroundChanged();
    }
}

void QQuickControl::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickControl);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    d->resizeBackground();
    if (newGeometry.width() != oldGeometry.width())
        emit availableWidthChanged();
    if (newGeometry.width() != oldGeometry.height())
        emit availableHeightChanged();
}

void QQuickControl::mirrorChange()
{
    emit effectiveLayoutDirectionChanged();
    emit mirroredChanged();
}

void QQuickControl::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    Q_UNUSED(newPadding);
    Q_UNUSED(oldPadding);
}

QT_END_NAMESPACE
