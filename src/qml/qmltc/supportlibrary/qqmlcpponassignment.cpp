// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlcpponassignment_p.h"

QT_BEGIN_NAMESPACE

void QQmlCppOnAssignmentHelper::set(QQmlPropertyValueInterceptor *interceptor,
                                    const QQmlProperty &property)
{
    interceptor->setTarget(property);
}

void QQmlCppOnAssignmentHelper::set(QQmlPropertyValueSource *valueSource,
                                    const QQmlProperty &property)
{
    valueSource->setTarget(property);
}

QT_END_NAMESPACE
