/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQMLPROPERTYDATA_P_H
#define QQMLPROPERTYDATA_P_H

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

#include <private/qqmlpropertyrawdata_p.h>

QT_BEGIN_NAMESPACE

class QQmlPropertyData : public QQmlPropertyRawData
{
public:
    enum WriteFlag {
        BypassInterceptor = 0x01,
        DontRemoveBinding = 0x02,
        RemoveBindingOnAliasWrite = 0x04
    };
    Q_DECLARE_FLAGS(WriteFlags, WriteFlag)

    inline QQmlPropertyData();
    inline QQmlPropertyData(const QQmlPropertyRawData &);

    inline bool operator==(const QQmlPropertyRawData &);

    static Flags flagsForProperty(const QMetaProperty &);
    void load(const QMetaProperty &);
    void load(const QMetaMethod &);
    QString name(QObject *) const;
    QString name(const QMetaObject *) const;

    void markAsOverrideOf(QQmlPropertyData *predecessor);

    inline void readProperty(QObject *target, void *property) const
    {
        void *args[] = { property, nullptr };
        readPropertyWithArgs(target, args);
    }

    inline void readPropertyWithArgs(QObject *target, void *args[]) const
    {
        if (hasStaticMetaCallFunction())
            staticMetaCallFunction()(target, QMetaObject::ReadProperty, relativePropertyIndex(), args);
        else if (isDirect())
            target->qt_metacall(QMetaObject::ReadProperty, coreIndex(), args);
        else
            QMetaObject::metacall(target, QMetaObject::ReadProperty, coreIndex(), args);
    }

    bool writeProperty(QObject *target, void *value, WriteFlags flags) const
    {
        int status = -1;
        void *argv[] = { value, nullptr, &status, &flags };
        if (flags.testFlag(BypassInterceptor) && hasStaticMetaCallFunction())
            staticMetaCallFunction()(target, QMetaObject::WriteProperty, relativePropertyIndex(), argv);
        else if (flags.testFlag(BypassInterceptor) && isDirect())
            target->qt_metacall(QMetaObject::WriteProperty, coreIndex(), argv);
        else
            QMetaObject::metacall(target, QMetaObject::WriteProperty, coreIndex(), argv);
        return true;
    }

    static Flags defaultSignalFlags()
    {
        Flags f;
        f.isSignal = true;
        f.type = Flags::FunctionType;
        f.isVMESignal = true;
        return f;
    }

    static Flags defaultSlotFlags()
    {
        Flags f;
        f.type = Flags::FunctionType;
        f.isVMEFunction = true;
        return f;
    }

private:
    friend class QQmlPropertyCache;
    void lazyLoad(const QMetaProperty &);
    void lazyLoad(const QMetaMethod &);
    bool notFullyResolved() const { return _flags.notFullyResolved; }
};

QQmlPropertyData::QQmlPropertyData()
{
    setCoreIndex(-1);
    setPropType(0);
    setNotifyIndex(-1);
    setOverrideIndex(-1);
    setRevision(0);
    setMetaObjectOffset(-1);
    setArguments(nullptr);
    trySetStaticMetaCallFunction(nullptr, 0);
}

QQmlPropertyData::QQmlPropertyData(const QQmlPropertyRawData &d)
{
    *(static_cast<QQmlPropertyRawData *>(this)) = d;
}

bool QQmlPropertyData::operator==(const QQmlPropertyRawData &other)
{
    return flags() == other.flags() &&
            propType() == other.propType() &&
            coreIndex() == other.coreIndex() &&
            notifyIndex() == other.notifyIndex() &&
            revision() == other.revision();
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlPropertyData::WriteFlags)

QT_END_NAMESPACE

#endif // QQMLPROPERTYDATA_P_H
