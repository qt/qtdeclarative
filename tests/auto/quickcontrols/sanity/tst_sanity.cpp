// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <QtQml>
#include <QtCore/private/qhooks_p.h>
#include <QtCore/qpair.h>
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
#include <QtQmlCompiler/private/qqmljslinter_p.h>
#include <QtQmlCompiler/private/qqmljstyperesolver_p.h>
#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>

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
    QMap<QString, QString> sourceQmlFiles;
    QMap<QString, QString> installedQmlFiles;
    QStringList m_importPaths;

    QQmlJSLinter m_linter;
    QList<QQmlJS::LoggerCategory> m_categories;
};

tst_Sanity::tst_Sanity()
    : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::DoNotFailOnWarnings),
      m_importPaths({ QLibraryInfo::path(QLibraryInfo::QmlImportsPath) }),
      m_linter(m_importPaths, m_importPaths),
      m_categories(QQmlJSLogger::defaultCategories())
{
    // We do not care about any warnings that aren't explicitly created by controls-sanity.
    // Mainly because a lot of false positives are generated because we are linting files from
    // different modules directly without their generated qmldirs.

    m_linter.setPluginsEnabled(true);

    for (auto &category : m_categories) {
        if (category.id() == qmlDeferredPropertyId || category.id() == qmlAttachedPropertyReuse) {
            category.setLevel(QtWarningMsg);
            category.setIgnored(false);
        } else {
            category.setLevel(QtCriticalMsg);
            category.setIgnored(true);
        }
    }
}

void tst_Sanity::initTestCase()
{
    QQmlDataTest::initTestCase();
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QString("import QtQuick.Templates 2.%1; Control { }").arg(15).toUtf8(), QUrl());

    const QStringList qmlTypeNames = QQmlMetaType::qmlTypeNames();

    // Collect the files from each style in the source tree.
    QDirIterator it(QQC2_IMPORT_PATH, QStringList() << "*.qml" << "*.js", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();
        if (qmlTypeNames.contains(QStringLiteral("QtQuick.Templates/") + info.baseName()))
            sourceQmlFiles.insert(info.dir().dirName() + "/" + info.fileName(), info.filePath());
    }

    // Then, collect the files from each installed style directory.
    const QVector<QPair<QString, QString>> styleRelativePaths = {
        { "basic", "QtQuick/Controls/Basic" },
        { "fusion", "QtQuick/Controls/Fusion" },
        { "material", "QtQuick/Controls/Material" },
        { "universal", "QtQuick/Controls/Universal" },
        // TODO: add native styles: QTBUG-87108
        { "ios", "QtQuick/Controls/iOS" }
    };
    for (const auto &stylePathPair : styleRelativePaths) {
        forEachControl(&engine, QQC2_IMPORT_PATH, stylePathPair.first, stylePathPair.second, QStringList(),
                [&](const QString &relativePath, const QUrl &absoluteUrl) {
             installedQmlFiles.insert(relativePath, absoluteUrl.toLocalFile());
        });
    }
}

void tst_Sanity::jsFiles()
{
    QMap<QString, QString>::const_iterator it;
    for (it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it) {
        if (QFileInfo(it.value()).suffix() == QStringLiteral("js"))
            QFAIL(qPrintable(it.value() +  ": JS files are not allowed"));
    }
}

void tst_Sanity::qmllint()
{
    QFETCH(QString, control);
    QFETCH(QString, filePath);

    QJsonArray output;
    bool success =
            m_linter.lintFile(filePath, nullptr, true, &output, m_importPaths, {}, {}, m_categories)
            == QQmlJSLinter::LintSuccess;

    QVERIFY2(success, qPrintable(QJsonDocument(output).toJson(QJsonDocument::Compact)));
}

void tst_Sanity::qmllint_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QString>("filePath");

    QMap<QString, QString>::const_iterator it;
    for (it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.key() << it.value();
}

void tst_Sanity::quickControlsSanityPlugin()
{
    QFETCH(QString, filePath);
    QFETCH(QString, result);

    QJsonArray output;

    bool hasWarnings = m_linter.lintFile(testFile(filePath), nullptr, true, &output, m_importPaths,
                                         {}, {}, m_categories)
            == QQmlJSLinter::HasWarnings;
    QVERIFY(hasWarnings);
    const auto &warningsOutput = output.first().toObject().value("warnings").toArray();
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
