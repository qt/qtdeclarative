// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickabstractcolorpicker_p_p.h"

#include "qquickcolordialogutils_p.h"

#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <QtQuickTemplates2/private/qquickdeferredexecute_p_p.h>

#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>

QQuickAbstractColorPickerPrivate::QQuickAbstractColorPickerPrivate() = default;

bool QQuickAbstractColorPickerPrivate::handlePress(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickAbstractColorPicker);
    QQuickControlPrivate::handlePress(point, timestamp);
    m_pressPoint = point;
    q->setPressed(true);
    q->updateColor(point);
    return true;
}

bool QQuickAbstractColorPickerPrivate::handleMove(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickAbstractColorPicker);
    QQuickControlPrivate::handleMove(point, timestamp);
    if (point != m_pressPoint)
        q->updateColor(point);
    return true;
}

bool QQuickAbstractColorPickerPrivate::handleRelease(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickAbstractColorPicker);
    QQuickControlPrivate::handleRelease(point, timestamp);
    m_pressPoint = QPointF();
    q->setKeepMouseGrab(false);
    q->setKeepTouchGrab(false);
    q->setPressed(false);
    q->updateColor(point);
    return true;
}

void QQuickAbstractColorPickerPrivate::handleUngrab()
{
    Q_Q(QQuickAbstractColorPicker);
    QQuickControlPrivate::handleUngrab();
    m_pressPoint = QPointF();
    q->setPressed(false);
}

void QQuickAbstractColorPickerPrivate::cancelHandle()
{
    Q_Q(QQuickAbstractColorPicker);
    quickCancelDeferred(q, handleName());
}

void QQuickAbstractColorPickerPrivate::executeHandle(bool complete)
{
    Q_Q(QQuickAbstractColorPicker);
    if (m_handle.wasExecuted())
        return;

    if (!m_handle || complete)
        quickBeginDeferred(q, handleName(), m_handle);
    if (complete)
        quickCompleteDeferred(q, handleName(), m_handle);
}

void QQuickAbstractColorPickerPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    Q_Q(QQuickAbstractColorPicker);
    QQuickControlPrivate::itemImplicitWidthChanged(item);
    if (item == m_handle)
        emit q->implicitHandleWidthChanged();
}

void QQuickAbstractColorPickerPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    Q_Q(QQuickAbstractColorPicker);
    QQuickControlPrivate::itemImplicitHeightChanged(item);
    if (item == m_handle)
        emit q->implicitHandleHeightChanged();
}

QQuickAbstractColorPicker::QQuickAbstractColorPicker(QQuickAbstractColorPickerPrivate &dd,
                                                     QQuickItem *parent)
    : QQuickControl(dd, parent)
{
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

QQuickAbstractColorPicker::~QQuickAbstractColorPicker()
{
    Q_D(QQuickAbstractColorPicker);
    if (d->m_handle)
        d->removeImplicitSizeListener(d->m_handle);
}

QColor QQuickAbstractColorPicker::color() const
{
    Q_D(const QQuickAbstractColorPicker);
    return d->m_hsl ? QColor::fromHslF(d->m_hsva.h, d->m_hsva.s, d->m_hsva.l, d->m_hsva.a)
                    : QColor::fromHsvF(d->m_hsva.h, d->m_hsva.s, d->m_hsva.v, d->m_hsva.a);
}

void QQuickAbstractColorPicker::setColor(const QColor &c)
{
    Q_D(QQuickAbstractColorPicker);
    // QColor represents a theoretical color, rather than simply an rgba value.
    // Therefore, two QColor objects can be different,
    // and yet translate to the same rgba value.
    // Since the color picker can reuse the same rgba value for multiple pixels,
    // we should not return early if the rgba() values are equal,
    // but only if the QColor objects are exactly the same.

    if (color() == c)
        return;

    // When called from QQuickColorDialogImpl, it might not have the same spec as the current color
    // picker.
    if (d->m_hsl && c.spec() == QColor::Spec::Hsv) {
        const auto sl = getSaturationAndLightness(c.hsvSaturationF(), c.valueF());
        d->m_hsva.h = qBound(.0, c.hsvHueF(), 1.0);
        d->m_hsva.s = qBound(.0, sl.first, 1.0);
        d->m_hsva.l = qBound(.0, sl.second, 1.0);
    } else if (!d->m_hsl && c.spec() == QColor::Spec::Hsl) {
        const auto sv = getSaturationAndValue(c.hslSaturationF(), c.lightnessF());
        d->m_hsva.h = qBound(.0, c.hslHueF(), 1.0);
        d->m_hsva.s = qBound(.0, sv.first, 1.0);
        d->m_hsva.v = qBound(.0, sv.second, 1.0);
    } else {
        d->m_hsva.h = qBound(.0, d->m_hsl ? c.hslHueF() : c.hsvHueF(), 1.0);
        d->m_hsva.s = qBound(.0, d->m_hsl ? c.hslSaturationF() : c.hsvSaturationF(), 1.0);
        d->m_hsva.v = qBound(.0, d->m_hsl ? c.lightnessF() : c.valueF(), 1.0);
    }

    d->m_hsva.a = qBound(.0, c.alphaF(), 1.0);

    emit colorChanged(color());
}

qreal QQuickAbstractColorPicker::alpha() const
{
    Q_D(const QQuickAbstractColorPicker);
    return d->m_hsva.a;
}

void QQuickAbstractColorPicker::setAlpha(qreal alpha)
{
    Q_D(QQuickAbstractColorPicker);

    if (!qt_is_finite(alpha))
        return;

    alpha = qBound(.0, alpha, 1.0);

    if (qFuzzyCompare(d->m_hsva.a, alpha))
        return;

    d->m_hsva.a = alpha;

    emit colorChanged(color());
}

qreal QQuickAbstractColorPicker::hue() const
{
    Q_D(const QQuickAbstractColorPicker);
    return d->m_hsva.h;
}
void QQuickAbstractColorPicker::setHue(qreal hue)
{
    Q_D(QQuickAbstractColorPicker);

    if (!qt_is_finite(hue))
        return;

    d->m_hsva.h = hue;

    emit colorChanged(color());
}

qreal QQuickAbstractColorPicker::saturation() const
{
    Q_D(const QQuickAbstractColorPicker);
    return d->m_hsva.s;
}

void QQuickAbstractColorPicker::setSaturation(qreal saturation)
{
    Q_D(QQuickAbstractColorPicker);
    if (!qt_is_finite(saturation))
        return;

    d->m_hsva.s = saturation;

    emit colorChanged(color());
}
qreal QQuickAbstractColorPicker::value() const
{
    Q_D(const QQuickAbstractColorPicker);
    return d->m_hsl ? getSaturationAndValue(d->m_hsva.s, d->m_hsva.l).second : d->m_hsva.v;
}
void QQuickAbstractColorPicker::setValue(qreal value)
{
    Q_D(QQuickAbstractColorPicker);
    if (!qt_is_finite(value))
        return;

    const auto sv = d->m_hsl ? getSaturationAndValue(d->m_hsva.s, d->m_hsva.l)
                       : std::pair<qreal, qreal>(d->m_hsva.s, value);
    d->m_hsva.s = sv.first;
    d->m_hsva.v = sv.second;

    emit colorChanged(color());
}

qreal QQuickAbstractColorPicker::lightness() const
{
    Q_D(const QQuickAbstractColorPicker);
    return d->m_hsl ? d->m_hsva.l : getSaturationAndLightness(d->m_hsva.s, d->m_hsva.v).second;
}
void QQuickAbstractColorPicker::setLightness(qreal lightness)
{
    Q_D(QQuickAbstractColorPicker);
    if (!qt_is_finite(lightness))
        return;

    const auto sl = !d->m_hsl ? getSaturationAndLightness(d->m_hsva.s, d->m_hsva.v)
                        : std::pair<qreal, qreal>(d->m_hsva.s, lightness);
    d->m_hsva.s = sl.first;
    d->m_hsva.l = sl.second;

    emit colorChanged(color());
}

/*!
    \internal

    This property holds whether the slider is pressed.
*/
bool QQuickAbstractColorPicker::isPressed() const
{
    Q_D(const QQuickAbstractColorPicker);
    return d->m_pressed;
}

void QQuickAbstractColorPicker::setPressed(bool pressed)
{
    Q_D(QQuickAbstractColorPicker);
    if (pressed == d->m_pressed)
        return;

    d->m_pressed = pressed;
    emit pressedChanged();
}

/*!
    \internal

    This property holds the handle item.
*/
QQuickItem *QQuickAbstractColorPicker::handle() const
{
    QQuickAbstractColorPickerPrivate *d = const_cast<QQuickAbstractColorPickerPrivate *>(d_func());
    if (!d->m_handle)
        d->executeHandle();
    return d->m_handle;
}

void QQuickAbstractColorPicker::setHandle(QQuickItem *handle)
{
    Q_D(QQuickAbstractColorPicker);
    if (handle == d->m_handle)
        return;

    if (!d->m_handle.isExecuting())
        d->cancelHandle();

    const qreal oldImplicitHandleWidth = implicitHandleWidth();
    const qreal oldImplicitHandleHeight = implicitHandleHeight();

    d->removeImplicitSizeListener(d->m_handle);
    QQuickControlPrivate::hideOldItem(d->m_handle);
    d->m_handle = handle;

    if (handle) {
        if (!handle->parentItem())
            handle->setParentItem(this);
        d->addImplicitSizeListener(handle);
    }

    if (!qFuzzyCompare(oldImplicitHandleWidth, implicitHandleWidth()))
        emit implicitHandleWidthChanged();
    if (!qFuzzyCompare(oldImplicitHandleHeight, implicitHandleHeight()))
        emit implicitHandleHeightChanged();
    if (!d->m_handle.isExecuting())
        emit handleChanged();
}

/*!
    \internal
    \readonly

    This property holds the implicit handle width.

    The value is equal to \c {handle ? handle.implicitWidth : 0}.

    This is typically used, together with \l {Control::}{implicitContentWidth} and
    \l {Control::}{implicitBackgroundWidth}, to calculate the \l {Item::}{implicitWidth}.

    \sa implicitHandleHeight
*/
qreal QQuickAbstractColorPicker::implicitHandleWidth() const
{
    Q_D(const QQuickAbstractColorPicker);
    if (!d->m_handle)
        return 0;
    return d->m_handle->implicitWidth();
}

/*!
    \internal
    \readonly

    This property holds the implicit handle height.

    The value is equal to \c {handle ? handle.implicitHeight : 0}.

    This is typically used, together with \l {Control::}{implicitContentHeight} and
    \l {Control::}{implicitBackgroundHeight}, to calculate the \l {Item::}{implicitHeight}.

    \sa implicitHandleWidth
*/
qreal QQuickAbstractColorPicker::implicitHandleHeight() const
{
    Q_D(const QQuickAbstractColorPicker);
    if (!d->m_handle)
        return 0;
    return d->m_handle->implicitHeight();
}

void QQuickAbstractColorPicker::componentComplete()
{
    Q_D(QQuickAbstractColorPicker);
    d->executeHandle(true);
    QQuickControl::componentComplete();
}

void QQuickAbstractColorPicker::updateColor(const QPointF &pos)
{
    QColor c = colorAt(pos);
    c.setAlphaF(alpha());
    setColor(c);

    emit colorPicked(c);
}
