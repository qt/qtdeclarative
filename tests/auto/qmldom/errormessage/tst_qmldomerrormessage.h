// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQmlDom/qqmldom_global.h>
#include <QtTest/QtTest>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT TestErrorMessage: public QObject
{
    Q_OBJECT
private slots:
  void testError();
};

} // Dom
} // QQmlJS
QT_END_NAMESPACE
