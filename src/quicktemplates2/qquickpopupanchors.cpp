// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
