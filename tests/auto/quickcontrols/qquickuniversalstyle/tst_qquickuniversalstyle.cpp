// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuickTest/quicktest.h>

class Setup : public QObject
{
    Q_OBJECT

public slots:
    void applicationAvailable()
    {
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuWindows);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(tst_qquickuniversalstyle, Setup)

#include "tst_qquickuniversalstyle.moc"
