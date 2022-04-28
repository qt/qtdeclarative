/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQmlCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlsysteminformation_p.h"
#include <QtCore/qsysinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SystemInformation
    \inherits QtObject
    \inqmlmodule QtCore
    \since 6.4
    \brief Provides information about the system.

    The SystemInformation singleton type provides information about the system,
    using the same API as \l QSysInfo.

    \qml
    property string prettyProductName: SystemInformation.prettyProductName
    \endqml

    \sa QSysInfo
*/

QQmlSystemInformation::QQmlSystemInformation(QObject *parent) : QObject(parent)
{
}

QString QQmlSystemInformation::buildCpuArchitecture() const
{
    return QSysInfo::buildCpuArchitecture();
}

QString QQmlSystemInformation::currentCpuArchitecture() const
{
    return QSysInfo::currentCpuArchitecture();
}

QString QQmlSystemInformation::buildAbi() const
{
    return QSysInfo::buildAbi();
}

QString QQmlSystemInformation::kernelType() const
{
    return QSysInfo::kernelType();
}

QString QQmlSystemInformation::kernelVersion() const
{
    return QSysInfo::kernelVersion();
}

QString QQmlSystemInformation::productType() const
{
    return QSysInfo::productType();
}

QString QQmlSystemInformation::productVersion() const
{
    return QSysInfo::productVersion();
}

QString QQmlSystemInformation::prettyProductName() const
{
    return QSysInfo::prettyProductName();
}

QString QQmlSystemInformation::machineHostName() const
{
    return QSysInfo::machineHostName();
}

QByteArray QQmlSystemInformation::machineUniqueId() const
{
    return QSysInfo::machineUniqueId();
}

QByteArray QQmlSystemInformation::bootUniqueId() const
{
    return QSysInfo::bootUniqueId();
}

QT_END_NAMESPACE

#include "moc_qqmlsysteminformation_p.cpp"
