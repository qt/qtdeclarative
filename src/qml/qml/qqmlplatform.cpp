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
#elif defined(Q_OS_WINRT)
    return QStringLiteral("winrt");
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
