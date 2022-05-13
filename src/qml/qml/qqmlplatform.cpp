// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlplatform_p.h"
#include "qqmlglobal_p.h"

QT_BEGIN_NAMESPACE

/*
    This object and its properties are documented as part of the Qt object,
    in qqmlengine.cpp
*/

QQmlPlatform::QQmlPlatform(QObject *parent)
    : QObject(parent)
{
}

QQmlPlatform::~QQmlPlatform()
{
}

QString QQmlPlatform::os()
{
#if defined(Q_OS_ANDROID)
    return QStringLiteral("android");
#elif defined(Q_OS_IOS)
    return QStringLiteral("ios");
#elif defined(Q_OS_TVOS)
    return QStringLiteral("tvos");
#elif defined(Q_OS_MAC)
    return QStringLiteral("osx");
#elif defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("linux");
#elif defined(Q_OS_QNX)
    return QStringLiteral("qnx");
#elif defined(Q_OS_WASM)
    return QStringLiteral("wasm");
#elif defined(Q_OS_UNIX)
    return QStringLiteral("unix");
#else
    return QStringLiteral("unknown");
#endif
}

QString QQmlPlatform::pluginName() const
{
    return QQml_guiProvider()->pluginName();
}

QT_END_NAMESPACE

#include "moc_qqmlplatform_p.cpp"
