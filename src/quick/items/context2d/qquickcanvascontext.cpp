// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qquickcanvascontext_p.h>


QT_BEGIN_NAMESPACE

QQuickCanvasContext::QQuickCanvasContext(QObject *parent)
    : QObject(parent)
{
}

QQuickCanvasContext::~QQuickCanvasContext()
{
}

void QQuickCanvasContext::prepare(const QSize& canvasSize, const QSize& tileSize, const QRect& canvasWindow, const QRect& dirtyRect, bool smooth, bool antialiasing)
{
    Q_UNUSED(canvasSize);
    Q_UNUSED(tileSize);
    Q_UNUSED(canvasWindow);
    Q_UNUSED(dirtyRect);
    Q_UNUSED(smooth);
    Q_UNUSED(antialiasing);
}

void QQuickCanvasContext::flush()
{
}

QT_END_NAMESPACE


#include "moc_qquickcanvascontext_p.cpp"
