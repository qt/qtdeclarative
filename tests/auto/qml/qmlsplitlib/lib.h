// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SPLITLIB_LIB_H
#define SPLITLIB_LIB_H
#include "tst_qmlsplitlib_library_export.h"

#include <QtQmlIntegration/qqmlintegration.h>
#include <QtCore/qglobal.h>
#include <QObject>

class TST_QMLSPLITLIB_LIBRARY_EXPORT SplitLib : public QObject
{
public:
    Q_OBJECT
    QML_ELEMENT

    Q_INVOKABLE bool transmogrify();
};


class TST_QMLSPLITLIB_LIBRARY_EXPORT Foo : public QObject
{
public:
    Q_OBJECT
    QML_NAMED_ELEMENT(Bar)
    QML_SINGLETON
};

#endif
