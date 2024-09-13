// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QQMLJSTYPEDESCRIPTIONREADER_H
#define TST_QQMLJSTYPEDESCRIPTIONREADER_H

#include <QtQmlCompiler/private/qqmljstypedescriptionreader_p.h>
#include <QtTest/QtTest>
#include <QtQuickTestUtils/private/qmlutils_p.h>

QT_BEGIN_NAMESPACE

class tst_qqmljstypedescriptionreader : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qqmljstypedescriptionreader() : QQmlDataTest(QT_QMLTEST_DATADIR) { }
private slots:
    void warningFreeQmltypes_data();
    void warningFreeQmltypes();
};

QT_END_NAMESPACE

#endif // TST_QQMLJSTYPEDESCRIPTIONREADER_H
