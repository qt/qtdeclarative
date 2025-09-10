// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickswitchdelegate_p.h"

#include "qquickitemdelegate_p_p.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SwitchDelegate
    \inherits ItemDelegate
//!     \nativetype QQuickSwitchDelegate
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-delegates
    \brief Item delegate with a switch indicator that can be toggled on or off.

    \image qtquickcontrols-switchdelegate.gif

    SwitchDelegate presents an item delegate that can be toggled on (checked) or
    off (unchecked). Switch delegates are typically used to select one or more
    options from a set of options. For smaller sets of options, or for options
    that need to be uniquely identifiable, consider using \l Switch instead.

    SwitchDelegate inherits its API from \l ItemDelegate, which is inherited
    from \l AbstractButton. For instance, you can set \l {AbstractButton::text}{text},
    and react to \l {AbstractButton::clicked}{clicks} using the \l AbstractButton
    API. The state of the switch delegate can be set with the
    \l {AbstractButton::}{checked} property.

    \code
    ListView {
        model: ["Option 1", "Option 2", "Option 3"]
        delegate: SwitchDelegate {
            text: modelData
        }
    }
    \endcode

    \sa {Customizing SwitchDelegate}, {Delegate Controls}
*/

class QQuickSwitchDelegatePrivate : public QQuickItemDelegatePrivate
{
    Q_DECLARE_PUBLIC(QQuickSwitchDelegate)

public:
    qreal positionAt(const QPointF &point) const;

    bool canDrag(const QPointF &movePoint) const;
    bool handleMove(const QPointF &point, ulong timestamp) override;
    bool handleRelease(const QPointF &point, ulong timestamp) override;

    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::ListView); }

    qreal position = 0;
};

qreal QQuickSwitchDelegatePrivate::positionAt(const QPointF &point) const
{
    Q_Q(const QQuickSwitchDelegate);
    qreal pos = 0.0;
    if (indicator)
        pos = indicator->mapFromItem(q, point).x() / indicator->width();
    if (q->isMirrored())
        return 1.0 - pos;
    return pos;
}

bool QQuickSwitchDelegatePrivate::canDrag(const QPointF &movePoint) const
{
    // don't start dragging the handle unless the initial press was at the indicator,
    // or the drag has reached the indicator area. this prevents unnatural jumps when
    // dragging far outside the indicator.
    const qreal pressPos = positionAt(pressPoint);
    const qreal movePos = positionAt(movePoint);
    return (pressPos >= 0.0 && pressPos <= 1.0) || (movePos >= 0.0 && movePos <= 1.0);
}

bool QQuickSwitchDelegatePrivate::handleMove(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickSwitchDelegate);
    QQuickItemDelegatePrivate::handleMove(point, timestamp);
    if (q->keepMouseGrab() || q->keepTouchGrab())
        q->setPosition(positionAt(point));
    return true;
}

bool QQuickSwitchDelegatePrivate::handleRelease(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickSwitchDelegate);
    QQuickItemDelegatePrivate::handleRelease(point, timestamp);
    q->setKeepMouseGrab(false);
    q->setKeepTouchGrab(false);
    return true;
}

QQuickSwitchDelegate::QQuickSwitchDelegate(QQuickItem *parent)
    : QQuickItemDelegate(*(new QQuickSwitchDelegatePrivate), parent)
{
    Q_D(QQuickSwitchDelegate);
    d->keepPressed = true;
    setCheckable(true);
}

/*!
    \qmlproperty real QtQuick.Controls::SwitchDelegate::position
    \readonly

    \input includes/qquickswitch.qdocinc position
*/
qreal QQuickSwitchDelegate::position() const
{
    Q_D(const QQuickSwitchDelegate);
    return d->position;
}

void QQuickSwitchDelegate::setPosition(qreal position)
{
    Q_D(QQuickSwitchDelegate);
    position = std::clamp(position, qreal(0.0),  qreal(1.0));
    if (qFuzzyCompare(d->position, position))
        return;

    d->position = position;
    emit positionChanged();
    emit visualPositionChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::SwitchDelegate::visualPosition
    \readonly

    \input includes/qquickswitch.qdocinc visualPosition
*/
qreal QQuickSwitchDelegate::visualPosition() const
{
    Q_D(const QQuickSwitchDelegate);
    if (isMirrored())
        return 1.0 - d->position;
    return d->position;
}

void QQuickSwitchDelegate::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickSwitchDelegate);
    if (!keepMouseGrab()) {
        const QPointF movePoint = event->position();
        if (d->canDrag(movePoint))
            setKeepMouseGrab(QQuickWindowPrivate::dragOverThreshold(movePoint.x() - d->pressPoint.x(), Qt::XAxis, event));
    }
    QQuickItemDelegate::mouseMoveEvent(event);
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickSwitchDelegate::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickSwitchDelegate);
    if (!keepTouchGrab() && event->type() == QEvent::TouchUpdate) {
        for (const QTouchEvent::TouchPoint &point : event->points()) {
            if (point.id() != d->touchId || point.state() != QEventPoint::Updated)
                continue;
            if (d->canDrag(point.position()))
                setKeepTouchGrab(QQuickWindowPrivate::dragOverThreshold(point.position().x() - d->pressPoint.x(), Qt::XAxis, &point));
        }
    }
    QQuickItemDelegate::touchEvent(event);
}
#endif

QFont QQuickSwitchDelegate::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::ListView);
}

void QQuickSwitchDelegate::mirrorChange()
{
    QQuickItemDelegate::mirrorChange();
    emit visualPositionChanged();
}

void QQuickSwitchDelegate::nextCheckState()
{
    Q_D(QQuickSwitchDelegate);
    if (keepMouseGrab() || keepTouchGrab()) {
        d->toggle(d->position > 0.5);
        // the checked state might not change => force a position update to
        // avoid that the handle is left somewhere in the middle (QTBUG-57944)
        setPosition(d->checked ? 1.0 : 0.0);
    } else {
        QQuickItemDelegate::nextCheckState();
    }
}

void QQuickSwitchDelegate::buttonChange(ButtonChange change)
{
    Q_D(QQuickSwitchDelegate);
    if (change == ButtonCheckedChange)
        setPosition(d->checked ? 1.0 : 0.0);
    else
        QQuickAbstractButton::buttonChange(change);
}

QT_END_NAMESPACE

#include "moc_qquickswitchdelegate_p.cpp"
