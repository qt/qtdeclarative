/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qquickpopupanchors_p.h"
#include "qquickpopupanchors_p_p.h"
#include "qquickpopup_p_p.h"

QT_BEGIN_NAMESPACE

QQuickPopupAnchors::QQuickPopupAnchors(QQuickPopup *popup)
    : QObject(*(new QQuickPopupAnchorsPrivate), popup)
{
    Q_D(QQuickPopupAnchors);
    d->popup = popup;
}

QQuickPopupAnchors::~QQuickPopupAnchors()
{
    Q_D(const QQuickPopupAnchors);
    if (d->centerIn) {
        auto centerInPrivate = QQuickItemPrivate::get(d->centerIn);
        centerInPrivate->removeItemChangeListener(this, QQuickItemPrivate::Destroyed);
    }
}

QQuickItem *QQuickPopupAnchors::centerIn() const
{
    Q_D(const QQuickPopupAnchors);
    return d->centerIn;
}

void QQuickPopupAnchors::setCenterIn(QQuickItem *item)
{
    Q_D(QQuickPopupAnchors);
    if (item == d->centerIn)
        return;

    if (d->centerIn) {
        auto centerInPrivate = QQuickItemPrivate::get(d->centerIn);
        centerInPrivate->removeItemChangeListener(this, QQuickItemPrivate::Destroyed);
    }

    d->centerIn = item;

    if (d->centerIn) {
        auto centerInPrivate = QQuickItemPrivate::get(d->centerIn);
        centerInPrivate->addItemChangeListener(this, QQuickItemPrivate::Destroyed);
    }

    QQuickPopupPrivate::get(d->popup)->reposition();

    emit centerInChanged();
}

void QQuickPopupAnchors::resetCenterIn()
{
    setCenterIn(nullptr);
}

void QQuickPopupAnchors::itemDestroyed(QQuickItem *)
{
    resetCenterIn();
}

QT_END_NAMESPACE

#include "moc_qquickpopupanchors_p.cpp"
