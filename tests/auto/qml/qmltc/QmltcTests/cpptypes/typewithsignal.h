// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TYPEWITHSIGNAL_H
#define TYPEWITHSIGNAL_H

#include <QtCore/qobject.h>
#include <QtQml/qqmlregistration.h>
#include <QtGui/qfont.h>

class TypeWithSignal : public QObject
{
    Q_OBJECT
    QML_ELEMENT

Q_SIGNALS:
    // value types by value
    void signalWithPrimitive(int);
    void signalWithGadget(QFont);

    // value types by const reference
    void signalWithConstReferenceToGadget(const QFont &);
    void signalWithConstReferenceToPrimitive(const int &);

    // object by pointers
    void signalWithPointer(QObject *);
    void signalWithPointerToConst(const QObject *);
    void signalWithPointerToConst2(QObject const *);
    void signalWithConstPointer(QObject *const);
    void signalWithConstPointerToConst(const QObject *const);
    void signalWithConstPointerToConst2(QObject const *const);
};

#endif // TYPEWITHSIGNAL_H
