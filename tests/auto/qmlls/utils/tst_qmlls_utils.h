// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMLLS_UTILS_H
#define TST_QMLLS_UTILS_H

#include <QtJsonRpc/private/qjsonrpcprotocol_p.h>
#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtCore/private/qfactoryloader_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qlibraryinfo.h>

#include <QtTest/qtest.h>

#include <QtQmlLS/private/qqmllsutils_p.h>
#include <QtQmlLS/private/qqmllscompletion_p.h>

#include <iostream>

using namespace Qt::StringLiterals;

class tst_qmlls_utils : public QQmlDataTest
{
    Q_OBJECT

    struct ExpectedCompletion
    {
        QString label;
        QLspSpecification::CompletionItemKind kind;
        QString snippet = {};
    };
    using ExpectedCompletions = QList<ExpectedCompletion>;

    using ExpectedDocumentation = std::tuple<QString, QString, QString>;
    using ExpectedDocumentations = QList<ExpectedDocumentation>;

public:
    tst_qmlls_utils()
        : QQmlDataTest(QT_QMLLS_UTILS_DATADIR),
          m_pluginLoader(QmlLSPluginInterface_iid, u"/qmlls"_s)
    {
    }

private slots:
    void textOffsetRowColumnConversions_data();
    void textOffsetRowColumnConversions();

    void findItemFromLocation_data();
    void findItemFromLocation();

    void findTypeDefinitionFromLocation_data();
    void findTypeDefinitionFromLocation();

    void findDefinitionFromLocation_data();
    void findDefinitionFromLocation();

    void findLocationOfItem_data();
    void findLocationOfItem();

    void findBaseObject();
    void findBaseObject_data();

    void findUsages();
    void findUsages_data();

    void renameUsages();
    void renameUsages_data();

    void resolveExpressionType();
    void resolveExpressionType_data();

    void isValidEcmaScriptIdentifier();
    void isValidEcmaScriptIdentifier_data();

    void completions_data();
    void completions();

    void cmakeBuildCommand();

private:
    using EnvironmentAndFile = std::tuple<QQmlJS::Dom::DomItem, QQmlJS::Dom::DomItem>;

    EnvironmentAndFile createEnvironmentAndLoadFile(const QString &file,
                                                    const QStringList &extraBuildDir = {});

    ExpectedCompletions quickSnippets(const QStringView firstPrefix,
                                      const QStringView secondPrefix) const;
    ExpectedCompletions quickBindingSnippets(const QStringView firstPrefix) const;

    using CacheKey = QString;
    // avoid loading the same file over and over when running all the tests
    QHash<CacheKey, std::shared_ptr<QQmlJS::Dom::DomEnvironment>> cache;
    QFactoryLoader m_pluginLoader;

};

#endif // TST_QMLLS_UTILS_H
