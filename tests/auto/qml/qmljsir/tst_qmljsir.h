// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

class tst_qmljsir : public QObject
{
    Q_OBJECT
private slots:
    void failureMode();
    void propertyVirtSpecifiers_data();
    void propertyVirtSpecifiers();
};
