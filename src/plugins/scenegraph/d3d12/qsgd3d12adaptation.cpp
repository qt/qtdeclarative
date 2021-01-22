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

#include "qsgd3d12adaptation_p.h"
#include "qsgd3d12renderloop_p.h"
#include "qsgd3d12threadedrenderloop_p.h"
#include "qsgd3d12context_p.h"

QT_BEGIN_NAMESPACE

QSGD3D12Adaptation::QSGD3D12Adaptation(QObject *parent)
    : QSGContextPlugin(parent)
{
}

QStringList QSGD3D12Adaptation::keys() const
{
    return QStringList() << QLatin1String("d3d12");
}

QSGContext *QSGD3D12Adaptation::create(const QString &) const
{
    if (!contextInstance)
        contextInstance = new QSGD3D12Context;

    return contextInstance;
}

QSGContextFactoryInterface::Flags QSGD3D12Adaptation::flags(const QString &) const
{
    return QSGContextFactoryInterface::SupportsShaderEffectNode;
}

QSGRenderLoop *QSGD3D12Adaptation::createWindowManager()
{
    static bool threaded = false;
    static bool envChecked = false;
    if (!envChecked) {
        envChecked = true;
        threaded = qgetenv("QSG_RENDER_LOOP") == QByteArrayLiteral("threaded");
    }

    if (threaded)
        return new QSGD3D12ThreadedRenderLoop;

    return new QSGD3D12RenderLoop;
}

QSGD3D12Context *QSGD3D12Adaptation::contextInstance = nullptr;

QT_END_NAMESPACE
