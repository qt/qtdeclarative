/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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
**
**
**
****************************************************************************/

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
