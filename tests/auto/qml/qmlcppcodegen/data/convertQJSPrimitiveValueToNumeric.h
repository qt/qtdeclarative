// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CONVERTQJSPRIMITIVEVALUETONUMERIC_H
#define CONVERTQJSPRIMITIVEVALUETONUMERIC_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

class Moo485 : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(quint8 u8 MEMBER u8 FINAL)
    Q_PROPERTY(qint8 i8 MEMBER i8 FINAL)
    Q_PROPERTY(quint16 u16 MEMBER u16 FINAL)
    Q_PROPERTY(qint16 i16 MEMBER i16 FINAL)
    Q_PROPERTY(quint32 u32 MEMBER u32 FINAL)
    Q_PROPERTY(qint32 i32 MEMBER i32 FINAL)
    Q_PROPERTY(float f MEMBER f FINAL)
    Q_PROPERTY(qreal r MEMBER r FINAL)
    Q_PROPERTY(double d MEMBER d FINAL)

public:
    quint8 u8;
    qint8 i8;
    quint16 u16;
    qint16 i16;
    quint32 u32;
    qint32 i32;
    float f;
    qreal r;
    double d;
};

#endif // CONVERTQJSPRIMITIVEVALUETONUMERIC_H
