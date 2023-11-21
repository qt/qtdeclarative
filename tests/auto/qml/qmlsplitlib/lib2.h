// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SPLITLIB_LIB2_H
#define SPLITLIB_LIB2_H
#include "tst-qmlsplitlib-library-2_export.h"

#include <QtQmlIntegration/qqmlintegration.h>
#include <QtCore/qglobal.h>
#include <QObject>

class TST_QMLSPLITLIB_LIBRARY_2_EXPORT SplitLib2 : public QObject
{
public:
    Q_OBJECT
    QML_ELEMENT

    Q_INVOKABLE bool transmogrify();
};


class TST_QMLSPLITLIB_LIBRARY_2_EXPORT Foo2 : public QObject
{
public:
    Q_OBJECT
    QML_NAMED_ELEMENT(Bar2)
    QML_SINGLETON
};

#endif
