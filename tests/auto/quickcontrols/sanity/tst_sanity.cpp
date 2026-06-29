// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtQml>
#include <QtCore/private/qhooks_p.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qset.h>
#include <QtCore/qplugin.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <private/qqmljslinter_p.h>
#include <utility>

Q_IMPORT_PLUGIN(QuickControlsSanityPlugin)

using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;
using namespace Qt::StringLiterals;

class tst_Sanity : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_Sanity();

private slots:
    void initTestCase() override;

    void jsFiles();
    void qmllint();
    void qmllint_data();

    void quickControlsSanityPlugin();
    void quickControlsSanityPlugin_data();

private:
    QStringList m_importPaths;

    QQmlJSLinter m_linter;
    QList<QQmlJS::LoggerCategory> m_categories;
};

tst_Sanity::tst_Sanity()
    : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::DoNotFailOnWarnings),
      m_importPaths({ QLibraryInfo::path(QLibraryInfo::QmlImportsPath) }),
      m_linter(m_importPaths, m_importPaths),
      m_categories(QQmlJSLogger::builtinCategories())
{
    // We do not care about any warnings that aren't explicitly created by controls-sanity.
    // Mainly because a lot of false positives are generated because we are linting files from
    // different modules directly without their generated qmldirs.

    m_linter.setPluginsEnabled(true);

    for (auto &category : m_categories)
        category.setSeverity(QQmlSA::WarningSeverity::Disable);

    for (auto &plugin: m_linter.plugins()) {
        if (plugin.name() != u"QuickControlsSanity") {
            plugin.setEnabled(false);
            continue;
        }

        for (const auto &category: plugin.categories()) {
            m_categories.append(category);
            m_categories.back().setSeverity(QQmlSA::WarningSeverity::Warning);
        }
    }
}

void tst_Sanity::initTestCase()
{
    QQmlDataTest::initTestCase();
    StyleInfo::instance()->initialize(QQC2_IMPORT_PATH);
}

void tst_Sanity::jsFiles()
{
    const QList<StyleInfo::QmlFileData> sourceQmlFiles = StyleInfo::instance()->sourceQmlFiles();
    for (auto it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it) {
        if (QFileInfo(it->absolutePath).suffix() == QStringLiteral("js"))
            QFAIL(qPrintable(it->absolutePath +  ": JS files are not allowed"));
    }
}

void tst_Sanity::qmllint()
{
    QFETCH(QString, control);
    QFETCH(QString, filePath);

    QQmlJSLinter::Result lintResult = m_linter.lintFileInBatch(filePath);
    QVERIFY2(lintResult.status == QQmlJSLinter::LintSuccess,
             qPrintable(QJsonDocument(lintResult.json).toJson(QJsonDocument::Compact)));
}

void tst_Sanity::qmllint_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QString>("filePath");

    const QList<StyleInfo::QmlFileData> sourceQmlFiles = StyleInfo::instance()->sourceQmlFiles();
    for (auto it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it) {
        QTest::newRow(qPrintable(it->relativePath)) << it->relativePath << it->absolutePath;
        QQmlJSLinter::LintOptions options;
        options.setFlag(QQmlJSLinter::Silent);
        options.setFlag(QQmlJSLinter::GenerateJson);
        m_linter.prepareFileForBatchLinting(it->absolutePath, nullptr, options, m_importPaths, { },
                                            { }, m_categories);
    }
}

void tst_Sanity::quickControlsSanityPlugin()
{
    QFETCH(QString, filePath);
    QFETCH(QString, result);

    QQmlJSLinter::LintOptions options;
    options.setFlag(QQmlJSLinter::Silent);
    options.setFlag(QQmlJSLinter::GenerateJson);
    QVERIFY(m_linter.prepareFileForBatchLinting(testFile(filePath), nullptr, options, m_importPaths,
                                                { }, { }, m_categories));
    QQmlJSLinter::Result lintResult = m_linter.lintFileInBatch(testFile(filePath));
    QCOMPARE(lintResult.status, QQmlJSLinter::HasWarnings);
    const auto &warningsOutput = lintResult.json.value("warnings").toArray();
    QCOMPARE(warningsOutput.first().toObject().value("message"), result);
}

void tst_Sanity::quickControlsSanityPlugin_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QString>("result");
    QTest::newRow("functionDeclarations") << QStringLiteral("functionDeclarations.qml")
                                          << QStringLiteral("Declared function \"add\"");
    QTest::newRow("signalHandlers") << QStringLiteral("signalHandlers.qml")
                                    << QStringLiteral("Declared signal handler \"onCompleted\"");
    QTest::newRow("anchors") << QStringLiteral("anchors.qml")
                             << QStringLiteral("Using anchors here");
}

QTEST_MAIN(tst_Sanity)

#include "tst_sanity.moc"
