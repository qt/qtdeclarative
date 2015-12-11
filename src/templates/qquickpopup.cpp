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
*/

QQuickPopupPrivate::QQuickPopupPrivate()
    : QObjectPrivate()
    , contentItem(Q_NULLPTR)
    , overlay(Q_NULLPTR)
    , focus(false)
    , modal(false)
    , showTransition(Q_NULLPTR)
    , hideTransition(Q_NULLPTR)
    , transitionManager(this)
{ }

QQuickPopupPrivate::~QQuickPopupPrivate()
{ }

void QQuickPopupPrivate::finalizeShowTransition()
{
    if (focus)
        contentItem->setFocus(true);
}

void QQuickPopupPrivate::finalizeHideTransition()
{
    overlay = Q_NULLPTR;
    contentItem->setParentItem(Q_NULLPTR);
    emit q_func()->visibleChanged();
}

QQuickPopupTransitionManager::QQuickPopupTransitionManager(QQuickPopupPrivate *priv)
    : QQuickTransitionManager()
    , state(Off)
    , pp(priv)
{ }

void QQuickPopupTransitionManager::transitionShow()
{
    if (isRunning())
        return;
    QList<QQuickStateAction> actions;
    state = Show;
    transition(actions, pp->showTransition, pp->contentItem);
}

void QQuickPopupTransitionManager::transitionHide()
{
    if (isRunning())
        return;
    QList<QQuickStateAction> actions;
    state = Hide;
    transition(actions, pp->hideTransition, pp->contentItem);
}

void QQuickPopupTransitionManager::finished()
{
    if (state == Show)
        pp->finalizeShowTransition();
    else if (state == Hide)
        pp->finalizeHideTransition();

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

QQuickPopup::~QQuickPopup()
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

    QQuickWindow *win = Q_NULLPTR;
    QObject *p = parent();
    while (p && !win) {
        if (QQuickItem *item = qobject_cast<QQuickItem *>(p)) {
            win = item->window();
            if (!win)
                p = item->parentItem();
        } else {
            win = qobject_cast<QQuickWindow *>(p);
            if (!win)
                p = p->parent();
        }
    }
    if (!win) {
        qmlInfo(this) << "cannot find any window to open popup in.";
        return;
    }

    if (QQuickApplicationWindow *appWin = qobject_cast<QQuickApplicationWindow*>(win)) {
        d->overlay = static_cast<QQuickOverlay *>(appWin->overlay());
        d->contentItem->setParentItem(d->overlay);
    } else {
        // FIXME Maybe try to open it in that window somehow
        qmlInfo(this) << "is not in an ApplicationWindow.";
        return;
    }

    emit aboutToShow();
    d->transitionManager.transitionShow();
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
    d->transitionManager.transitionHide();
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
    \qmlproperty Transition Qt.labs.controls::Popup::showTransition

    This property holds the transition that is applied to the content item
    when the popup is opened.
*/
QQuickTransition *QQuickPopup::showTransition() const
{
    return d_func()->showTransition;
}

void QQuickPopup::setShowTransition(QQuickTransition *t)
{
    Q_D(QQuickPopup);
    if (d->showTransition == t)
        return;
    d->showTransition = t;
    emit showTransitionChanged();
}

/*!
    \qmlproperty Transition Qt.labs.controls::Popup::hideTransition

    This property holds the transition that is applied to the content item
    when the popup is closed.
*/
QQuickTransition *QQuickPopup::hideTransition() const
{
    return d_func()->hideTransition;
}

void QQuickPopup::setHideTransition(QQuickTransition *t)
{
    Q_D(QQuickPopup);
    if (d->hideTransition == t)
        return;
    d->hideTransition = t;
    emit hideTransitionChanged();
}

QT_END_NAMESPACE

#include "moc_qquickpopup_p.cpp"
