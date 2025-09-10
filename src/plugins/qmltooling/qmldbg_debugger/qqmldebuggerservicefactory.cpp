// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldebuggerservicefactory.h"
#include "qqmlenginedebugservice.h"
#include "qv4debugservice.h"
#include <private/qqmldebugserviceinterfaces_p.h>

QT_BEGIN_NAMESPACE

QQmlDebugService *QQmlDebuggerServiceFactory::create(const QString &key)
{
    if (key == QQmlEngineDebugServiceImpl::s_key)
        return new QQmlEngineDebugServiceImpl(this);

    if (key == QV4DebugServiceImpl::s_key)
        return new QV4DebugServiceImpl(this);

    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qqmldebuggerservicefactory.cpp"
