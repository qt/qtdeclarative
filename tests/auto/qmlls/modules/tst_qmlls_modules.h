// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TST_QMLLSMODULES_H
#define TST_QMLLSMODULES_H

#include <QtJsonRpc/private/qjsonrpcprotocol_p.h>
#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtCore/private/qduplicatetracker_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qstringlist.h>

#include <QtTest/qtest.h>
#include <QtQmlLS/private/qlspcustomtypes_p.h>

#include <iostream>
#include <variant>


class tst_qmlls_modules : public QQmlDataTest
{
    using ExpectedCompletion = QPair<QString, QLspSpecification::CompletionItemKind>;
    using ExpectedCompletions = QList<ExpectedCompletion>;

    using ExpectedDocumentation = std::tuple<QString, QString, QString>;
    using ExpectedDocumentations = QList<ExpectedDocumentation>;

    Q_OBJECT
public:
    tst_qmlls_modules();
    void checkCompletions(QByteArray uri, int lineNr, int character, ExpectedCompletions expected,
                          QStringList notExpected);
private slots:
    void initTestCase() final;
    void completions_data();
    void completions();
    void function_documentations_data();
    void function_documentations();
    void buildDir();
    void cleanupTestCase();
    void goToTypeDefinition_data();
    void goToTypeDefinition();
    void findUsages_data();
    void findUsages();

private:
    QProcess m_server;
    QLanguageServerProtocol m_protocol;
    QString m_qmllsPath;
    QList<QByteArray> m_uriToClose;
};

#endif // TST_QMLLSMODULES_H
