// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TAKEENUMS_H
#define TAKEENUMS_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

class TakeEnums : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Int8 : qint8 { Value = -2 };
    Q_ENUM(Int8)
    enum class UInt8 : quint8 { Value = 200 };
    Q_ENUM(UInt8)
    enum class Int16 : qint16 { Value = -300 };
    Q_ENUM(Int16)
    enum class UInt16 : quint16 { Value = 4660 };
    Q_ENUM(UInt16)
    enum class Int64 : qint64 { Value = -2 };
    Q_ENUM(Int64)

    explicit TakeEnums(QObject *parent = nullptr) : QObject(parent) { }

    Q_INVOKABLE void takeInt8(TakeEnums::Int8 a) { takenInt8 = a; }
    Q_INVOKABLE void takeUInt8(TakeEnums::UInt8 a) { takenUInt8 = a; }
    Q_INVOKABLE void takeInt16(TakeEnums::Int16 a) { takenInt16 = a; }
    Q_INVOKABLE void takeUInt16(TakeEnums::UInt16 a) { takenUInt16 = a; }
    Q_INVOKABLE void takeInt64(TakeEnums::Int64 a) { takenInt64 = a; }

    Int8 takenInt8 = Int8(0);
    UInt8 takenUInt8 = UInt8(0);
    Int16 takenInt16 = Int16(0);
    UInt16 takenUInt16 = UInt16(0);
    Int64 takenInt64 = Int64(0);
};

#endif // TAKEENUMS_H
