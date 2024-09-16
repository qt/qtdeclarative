// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMLLS_CLI_H
#define TST_QMLLS_CLI_H

#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qlibraryinfo.h>

#include <QtTest/qtest.h>

class tst_qmlls_cli: public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qmlls_cli() : QQmlDataTest(QT_QMLTEST_DATADIR) { }
    [[nodiscard]] auto startServerRAII();
    void startServerImpl();
    void stopServerImpl();

private slots:
    void initTestCase();
    void cleanup();
    void warnings_data();
    void warnings();

public:
    QProcess m_server;
    QString m_qmllsPath;
    std::unique_ptr<QLanguageServerProtocol> m_protocol;
};

#endif // TST_QMLLS_CLI_H
