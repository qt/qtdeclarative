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

#include "qquickpanel_p.h"
#include "qquickpanel_p_p.h"
#include "qquickapplicationwindow_p.h"
#include "qquickoverlay_p.h"
#include <QtQml/qqmlinfo.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

QQuickPanelPrivate::QQuickPanelPrivate()
    : QObjectPrivate()
    , contentItem(Q_NULLPTR)
    , overlay(Q_NULLPTR)
    , focus(false)
    , modal(false)
    , showTransition(Q_NULLPTR)
    , hideTransition(Q_NULLPTR)
    , transitionManager(this)
{ }

QQuickPanelPrivate::~QQuickPanelPrivate()
{ }

void QQuickPanelPrivate::finalizeShowTransition()
{
    if (focus)
        contentItem->setFocus(true);
}

void QQuickPanelPrivate::finalizeHideTransition()
{
    overlay = Q_NULLPTR;
    contentItem->setParentItem(Q_NULLPTR);
    emit q_func()->visibleChanged();
}

QQuickPanelTransitionManager::QQuickPanelTransitionManager(QQuickPanelPrivate *priv)
    : QQuickTransitionManager()
    , state(Off)
    , pp(priv)
{ }

void QQuickPanelTransitionManager::transitionShow()
{
    if (isRunning())
        return;
    QList<QQuickStateAction> actions;
    state = Show;
    transition(actions, pp->showTransition, pp->contentItem);
}

void QQuickPanelTransitionManager::transitionHide()
{
    if (isRunning())
        return;
    QList<QQuickStateAction> actions;
    state = Hide;
    transition(actions, pp->hideTransition, pp->contentItem);
}

void QQuickPanelTransitionManager::finished()
{
    if (state == Show)
        pp->finalizeShowTransition();
    else if (state == Hide)
        pp->finalizeHideTransition();

    state = Off;
}

QQuickPanel::QQuickPanel(QObject *parent)
    : QObject(*(new QQuickPanelPrivate), parent)
{
}

QQuickPanel::QQuickPanel(QQuickPanelPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

QQuickPanel::~QQuickPanel()
{
}

void QQuickPanel::show()
{
    Q_D(QQuickPanel);
    if (!d->contentItem) {
        qmlInfo(this) << "no panel content to show.";
        return;
    }
    if (d->overlay) {
        // FIXME qmlInfo needs to know about QQuickWindow and/or QObject
        static_cast<QDebug>(qmlInfo(this) << "panel already showing in window") << d->overlay->window();
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
        qmlInfo(this) << "cannot find any window to show panel.";
        return;
    }

    if (QQuickApplicationWindow *appWin = qobject_cast<QQuickApplicationWindow*>(win)) {
        d->overlay = static_cast<QQuickOverlay *>(appWin->overlay());
        d->contentItem->setParentItem(d->overlay);
    } else {
        // FIXME Maybe try to show it somehow on that window
        qmlInfo(this) << "is not in an ApplicationWindow.";
        return;
    }

    emit aboutToShow();
    d->transitionManager.transitionShow();
    emit visibleChanged();
}

void QQuickPanel::hide()
{
    Q_D(QQuickPanel);

    if (!d->overlay) {
        // TODO This could mean we showed the panel item on a plain QQuickWindow
        qmlInfo(this) << "trying to hide non-visible Panel.";
        return;
    }

    d->contentItem->setFocus(false);
    emit aboutToHide();
    d->transitionManager.transitionHide();
}

void QQuickPanel::setContentItem(QQuickItem *item)
{
    Q_D(QQuickPanel);
    if (d->overlay) {
        // FIXME qmlInfo needs to know about QQuickItem and/or QObject
        static_cast<QDebug>(qmlInfo(this) << "cannot set content item") << item << "while Panel is visible.";
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

QQuickItem *QQuickPanel::contentItem() const
{
    Q_D(const QQuickPanel);
    return d->contentItem;
}

bool QQuickPanel::hasFocus() const
{
    Q_D(const QQuickPanel);
    return d->focus;
}

void QQuickPanel::setFocus(bool focus)
{
    Q_D(QQuickPanel);
    if (d->focus == focus)
        return;
    d->focus = focus;
    emit focusChanged();
}

bool QQuickPanel::isModal() const
{
    Q_D(const QQuickPanel);
    return d->modal;
}

void QQuickPanel::setModal(bool modal)
{
    Q_D(QQuickPanel);
    if (d->modal == modal)
        return;
    d->modal = modal;
    emit modalChanged();
}

bool QQuickPanel::isVisible() const
{
    Q_D(const QQuickPanel);
    return d->overlay != Q_NULLPTR /*&& !d->transitionManager.isRunning()*/;
}

QQuickTransition *QQuickPanel::showTransition() const
{
    return d_func()->showTransition;
}

void QQuickPanel::setShowTransition(QQuickTransition *t)
{
    Q_D(QQuickPanel);
    if (d->showTransition == t)
        return;
    d->showTransition = t;
    emit showTransitionChanged();
}

QQuickTransition *QQuickPanel::hideTransition() const
{
    return d_func()->hideTransition;
}

void QQuickPanel::setHideTransition(QQuickTransition *t)
{
    Q_D(QQuickPanel);
    if (d->hideTransition == t)
        return;
    d->hideTransition = t;
    emit hideTransitionChanged();
}

QT_END_NAMESPACE

#include "moc_qquickpanel_p.cpp"
