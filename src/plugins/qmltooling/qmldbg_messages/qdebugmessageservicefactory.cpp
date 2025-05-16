// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qdebugmessageservicefactory.h"
#include "qdebugmessageservice.h"
#include <private/qqmldebugserviceinterfaces_p.h>

QT_BEGIN_NAMESPACE

QQmlDebugService *QDebugMessageServiceFactory::create(const QString &key)
{
    if (key == QDebugMessageServiceImpl::s_key)
        return new QDebugMessageServiceImpl(this);

    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qdebugmessageservicefactory.cpp"
