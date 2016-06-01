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

#include "qquicktooltip_p.h"
#include "qquickpopup_p_p.h"
#include "qquickcontrol_p_p.h"

#include <QtCore/qbasictimer.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickwindow.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ToolTip
    \inherits Popup
    \instantiates QQuickToolTip
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-popups
    \brief Provides tool tips for any control.

    A tool tip is a short piece of text that informs the user of a control's
    function. It is typically placed above or below the parent control. The
    tip text can be any \l{Rich Text Processing}{rich text} formatted string.

    \image qtquickcontrols2-tooltip.png

    The most straight-forward way to setup tool tips for controls is to
    specify \l text and \l {visible}{visibility} via attached properties.
    The following example illustrates this approach:

    \snippet qtquickcontrols2-tooltip.qml 1

    Under normal circumstances, there is only one tool tip visible at a time.
    In order to save resources, all items that use the ToolTip attached property
    share the same visual tool tip label instance. Even though the visuals are
    shared, \c text, \c timeout and \c delay are stored individually for each item
    that uses the respective attached property. However, multiple items cannot
    make the shared tool tip visible at the same time. The shared tool tip is only
    shown for the last item that made it visible. The position of the shared tool
    tip is determined by the framework.

    \section2 Delay and Timeout

    Tool tips are typically transient in a sense that they are shown as a
    result of a certain external event or user interaction, and they usually
    hide after a certain timeout. It is possible to control the delay when
    a tool tip is shown, and the timeout when it is hidden. This makes it
    possible to implement varying strategies for showing and hiding tool tips.

    For example, on touch screens, it is a common pattern to show a tool tip
    as a result of pressing and holding down a button. The following example
    demonstrates how to delay showing a tool tip until the press-and-hold
    interval is reached. In this example, the tool tip hides as soon as the
    button is released.

    \snippet qtquickcontrols2-tooltip-pressandhold.qml 1

    With pointer devices, however, it might be desired to show a tool tip as
    a result of hovering a button for a while. The following example presents
    how to show a tool tip after hovering a button for a second, and hide it
    after a timeout of five seconds.

    \snippet qtquickcontrols2-tooltip-hover.qml 1

    \section2 Custom Tool Tips

    Should one need more fine-grained control over the tool tip position, or
    multiple simultaneous tool tip instances are needed, it is also possible
    to create local tool tip instances. This way, it is possible to
    \l {Customizing ToolTip}{customize} the tool tip, and the whole  \l Popup
    API is available. The following example presents a tool tip that presents
    the value of a slider when the handle is dragged.

    \image qtquickcontrols2-tooltip-slider.png

    \snippet qtquickcontrols2-tooltip-slider.qml 1

    \sa {Customizing ToolTip}, {Popup Controls}
*/

class QQuickToolTipPrivate : public QQuickPopupPrivate
{
    Q_DECLARE_PUBLIC(QQuickToolTip)

public:
    QQuickToolTipPrivate() : delay(0), timeout(-1) { }

    void startDelay();
    void stopDelay();

    void startTimeout();
    void stopTimeout();

    int delay;
    int timeout;
    QString text;
    QBasicTimer delayTimer;
    QBasicTimer timeoutTimer;
};

void QQuickToolTipPrivate::startDelay()
{
    Q_Q(QQuickToolTip);
    if (delay > 0)
        delayTimer.start(delay, q);
}

void QQuickToolTipPrivate::stopDelay()
{
    delayTimer.stop();
}

void QQuickToolTipPrivate::startTimeout()
{
    Q_Q(QQuickToolTip);
    if (timeout > 0)
        timeoutTimer.start(timeout, q);
}

void QQuickToolTipPrivate::stopTimeout()
{
    timeoutTimer.stop();
}

QQuickToolTip::QQuickToolTip(QQuickItem *parent) :
    QQuickPopup(*(new QQuickToolTipPrivate), parent)
{
    Q_D(QQuickToolTip);
    d->allowVerticalFlip = true;
    d->allowHorizontalFlip = true;
}

/*!
    \qmlproperty string QtQuick.Controls::ToolTip::text

    This property holds the text shown on the tool tip.
*/
QString QQuickToolTip::text() const
{
    Q_D(const QQuickToolTip);
    return d->text;
}

void QQuickToolTip::setText(const QString &text)
{
    Q_D(QQuickToolTip);
    if (d->text == text)
        return;

    d->text = text;
    emit textChanged();
}

/*!
    \qmlproperty int QtQuick.Controls::ToolTip::delay

    This property holds the delay (milliseconds) after which the tool tip is
    shown. A tooltip with a negative delay is shown immediately. The default
    value is \c 0.
*/
int QQuickToolTip::delay() const
{
    Q_D(const QQuickToolTip);
    return d->delay;
}

void QQuickToolTip::setDelay(int delay)
{
    Q_D(QQuickToolTip);
    if (d->delay == delay)
        return;

    d->delay = delay;
    emit delayChanged();
}

/*!
    \qmlproperty int QtQuick.Controls::ToolTip::timeout

    This property holds the timeout (milliseconds) after which the tool tip is
    hidden. A tooltip with a negative timeout does not hide automatically. The
    default value is \c -1.
*/
int QQuickToolTip::timeout() const
{
    Q_D(const QQuickToolTip);
    return d->timeout;
}

void QQuickToolTip::setTimeout(int timeout)
{
    Q_D(QQuickToolTip);
    if (d->timeout == timeout)
        return;

    if (timeout <= 0)
        d->stopTimeout();
    else if (isVisible())
        d->startTimeout();

    d->timeout = timeout;
    emit timeoutChanged();
}

QQuickToolTipAttached *QQuickToolTip::qmlAttachedProperties(QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item)
        qmlInfo(object) << "ToolTip must be attached to an Item";

    return new QQuickToolTipAttached(item);
}

void QQuickToolTip::open()
{
    Q_D(QQuickToolTip);
    if (d->delay > 0)
        d->startDelay();
    else
        QQuickPopup::open();
}

void QQuickToolTip::close()
{
    Q_D(QQuickToolTip);
    d->stopDelay();
    QQuickPopup::close();
}

QFont QQuickToolTip::defaultFont() const
{
    return QQuickControlPrivate::themeFont(QPlatformTheme::TipLabelFont);
}

void QQuickToolTip::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    Q_D(QQuickToolTip);
    QQuickPopup::itemChange(change, data);
    if (change == QQuickItem::ItemVisibleHasChanged) {
        if (data.boolValue)
            d->startTimeout();
        else
            d->stopTimeout();

        QQuickToolTipAttached *attached = qobject_cast<QQuickToolTipAttached *>(qmlAttachedPropertiesObject<QQuickToolTip>(d->parentItem, false));
        if (attached)
            emit attached->visibleChanged();
    }
}

void QQuickToolTip::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickToolTip);
    if (event->timerId() == d->timeoutTimer.timerId()) {
        d->stopTimeout();
        close();
    } else if (event->timerId() == d->delayTimer.timerId()) {
        d->stopDelay();
        QQuickPopup::open();
    }
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickToolTip::accessibleRole() const
{
    return QAccessible::ToolTip;
}
#endif

class QQuickToolTipAttachedPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickToolTipAttached)

public:
    QQuickToolTipAttachedPrivate() : delay(0), timeout(-1) { }

    QQuickToolTip *instance(bool create) const;

    int delay;
    int timeout;
    QString text;
};

QQuickToolTip *QQuickToolTipAttachedPrivate::instance(bool create) const
{
    static QPointer<QQuickToolTip> tip;
    if (!tip && create) {
        // TODO: a cleaner way to create the instance? QQml(Meta)Type?
        QQmlContext *context = qmlContext(parent);
        if (context) {
            QQmlComponent component(context->engine());
            component.setData("import QtQuick.Controls 2.0; ToolTip { }", QUrl());

            QObject *object = component.create(context);
            tip = qobject_cast<QQuickToolTip *>(object);
            if (!tip)
                delete object;
        }
    }
    return tip;
}

QQuickToolTipAttached::QQuickToolTipAttached(QQuickItem *item) :
    QObject(*(new QQuickToolTipAttachedPrivate), item)
{
}

/*!
    \qmlattachedproperty string QtQuick.Controls::ToolTip::text

    This attached property holds the text of the shared tool tip instance.
    The property can be attached to any item.
*/
QString QQuickToolTipAttached::text() const
{
    Q_D(const QQuickToolTipAttached);
    return d->text;
}

void QQuickToolTipAttached::setText(const QString &text)
{
    Q_D(QQuickToolTipAttached);
    if (d->text == text)
        return;

    d->text = text;
    emit textChanged();

    if (QQuickToolTip *tip = d->instance(true))
        tip->setText(text);
}

/*!
    \qmlattachedproperty int QtQuick.Controls::ToolTip::delay

    This attached property holds the delay (milliseconds) of the shared tool tip.
    The property can be attached to any item.
*/
int QQuickToolTipAttached::delay() const
{
    Q_D(const QQuickToolTipAttached);
    return d->delay;
}

void QQuickToolTipAttached::setDelay(int delay)
{
    Q_D(QQuickToolTipAttached);
    if (d->delay == delay)
        return;

    d->delay = delay;
    emit delayChanged();
}

/*!
    \qmlattachedproperty int QtQuick.Controls::ToolTip::timeout

    This attached property holds the timeout (milliseconds) of the shared tool tip.
    The property can be attached to any item.
*/
int QQuickToolTipAttached::timeout() const
{
    Q_D(const QQuickToolTipAttached);
    return d->timeout;
}

void QQuickToolTipAttached::setTimeout(int timeout)
{
    Q_D(QQuickToolTipAttached);
    if (d->timeout == timeout)
        return;

    d->timeout = timeout;
    emit timeoutChanged();
}

/*!
    \qmlattachedproperty bool QtQuick.Controls::ToolTip::visible

    This attached property holds whether the shared tool tip is visible.
    The property can be attached to any item.
*/
bool QQuickToolTipAttached::isVisible() const
{
    Q_D(const QQuickToolTipAttached);
    QQuickToolTip *tip = d->instance(false);
    if (!tip)
        return false;

    return tip->isVisible() && tip->parentItem() == parent();
}

void QQuickToolTipAttached::setVisible(bool visible)
{
    Q_D(QQuickToolTipAttached);
    if (visible)
        show(d->text);
    else
        hide();
}

/*!
    \qmlattachedproperty ToolTip QtQuick.Controls::ToolTip::toolTip

    This attached property holds the shared tool tip instance. The property
    can be attached to any item.
*/
QQuickToolTip *QQuickToolTipAttached::toolTip() const
{
    Q_D(const QQuickToolTipAttached);
    return d->instance(true);
}

/*!
    \qmlattachedmethod void QtQuick.Controls::ToolTip::show(string text, int timeout = -1)

    This attached method shows the shared tooltip with \a text and \a timeout (milliseconds).
    The method can be attached to any item.
*/
void QQuickToolTipAttached::show(const QString &text, int ms)
{
    Q_D(QQuickToolTipAttached);
    QQuickToolTip *tip = d->instance(true);
    if (!tip)
        return;

    tip->resetWidth();
    tip->resetHeight();
    tip->setParentItem(qobject_cast<QQuickItem *>(parent()));
    tip->setTimeout(ms >= 0 ? ms : d->timeout);
    tip->setDelay(d->delay);
    tip->setText(text);
    tip->open();
}

/*!
    \qmlattachedmethod void QtQuick.Controls::ToolTip::hide()

    This attached method hides the shared tooltip. The method can be attached to any item.
*/
void QQuickToolTipAttached::hide()
{
    Q_D(QQuickToolTipAttached);
    QQuickToolTip *tip = d->instance(false);
    if (!tip)
        return;

    tip->close();
}

QT_END_NAMESPACE
