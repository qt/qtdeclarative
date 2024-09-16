// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QECHODEVICE_H
#define QECHODEVICE_H

#include <QtCore/qiodevice.h>

QT_BEGIN_NAMESPACE

class QIOPipePrivate;
class QIOPipe : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QIOPipe)

public:
    QIOPipe(QObject *parent = nullptr);

    bool open(QIODevice::OpenMode mode);

    QIODevice *end1() const;
    QIODevice *end2() const;
};

QT_END_NAMESPACE

#endif // QECHODEVICE_H
