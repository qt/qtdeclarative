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

#ifndef QQMLSYSTEMINFORMATION_P_H
#define QQMLSYSTEMINFORMATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtQmlCore/private/qqmlcoreglobal_p.h>
#include <QtQml/qqmlregistration.h>

QT_BEGIN_NAMESPACE
class Q_QMLCORE_PRIVATE_EXPORT QQmlSystemInformation : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(SystemInformation)
    QML_ADDED_IN_VERSION(6, 4)

    Q_PROPERTY(QString buildCpuArchitecture READ buildCpuArchitecture CONSTANT FINAL)
    Q_PROPERTY(QString currentCpuArchitecture READ currentCpuArchitecture CONSTANT FINAL)
    Q_PROPERTY(QString buildAbi READ buildAbi CONSTANT FINAL)
    Q_PROPERTY(QString kernelType READ kernelType CONSTANT FINAL)
    Q_PROPERTY(QString kernelVersion READ kernelVersion CONSTANT FINAL)
    Q_PROPERTY(QString productType READ productType CONSTANT FINAL)
    Q_PROPERTY(QString productVersion READ productVersion CONSTANT FINAL)
    Q_PROPERTY(QString prettyProductName READ prettyProductName CONSTANT FINAL)
    Q_PROPERTY(QString machineHostName READ machineHostName CONSTANT FINAL)
    Q_PROPERTY(QByteArray machineUniqueId READ machineUniqueId CONSTANT FINAL)
    Q_PROPERTY(QByteArray bootUniqueId READ bootUniqueId CONSTANT FINAL)

public:
    explicit QQmlSystemInformation(QObject *parent = nullptr);

    QString buildCpuArchitecture() const;
    QString currentCpuArchitecture() const;
    QString buildAbi() const;
    QString kernelType() const;
    QString kernelVersion() const;
    QString productType() const;
    QString productVersion() const;
    QString prettyProductName() const;
    QString machineHostName() const;
    QByteArray machineUniqueId() const;
    QByteArray bootUniqueId() const;
};
QT_END_NAMESPACE

#endif // QQMLSYSTEMINFORMATION_P_H
