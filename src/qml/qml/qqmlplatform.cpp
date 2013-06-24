/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlplatform_p.h"

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
    return QLatin1String("android");
#elif defined(Q_OS_BLACKBERRY)
    return QLatin1String("blackberry");
#elif defined(Q_OS_IOS)
    return QLatin1String("ios");
#elif defined(Q_OS_MAC)
    return QLatin1String("osx");
#elif defined(Q_OS_WINCE)
    return QLatin1String("wince");
#elif defined(Q_OS_WIN)
    return QLatin1String("windows");
#elif defined(Q_OS_LINUX)
    return QLatin1String("linux");
#elif defined(Q_OS_UNIX)
    return QLatin1String("unix");
#else
    return QLatin1String("unknown");
#endif
}

QT_END_NAMESPACE
