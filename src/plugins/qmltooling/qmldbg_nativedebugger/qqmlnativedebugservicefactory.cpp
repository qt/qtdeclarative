// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qqmlnativedebugservice.h"
#include "qqmlnativedebugservicefactory.h"
#include <private/qqmldebugserviceinterfaces_p.h>

QT_BEGIN_NAMESPACE

QQmlDebugService *QQmlNativeDebugServiceFactory::create(const QString &key)
{
    if (key == QQmlNativeDebugServiceImpl::s_key)
        return new QQmlNativeDebugServiceImpl(this);

    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qqmlnativedebugservicefactory.cpp"
