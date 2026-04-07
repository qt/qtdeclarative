// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MyType_H
#define MyType_H

#include <QtCore/qobject.h>
#include <QtQml/qqmlregistration.h>

class MyType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int myProperty MEMBER m_myProperty NOTIFY myPropertyChanged)

signals:
    void myPropertyChanged();

private:
    int m_myProperty;
};

#endif // MyType_H
