/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
