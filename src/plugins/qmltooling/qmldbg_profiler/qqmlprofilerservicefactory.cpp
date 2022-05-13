// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlprofilerservice.h"
#include "qqmlenginecontrolservice.h"
#include "qqmlprofilerservicefactory.h"

QT_BEGIN_NAMESPACE

QQmlDebugService *QQmlProfilerServiceFactory::create(const QString &key)
{
    if (key == QQmlProfilerServiceImpl::s_key)
        return new QQmlProfilerServiceImpl(this);

    if (key == QQmlEngineControlServiceImpl::s_key)
        return new QQmlEngineControlServiceImpl(this);

    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qqmlprofilerservicefactory.cpp"
