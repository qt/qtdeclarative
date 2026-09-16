// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QQMLLANGUAGESERVER_H
#define TST_QQMLLANGUAGESERVER_H

#include <QtCore/qobject.h>
#include <QtTest/qtest.h>

class tst_qqmllanguageserver : public QObject
{
    Q_OBJECT
public:
    tst_qqmllanguageserver();

private slots:
    void noFreezeAfterRequestToClient();
};

#endif // TST_QQMLLANGUAGESERVER_H
