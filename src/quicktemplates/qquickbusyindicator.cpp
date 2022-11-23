// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickbusyindicator_p.h"
#include "qquickcontrol_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype BusyIndicator
    \inherits Control
//!     \instantiates QQuickBusyIndicator
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-indicators
    \brief Indicates background activity, for example, while content is being loaded.

    \image qtquickcontrols-busyindicator.gif

    The busy indicator should be used to indicate activity while content is
    being loaded or the UI is blocked waiting for a resource to become available.

    The following snippet shows how to use the BusyIndicator:

    \qml
    BusyIndicator {
        running: image.status === Image.Loading
    }
    \endqml

    BusyIndicator is similar to an indeterminate \l ProgressBar. Both can be
    used to indicate background activity. The main difference is visual, and
    that ProgressBar can also present a concrete amount of progress (when it
    can be determined). Due to the visual difference, busy indicators and
    indeterminate progress bars fit different places in user interfaces.
    Typical places for a busy indicator:
    \list
    \li in the corner of a \l ToolBar
    \li as an overlay on top of a \l Page
    \li on the side of an \l ItemDelegate
    \endlist

    \sa {Customizing BusyIndicator}, {Indicator Controls}, ProgressBar
*/

class QQuickBusyIndicatorPrivate : public QQuickControlPrivate
{
public:
    bool running = true;
};

QQuickBusyIndicator::QQuickBusyIndicator(QQuickItem *parent)
    : QQuickControl(*(new QQuickBusyIndicatorPrivate), parent)
{
}

/*!
    \qmlproperty bool QtQuick.Controls::BusyIndicator::running

    This property holds whether the busy indicator is currently indicating
    activity.

    \note The indicator is only visible when this property is set to \c true.

    The default value is \c true.
*/
bool QQuickBusyIndicator::isRunning() const
{
    Q_D(const QQuickBusyIndicator);
    return d->running;
}

void QQuickBusyIndicator::setRunning(bool running)
{
    Q_D(QQuickBusyIndicator);
    if (d->running == running)
        return;

    d->running = running;
    emit runningChanged();
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickBusyIndicator::touchEvent(QTouchEvent *event)
{
    event->ignore(); // QTBUG-61785
}
#endif

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickBusyIndicator::accessibleRole() const
{
    return QAccessible::Indicator;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickbusyindicator_p.cpp"
