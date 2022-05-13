// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickprofileradapterfactory.h"
#include "qquickprofileradapter.h"
#include <private/qqmldebugconnector_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>

QT_BEGIN_NAMESPACE

QQmlAbstractProfilerAdapter *QQuickProfilerAdapterFactory::create(const QString &key)
{
    if (key != QLatin1String("QQuickProfilerAdapter"))
        return nullptr;
    return new QQuickProfilerAdapter(this);
}

QT_END_NAMESPACE

#include "moc_qquickprofileradapterfactory.cpp"
