// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickswitch_p.h"
#include "qquickabstractbutton_p_p.h"

#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickevents_p_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Switch
    \inherits AbstractButton
//!     \nativetype QQuickSwitch
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-buttons
    \brief Switch button that can be toggled on or off.

    \image qtquickcontrols-switch.gif

    Switch is an option button that can be dragged or toggled on (checked) or
    off (unchecked). Switches are typically used to select between two states.
    For larger sets of options, such as those in a list, consider using
    \l SwitchDelegate instead.

    Switch inherits its API from \l AbstractButton. For instance, the state
    of the switch can be set with the \l {AbstractButton::}{checked} property.
    The \l clicked and \l toggled signals are emitted when the switch is
    interactively clicked by the user via touch, mouse, or keyboard.

    \code
    ColumnLayout {
        Switch {
            text: qsTr("Wi-Fi")
            checked: Networking.wifiEnabled
            onClicked: Networking.wifiEnabled = checked
        }
        Switch {
            text: qsTr("Bluetooth")
            checked: Networking.bluetoothEnabled
            onClicked: Networking.bluetoothEnabled = checked
        }
    }
    \endcode

    \sa {Customizing Switch}, {Button Controls}
*/

class QQuickSwitchPrivate : public QQuickAbstractButtonPrivate
{
    Q_DECLARE_PUBLIC(QQuickSwitch)

public:
    qreal positionAt(const QPointF &point) const;

    bool canDrag(const QPointF &movePoint) const;
    bool handleMove(const QPointF &point, ulong timestamp) override;
    bool handleRelease(const QPointF &point, ulong timestamp) override;

    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::Switch); }

    qreal position = 0;
};

qreal QQuickSwitchPrivate::positionAt(const QPointF &point) const
{
    Q_Q(const QQuickSwitch);
    qreal pos = 0.0;
    if (indicator)
        pos = indicator->mapFromItem(q, point).x() / indicator->width();
    if (q->isMirrored())
        return 1.0 - pos;
    return pos;
}

bool QQuickSwitchPrivate::canDrag(const QPointF &movePoint) const
{
    // don't start dragging the handle unless the initial press was at the indicator,
    // or the drag has reached the indicator area. this prevents unnatural jumps when
    // dragging far outside the indicator.
    const qreal pressPos = positionAt(pressPoint);
    const qreal movePos = positionAt(movePoint);
    return (pressPos >= 0.0 && pressPos <= 1.0) || (movePos >= 0.0 && movePos <= 1.0);
}

bool QQuickSwitchPrivate::handleMove(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickSwitch);
    QQuickAbstractButtonPrivate::handleMove(point, timestamp);
    if (q->keepMouseGrab() || q->keepTouchGrab())
        q->setPosition(positionAt(point));
    return true;
}

bool QQuickSwitchPrivate::handleRelease(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickSwitch);
    QQuickAbstractButtonPrivate::handleRelease(point, timestamp);
    q->setKeepMouseGrab(false);
    q->setKeepTouchGrab(false);
    return true;
}

QQuickSwitch::QQuickSwitch(QQuickItem *parent)
    : QQuickAbstractButton(*(new QQuickSwitchPrivate), parent)
{
    Q_D(QQuickSwitch);
    d->keepPressed = true;
    setCheckable(true);
}

/*!
    \qmlproperty real QtQuick.Controls::Switch::position
    \readonly

    \input includes/qquickswitch.qdocinc position
*/
qreal QQuickSwitch::position() const
{
    Q_D(const QQuickSwitch);
    return d->position;
}

void QQuickSwitch::setPosition(qreal position)
{
    Q_D(QQuickSwitch);
    position = std::clamp(position, qreal(0.0), qreal(1.0));
    if (qFuzzyCompare(d->position, position))
        return;

    d->position = position;
    emit positionChanged();
    emit visualPositionChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::Switch::visualPosition
    \readonly

    \input includes/qquickswitch.qdocinc visualPosition
*/
qreal QQuickSwitch::visualPosition() const
{
    Q_D(const QQuickSwitch);
    if (isMirrored())
        return 1.0 - d->position;
    return d->position;
}

void QQuickSwitch::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickSwitch);
    if (!keepMouseGrab()) {
        const QPointF movePoint = event->position();
        if (d->canDrag(movePoint))
            setKeepMouseGrab(QQuickWindowPrivate::dragOverThreshold(movePoint.x() - d->pressPoint.x(), Qt::XAxis, event));
    }
    QQuickAbstractButton::mouseMoveEvent(event);
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickSwitch::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickSwitch);
    if (!keepTouchGrab() && event->type() == QEvent::TouchUpdate) {
        for (const QTouchEvent::TouchPoint &point : event->points()) {
            if (point.id() != d->touchId || point.state() != QEventPoint::Updated)
                continue;
            if (d->canDrag(point.position()))
                setKeepTouchGrab(QQuickWindowPrivate::dragOverThreshold(point.position().x() - d->pressPoint.x(), Qt::XAxis, &point));
        }
    }
    QQuickAbstractButton::touchEvent(event);
}
#endif

void QQuickSwitch::mirrorChange()
{
    QQuickAbstractButton::mirrorChange();
    emit visualPositionChanged();
}

void QQuickSwitch::nextCheckState()
{
    Q_D(QQuickSwitch);
    if (keepMouseGrab() || keepTouchGrab()) {
        d->toggle(d->position > 0.5);
        // the checked state might not change => force a position update to
        // avoid that the handle is left somewhere in the middle (QTBUG-57944)
        setPosition(d->checked ? 1.0 : 0.0);
    } else {
        QQuickAbstractButton::nextCheckState();
    }
}

void QQuickSwitch::buttonChange(ButtonChange change)
{
    Q_D(QQuickSwitch);
    if (change == ButtonCheckedChange)
        setPosition(d->checked ? 1.0 : 0.0);
    else
        QQuickAbstractButton::buttonChange(change);
}

QFont QQuickSwitch::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::Switch);
}

QT_END_NAMESPACE

#include "moc_qquickswitch_p.cpp"
