// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
    void checkCompletions(const QByteArray &filePath, int lineNr, int character,
                          ExpectedCompletions expected, QStringList notExpected);
    std::optional<QByteArray> openFile(const QString &uri);
    std::optional<QByteArray> openFileFromAbsolutePath(const QString &uri);
    void ignoreDiagnostics();
private slots:
    void init() final;
    void cleanup();
    void initTestCase() final;
    void function_documentations_data();
    void function_documentations();
    void buildDir();
    void goToTypeDefinition_data();
    void goToTypeDefinition();
    void goToDefinition_data();
    void goToDefinition();
    void findUsages_data();
    void findUsages();
    void documentFormatting_data();
    void documentFormatting();
    void renameUsages_data();
    void renameUsages();
    void linting_data();
    void linting();
    void warnings_data();
    void warnings();
    void rangeFormatting_data();
    void rangeFormatting();
    void qmldirImports_data();
    void qmldirImports();
    void quickFixes_data();
    void quickFixes();
    void automaticSemicolonInsertionForCompletions_data();
    void automaticSemicolonInsertionForCompletions();

private:
    QProcess m_server;
    std::unique_ptr<QLanguageServerProtocol> m_protocol;
    QString m_qmllsPath;
    QList<QByteArray> m_uriToClose;
};

#endif // TST_QMLLSMODULES_H
