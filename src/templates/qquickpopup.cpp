/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#include "qquickpopup_p.h"
#include "qquickpopup_p_p.h"
#include "qquickapplicationwindow_p.h"
#include "qquickoverlay_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Popup
    \inherits QtObject
    \instantiates QQuickPopup
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-popups
    \brief A popup control.

    Popup is the base type of popup-like user interface controls.

    \labs
*/

QQuickPopupPrivate::QQuickPopupPrivate()
    : QObjectPrivate()
    , focus(false)
    , modal(false)
    , contentItem(Q_NULLPTR)
    , overlay(Q_NULLPTR)
    , enter(Q_NULLPTR)
    , exit(Q_NULLPTR)
    , transitionManager(this)
{
}

void QQuickPopupPrivate::finalizeEnterTransition()
{
    if (focus)
        contentItem->setFocus(true);
}

void QQuickPopupPrivate::finalizeExitTransition()
{
    Q_Q(QQuickPopup);
    overlay = Q_NULLPTR;
    contentItem->setParentItem(Q_NULLPTR);
    emit q->visibleChanged();
}

QQuickPopupTransitionManager::QQuickPopupTransitionManager(QQuickPopupPrivate *popup)
    : QQuickTransitionManager()
    , state(Off)
    , popup(popup)
{
}

void QQuickPopupTransitionManager::transitionEnter()
{
    if (state == Enter && isRunning())
        return;
    QList<QQuickStateAction> actions;
    state = Enter;
    transition(actions, popup->enter, popup->contentItem);
}

void QQuickPopupTransitionManager::transitionExit()
{
    if (state == Exit && isRunning())
        return;
    QList<QQuickStateAction> actions;
    state = Exit;
    transition(actions, popup->exit, popup->contentItem);
}

void QQuickPopupTransitionManager::finished()
{
    if (state == Enter)
        popup->finalizeEnterTransition();
    else if (state == Exit)
        popup->finalizeExitTransition();

    state = Off;
}

QQuickPopup::QQuickPopup(QObject *parent)
    : QObject(*(new QQuickPopupPrivate), parent)
{
}

QQuickPopup::QQuickPopup(QQuickPopupPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

/*!
    \qmlmethod void Qt.labs.controls::Popup::open()

    Opens the popup.
*/
void QQuickPopup::open()
{
    Q_D(QQuickPopup);
    if (!d->contentItem) {
        qmlInfo(this) << "no popup content to show.";
        return;
    }
    if (d->overlay) {
        // FIXME qmlInfo needs to know about QQuickWindow and/or QObject
        static_cast<QDebug>(qmlInfo(this) << "popup already open in window") << d->overlay->window();
        return;
    }

    QQuickWindow *window = Q_NULLPTR;
    QObject *p = parent();
    while (p && !window) {
        if (QQuickItem *item = qobject_cast<QQuickItem *>(p)) {
            window = item->window();
            if (!window)
                p = item->parentItem();
        } else {
            window = qobject_cast<QQuickWindow *>(p);
            if (!window)
                p = p->parent();
        }
    }
    if (!window) {
        qmlInfo(this) << "cannot find any window to open popup in.";
        return;
    }

    QQuickApplicationWindow *applicationWindow = qobject_cast<QQuickApplicationWindow*>(window);
    if (!applicationWindow) {
        // FIXME Maybe try to open it in that window somehow
        qmlInfo(this) << "is not in an ApplicationWindow.";
        return;
    }

    d->overlay = static_cast<QQuickOverlay *>(applicationWindow->overlay());
    d->contentItem->setParentItem(d->overlay);
    emit aboutToShow();
    d->transitionManager.transitionEnter();
    emit visibleChanged();
}

/*!
    \qmlmethod void Qt.labs.controls::Popup::close()

    Closes the popup.
*/
void QQuickPopup::close()
{
    Q_D(QQuickPopup);
    if (!d->overlay) {
        // TODO This could mean we opened the popup item in a plain QQuickWindow
        qmlInfo(this) << "trying to close non-visible Popup.";
        return;
    }

    d->contentItem->setFocus(false);
    emit aboutToHide();
    d->transitionManager.transitionExit();
}

/*!
    \qmlproperty Item Qt.labs.controls::Popup::contentItem

    This property holds the content item of the popup.

    The content item is the visual implementation of the popup. When the
    popup is made visible, the content item is automatically reparented to
    the \l {ApplicationWindow::overlay}{overlay item} of its application
    window.
*/
QQuickItem *QQuickPopup::contentItem() const
{
    Q_D(const QQuickPopup);
    return d->contentItem;
}

void QQuickPopup::setContentItem(QQuickItem *item)
{
    Q_D(QQuickPopup);
    if (d->overlay) {
        // FIXME qmlInfo needs to know about QQuickItem and/or QObject
        static_cast<QDebug>(qmlInfo(this) << "cannot set content item") << item << "while Popup is visible.";
        return;
    }
    if (d->contentItem != item) {
        delete d->contentItem;
        d->contentItem = item;
        if (item)
            QQuickItemPrivate::get(item)->isTabFence = true;
        emit contentItemChanged();
    }
}

/*!
    \qmlproperty bool Qt.labs.controls::Popup::focus

    This property holds whether the popup has focus.
*/
bool QQuickPopup::hasFocus() const
{
    Q_D(const QQuickPopup);
    return d->focus;
}

void QQuickPopup::setFocus(bool focus)
{
    Q_D(QQuickPopup);
    if (d->focus == focus)
        return;
    d->focus = focus;
    emit focusChanged();
}

/*!
    \qmlproperty bool Qt.labs.controls::Popup::modal

    This property holds whether the popup is modal.
*/
bool QQuickPopup::isModal() const
{
    Q_D(const QQuickPopup);
    return d->modal;
}

void QQuickPopup::setModal(bool modal)
{
    Q_D(QQuickPopup);
    if (d->modal == modal)
        return;
    d->modal = modal;
    emit modalChanged();
}

/*!
    \qmlproperty bool Qt.labs.controls::Popup::visible

    This property holds whether the popup is visible.
*/
bool QQuickPopup::isVisible() const
{
    Q_D(const QQuickPopup);
    return d->overlay != Q_NULLPTR /*&& !d->transitionManager.isRunning()*/;
}

/*!
    \qmlproperty Transition Qt.labs.controls::Popup::enter

    This property holds the transition that is applied to the content item
    when the popup is opened and enters the screen.
*/
QQuickTransition *QQuickPopup::enter() const
{
    Q_D(const QQuickPopup);
    return d->enter;
}

void QQuickPopup::setEnter(QQuickTransition *transition)
{
    Q_D(QQuickPopup);
    if (d->enter == transition)
        return;
    d->enter = transition;
    emit enterChanged();
}

/*!
    \qmlproperty Transition Qt.labs.controls::Popup::exit

    This property holds the transition that is applied to the content item
    when the popup is closed and exits the screen.
*/
QQuickTransition *QQuickPopup::exit() const
{
    Q_D(const QQuickPopup);
    return d->exit;
}

void QQuickPopup::setExit(QQuickTransition *transition)
{
    Q_D(QQuickPopup);
    if (d->exit == transition)
        return;
    d->exit = transition;
    emit exitChanged();
}

QT_END_NAMESPACE

#include "moc_qquickpopup_p.cpp"
